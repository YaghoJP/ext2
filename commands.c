#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ext2_fs.h"

// Protótipos de ext2_ops.c
extern struct ext2_super_block sb;
extern unsigned int block_size;
int get_inode(unsigned int inode_num, struct ext2_inode *inode_buf);
void write_inode(unsigned int inode_num, const struct ext2_inode *inode_buf);
int read_block(unsigned int block_num, void *buffer);
// --- LINHA ADICIONADA AQUI ---
void write_block(unsigned int block_num, const void *buffer);
// --- FIM DA LINHA ADICIONADA ---
unsigned int find_inode_by_path(const char *path, unsigned int start_inode_num);
unsigned int alloc_inode();
unsigned int alloc_block();
void free_inode_resource(unsigned int inode_num);
void free_block_resource(unsigned int block_num);
int add_dir_entry(unsigned int parent_inode_num, unsigned int new_inode_num, const char *name, uint8_t file_type);
int remove_dir_entry(unsigned int parent_inode_num, const char *name_to_remove);

// --- Comandos de Leitura ---

void do_info() {
    printf("--- Informações do Superbloco ---\n");
    printf("Total de inodes: %u\n", sb.s_inodes_count);
    printf("Total de blocos: %u\n", sb.s_blocks_count);
    printf("Inodes livres: %u\n", sb.s_free_inodes_count);
    printf("Blocos livres: %u\n", sb.s_free_blocks_count);
    printf("Tamanho do bloco: %u bytes\n", block_size);
    printf("Última escrita: %s", ctime((time_t*)&sb.s_wtime));
}

void do_attr(unsigned int inode_num) {
    struct ext2_inode inode;
    if (get_inode(inode_num, &inode) != 0) {
        printf("Erro: Não foi possível obter o inode %u.\n", inode_num);
        return;
    }
    printf("--- Atributos do Inode ---\n");
    printf("Inode: %u\n", inode_num);
    printf("Tipo: ");
    if (inode.i_mode & EXT2_S_IFDIR) printf("Diretório\n");
    else if (inode.i_mode & EXT2_S_IFREG) printf("Arquivo Regular\n");
    else printf("Outro\n");
    printf("Tamanho: %u bytes\n", inode.i_size);
    printf("Contagem de links: %u\n", inode.i_links_count);
    printf("Blocos alocados: %u\n", inode.i_blocks / (block_size / 512));
    printf("Data de criação: %s", ctime((time_t*)&inode.i_ctime));
}

void do_ls(unsigned int dir_inode_num) {
    struct ext2_inode dir_inode;
    get_inode(dir_inode_num, &dir_inode);
    if (!(dir_inode.i_mode & EXT2_S_IFDIR)) {
        printf("ls: não é um diretório\n");
        return;
    }
    char block_buf[block_size];
    for (int i = 0; i < 12 && dir_inode.i_block[i] != 0; ++i) {
        read_block(dir_inode.i_block[i], block_buf);
        struct ext2_dir_entry_2 *entry = (struct ext2_dir_entry_2 *)block_buf;
        unsigned int offset = 0;
        while (offset < block_size && entry->rec_len > 0) {
            if (entry->inode != 0) {
                char name[entry->name_len + 1];
                memcpy(name, entry->name, entry->name_len);
                name[entry->name_len] = '\0';
                if (entry->file_type == EXT2_FT_DIR) printf("%s/  ", name);
                else printf("%s   ", name);
            }
            offset += entry->rec_len;
            entry = (struct ext2_dir_entry_2 *)((char *)block_buf + offset);
        }
    }
    printf("\n");
}

void do_cat(unsigned int file_inode_num) {
    struct ext2_inode file_inode;
    get_inode(file_inode_num, &file_inode);
    if (!(file_inode.i_mode & EXT2_S_IFREG)) {
        printf("cat: não é um arquivo regular\n");
        return;
    }
    char *block_buf = malloc(block_size);
    unsigned int bytes_remaining = file_inode.i_size;
    for (int i = 0; i < 12 && file_inode.i_block[i] != 0 && bytes_remaining > 0; i++) {
        read_block(file_inode.i_block[i], block_buf);
        unsigned int bytes_to_write = (bytes_remaining > block_size) ? block_size : bytes_remaining;
        fwrite(block_buf, 1, bytes_to_write, stdout);
        bytes_remaining -= bytes_to_write;
    }
    free(block_buf);
    printf("\n");
}

// --- Comandos de Escrita ---

void do_touch(unsigned int parent_inode_num, const char* filename) {
    if (find_inode_by_path(filename, parent_inode_num) != 0) {
        fprintf(stderr, "touch: arquivo '%s' já existe\n", filename);
        return;
    }
    unsigned int new_inode_num = alloc_inode();
    if (new_inode_num == 0) {
        fprintf(stderr, "touch: falha ao alocar inode\n");
        return;
    }
    struct ext2_inode new_inode = {0};
    new_inode.i_mode = EXT2_S_IFREG | 0644;
    new_inode.i_links_count = 1;
    new_inode.i_ctime = time(NULL);
    write_inode(new_inode_num, &new_inode);
    if (add_dir_entry(parent_inode_num, new_inode_num, filename, EXT2_FT_REG_FILE) != 0) {
        fprintf(stderr, "touch: falha ao adicionar entrada no diretório\n");
        free_inode_resource(new_inode_num); // Rollback
        return;
    }
    printf("Arquivo '%s' criado.\n", filename);
}

void do_mkdir(unsigned int parent_inode_num, const char* dirname) {
    if (find_inode_by_path(dirname, parent_inode_num) != 0) {
        fprintf(stderr, "mkdir: diretório '%s' já existe\n", dirname);
        return;
    }
    unsigned int new_inode_num = alloc_inode();
    unsigned int new_block_num = alloc_block();
    if (new_inode_num == 0 || new_block_num == 0) {
        fprintf(stderr, "mkdir: falha ao alocar recursos\n");
        if(new_inode_num) free_inode_resource(new_inode_num);
        if(new_block_num) free_block_resource(new_block_num);
        return;
    }

    struct ext2_inode new_dir_inode = {0};
    new_dir_inode.i_mode = EXT2_S_IFDIR | 0755;
    new_dir_inode.i_size = block_size;
    new_dir_inode.i_links_count = 2;
    new_dir_inode.i_ctime = time(NULL);
    new_dir_inode.i_blocks = block_size / 512;
    new_dir_inode.i_block[0] = new_block_num;
    write_inode(new_inode_num, &new_dir_inode);

    char block_buf[block_size];
    memset(block_buf, 0, block_size);
    struct ext2_dir_entry_2 *self_entry = (struct ext2_dir_entry_2*)block_buf;
    self_entry->inode = new_inode_num; self_entry->rec_len = 12; self_entry->name_len = 1; self_entry->file_type = EXT2_FT_DIR; strcpy(self_entry->name, ".");
    struct ext2_dir_entry_2 *parent_entry = (struct ext2_dir_entry_2*)(block_buf + 12);
    parent_entry->inode = parent_inode_num; parent_entry->rec_len = block_size - 12; parent_entry->name_len = 2; parent_entry->file_type = EXT2_FT_DIR; strcpy(parent_entry->name, "..");
    write_block(new_block_num, block_buf);

    if (add_dir_entry(parent_inode_num, new_inode_num, dirname, EXT2_FT_DIR) != 0) return;
    
    struct ext2_inode parent_inode;
    get_inode(parent_inode_num, &parent_inode);
    parent_inode.i_links_count++;
    write_inode(parent_inode_num, &parent_inode);
    
    printf("Diretório '%s' criado.\n", dirname);
}

void do_rm(unsigned int parent_inode_num, const char *filename) {
    unsigned int target_inode_num = find_inode_by_path(filename, parent_inode_num);
    if (target_inode_num == 0) {
        fprintf(stderr, "rm: arquivo '%s' não encontrado\n", filename);
        return;
    }
    struct ext2_inode target_inode;
    get_inode(target_inode_num, &target_inode);
    if(target_inode.i_mode & EXT2_S_IFDIR) {
        fprintf(stderr, "rm: '%s' é um diretório. Use rmdir.\n", filename);
        return;
    }
    if (remove_dir_entry(parent_inode_num, filename) != 0) {
        fprintf(stderr, "rm: falha ao remover entrada de diretório\n");
        return;
    }
    target_inode.i_links_count--;
    if (target_inode.i_links_count == 0) {
        for(int i = 0; i < 12 && target_inode.i_block[i] != 0; i++) {
            free_block_resource(target_inode.i_block[i]);
        }
        free_inode_resource(target_inode_num);
    } else {
        write_inode(target_inode_num, &target_inode);
    }
    printf("Arquivo '%s' removido.\n", filename);
}

void do_rmdir(unsigned int parent_inode_num, const char *dirname) {
    unsigned int target_inode_num = find_inode_by_path(dirname, parent_inode_num);
    if (target_inode_num == 0) {
        fprintf(stderr, "rmdir: diretório '%s' não encontrado\n", dirname);
        return;
    }
    struct ext2_inode target_inode;
    get_inode(target_inode_num, &target_inode);
    if(!(target_inode.i_mode & EXT2_S_IFDIR)) {
        fprintf(stderr, "rmdir: '%s' não é um diretório.\n", dirname);
        return;
    }
    if(target_inode.i_links_count > 2) {
        fprintf(stderr, "rmdir: diretório '%s' não está vazio.\n", dirname);
        return;
    }
    
    if (remove_dir_entry(parent_inode_num, dirname) != 0) return;
    
    free_block_resource(target_inode.i_block[0]);
    free_inode_resource(target_inode_num);

    struct ext2_inode parent_inode;
    get_inode(parent_inode_num, &parent_inode);
    parent_inode.i_links_count--;
    write_inode(parent_inode_num, &parent_inode);

    printf("Diretório '%s' removido.\n", dirname);
}

void do_rename(unsigned int parent_inode_num, const char* oldname, const char* newname) {
    unsigned int target_inode_num = find_inode_by_path(oldname, parent_inode_num);
    if (target_inode_num == 0) {
        fprintf(stderr, "rename: '%s' não encontrado.\n", oldname);
        return;
    }
    if (find_inode_by_path(newname, parent_inode_num) != 0) {
        fprintf(stderr, "rename: '%s' já existe.\n", newname);
        return;
    }
    struct ext2_inode target_inode;
    get_inode(target_inode_num, &target_inode);
    uint8_t ftype = (target_inode.i_mode & EXT2_S_IFDIR) ? EXT2_FT_DIR : EXT2_FT_REG_FILE;

    if (add_dir_entry(parent_inode_num, target_inode_num, newname, ftype) != 0) return;
    if (remove_dir_entry(parent_inode_num, oldname) != 0) return;
    
    printf("'%s' renomeado para '%s'.\n", oldname, newname);
}

void copy_block_to_file(uint32_t block_num, FILE *dest_file, unsigned int *bytes_remaining, char *block_buf) {
    if (*bytes_remaining == 0) return;
    read_block(block_num, block_buf);
    unsigned int bytes_to_write = (*bytes_remaining < block_size) ? *bytes_remaining : block_size;
    fwrite(block_buf, 1, bytes_to_write, dest_file);
    *bytes_remaining -= bytes_to_write;
}

void do_cp(unsigned int current_dir_inode, const char* source_in_image, const char* dest_on_host) {
    unsigned int source_inode_num = find_inode_by_path(source_in_image, current_dir_inode);
    if (source_inode_num == 0) {
        printf("cp: arquivo de origem '%s' não encontrado na imagem.\n", source_in_image);
        return;
    }

    struct ext2_inode source_inode;
    get_inode(source_inode_num, &source_inode);

    if (!(source_inode.i_mode & EXT2_S_IFREG)) {
        printf("cp: '%s' não é um arquivo regular.\n", source_in_image);
        return;
    }

    FILE* dest_file = fopen(dest_on_host, "wb");
    if (!dest_file) {
        perror("cp: falha ao abrir arquivo de destino no sistema");
        return;
    }

    char *block_buf = malloc(block_size);
    unsigned int bytes_remaining = source_inode.i_size;

    // 🔹 1. Diretos
    for (int i = 0; i < 12 && bytes_remaining > 0; i++) {
        if (source_inode.i_block[i] != 0) {
            copy_block_to_file(source_inode.i_block[i], dest_file, &bytes_remaining, block_buf);
        }
    }

    // 🔹 2. Indireto
    if (bytes_remaining > 0 && source_inode.i_block[12] != 0) {
        uint32_t *indirect_blocks = malloc(block_size);
        read_block(source_inode.i_block[12], (char*)indirect_blocks);
        int num_ptrs = block_size / sizeof(uint32_t);

        for (int i = 0; i < num_ptrs && bytes_remaining > 0; i++) {
            if (indirect_blocks[i] != 0) {
                copy_block_to_file(indirect_blocks[i], dest_file, &bytes_remaining, block_buf);
            }
        }

        free(indirect_blocks);
    }

    // 🔹 3. Duplamente indireto
    if (bytes_remaining > 0 && source_inode.i_block[13] != 0) {
        uint32_t *double_indirect = malloc(block_size);
        read_block(source_inode.i_block[13], (char*)double_indirect);
        int num_ptrs = block_size / sizeof(uint32_t);

        for (int i = 0; i < num_ptrs && bytes_remaining > 0; i++) {
            if (double_indirect[i] != 0) {
                uint32_t *indirect = malloc(block_size);
                read_block(double_indirect[i], (char*)indirect);

                for (int j = 0; j < num_ptrs && bytes_remaining > 0; j++) {
                    if (indirect[j] != 0) {
                        copy_block_to_file(indirect[j], dest_file, &bytes_remaining, block_buf);
                    }
                }

                free(indirect);
            }
        }

        free(double_indirect);
    }

    free(block_buf);
    fclose(dest_file);
    printf("Arquivo '%s' copiado para '%s'.\n", source_in_image, dest_on_host);
}