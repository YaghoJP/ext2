#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ext2_fs.h"

// Protótipos de `commands.c`
void do_info();
void do_attr(unsigned int inode_num);
void do_ls(unsigned int dir_inode_num);
void do_cat(unsigned int file_inode_num);
void do_touch(unsigned int parent_inode_num, const char* filename);
void do_mkdir(unsigned int parent_inode_num, const char* dirname);
void do_rm(unsigned int parent_inode_num, const char *filename);
void do_rmdir(unsigned int parent_inode_num, const char *dirname);
void do_rename(unsigned int parent_inode_num, const char* oldname, const char* newname);
void do_cp(unsigned int current_dir_inode, const char* source_in_image, const char* dest_on_host);

// Protótipos de `ext2_ops.c`
int ext2_init(const char *image_path);
void ext2_exit();
unsigned int find_inode_by_path(const char *path, unsigned int start_inode_num);
int get_inode(unsigned int inode_num,  ext2_inode *inode_buf);

// Estado do Shell
static unsigned int current_inode = EXT2_ROOT_INO;
static char current_path[1024] = "/";

// Função para atualizar o string do caminho (simplificada)
void update_path(const char *new_dir_name) {
    if (strcmp(new_dir_name, ".") == 0) return;
    if (strcmp(new_dir_name, "..") == 0) {
        if (strcmp(current_path, "/") != 0) {
            char *last_slash = strrchr(current_path, '/');
            if (last_slash == current_path) *(last_slash + 1) = '\0';
            else *last_slash = '\0';
        }
    } else {
        if (strcmp(current_path, "/") != 0) strcat(current_path, "/");
        strcat(current_path, new_dir_name);
    }
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <arquivo_de_imagem_ext2>\n", argv[0]);
        return 1;
    }

    if (ext2_init(argv[1]) != 0) return 1;

    char line[256];
    char cmd[32], arg1[128], arg2[128];

    while (1) {
        printf("ext2shell:[%s] $ ", current_path);
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) break;

        memset(cmd, 0, sizeof(cmd)); memset(arg1, 0, sizeof(arg1)); memset(arg2, 0, sizeof(arg2));
        sscanf(line, "%31s %127s %127s", cmd, arg1, arg2);

        if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0 || strcmp(cmd, "sair") == 0) break;
        else if (strcmp(cmd, "info") == 0) do_info();
        else if (strcmp(cmd, "ls") == 0) do_ls(current_inode);
        else if (strcmp(cmd, "pwd") == 0) printf("%s\n", current_path);
        else if (strcmp(cmd, "attr") == 0) {
            if (!*arg1) printf("Uso: attr <arquivo|diretorio>\n");
            else {
                unsigned int ino = find_inode_by_path(arg1, current_inode);
                if (ino) do_attr(ino);
                else printf("attr: '%s' não encontrado.\n", arg1);
            }
        }
        else if (strcmp(cmd, "cat") == 0) {
            if (!*arg1) printf("Uso: cat <arquivo>\n");
            else {
                unsigned int ino = find_inode_by_path(arg1, current_inode);
                if(ino) do_cat(ino);
                else printf("cat: '%s' não encontrado.\n", arg1);
            }
        }
        else if (strcmp(cmd, "cd") == 0) {
            if (!*arg1) { current_inode = EXT2_ROOT_INO; strcpy(current_path, "/"); }
            else {
                unsigned int ino = find_inode_by_path(arg1, current_inode);
                if (ino) {
                      ext2_inode new_dir_inode;
                    get_inode(ino, &new_dir_inode);
                    if (new_dir_inode.i_mode & EXT2_S_IFDIR) {
                        current_inode = ino;
                        update_path(arg1);
                    } else printf("cd: '%s' não é um diretório\n", arg1);
                } else printf("cd: '%s' não encontrado\n", arg1);
            }
        }
        else if (strcmp(cmd, "touch") == 0) {
            if (!*arg1) printf("Uso: touch <nome_arquivo>\n");
            else do_touch(current_inode, arg1);
        }
        else if (strcmp(cmd, "mkdir") == 0) {
            if (!*arg1) printf("Uso: mkdir <nome_diretorio>\n");
            else do_mkdir(current_inode, arg1);
        }
        else if (strcmp(cmd, "rm") == 0) {
            if (!*arg1) printf("Uso: rm <nome_arquivo>\n");
            else do_rm(current_inode, arg1);
        }
        else if (strcmp(cmd, "rmdir") == 0) {
            if (!*arg1) printf("Uso: rmdir <nome_diretorio>\n");
            else do_rmdir(current_inode, arg1);
        }
        else if (strcmp(cmd, "rename") == 0 || strcmp(cmd, "mv") == 0) {
            if (!*arg1 || !*arg2) printf("Uso: rename <nome_antigo> <nome_novo>\n");
            else do_rename(current_inode, arg1, arg2);
        }
        else if (strcmp(cmd, "cp") == 0) {
             if (!*arg1 || !*arg2) printf("Uso: cp <origem_na_imagem> <destino_no_host>\n");
             else do_cp(current_inode, arg1, arg2);
        }
        else if (strlen(cmd) > 0) {
            printf("Comando não encontrado: %s\n", cmd);
        }
    }

    ext2_exit();
    printf("\nSaindo do ext2shell.\n");
    return 0;
}