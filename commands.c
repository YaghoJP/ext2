#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ext2_fs.h"

// Protótipos de ext2_ops.c
extern ext2_super_block sb;
extern unsigned int block_size;
int get_inode(unsigned int inode_num, ext2_inode *inode_buf);
void write_inode(unsigned int inode_num, const ext2_inode *inode_buf);
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
int read_superblock(ext2_super_block *sb);
int read_inode(uint32_t inode_num, ext2_inode *inode);
// --- Comandos de Leitura ---

void do_info() {

    printf("\nVolume name.....: %s\n", sb.s_volume_name);
    printf("Image size......: %lu bytes\n", block_size * sb.s_blocks_count * 1UL);
    printf("Free space......: %u KiB\n", sb.s_free_blocks_count * block_size / 1024);
    printf("Free inodes.....: %u\n", sb.s_free_inodes_count);
    printf("Free blocks.....: %u\n", sb.s_free_blocks_count);
    printf("Block size......: %u bytes\n", block_size);
    printf("Inode size......: %lu bytes\n", sizeof(ext2_inode));
    printf("Groups count....: %u\n", sb.s_blocks_count / sb.s_blocks_per_group);
    printf("Groups size.....: %u blocks\n", sb.s_blocks_per_group);
    printf("Groups inodes...: %u inodes\n", sb.s_inodes_per_group);
    printf("Inodetable size.: %lu blocks\n\n", (sb.s_inodes_per_group * sizeof(ext2_inode)) / block_size);
}

void do_attr(unsigned int inode_num) {
    ext2_inode inode;
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
    time_t mtime = inode.i_ctime;
    struct tm *tm_info = localtime(&mtime);
    char date_buf[64];
    strftime(date_buf, sizeof(date_buf), "%d/%m/%Y %H:%M", tm_info);
    printf("%s\n", date_buf);
    printf("Data de criação: %s", date_buf);
}

void do_ls(unsigned int dir_inode_num) {
    ext2_inode dir_inode;
    get_inode(dir_inode_num, &dir_inode);
    if (!(dir_inode.i_mode & EXT2_S_IFDIR)) {
        printf("ls: não é um diretório\n");
        return;
    }
    char block_buf[block_size];
    for (int i = 0; i < 12 && dir_inode.i_block[i] != 0; ++i) {
        read_block(dir_inode.i_block[i], block_buf);
          ext2_dir_entry_2 *entry = (  ext2_dir_entry_2 *)block_buf;
        unsigned int offset = 0;
        while (offset < block_size && entry->rec_len > 0) {
            if (entry->inode != 0) {
                char name[entry->name_len + 1];
                memcpy(name, entry->name, entry->name_len);
                name[entry->name_len] = '\0';
                printf("%s\n", name);
                printf("inode: %u\n", entry->inode);
                printf("record length: %u\n", entry->rec_len);
                printf("name length: %u\n", entry->name_len);
                printf("file type: %u\n\n", entry->file_type);
            }
            offset += entry->rec_len;
            entry = (  ext2_dir_entry_2 *)((char *)block_buf + offset);
        }
    }
    printf("\n");
}

void do_cat(unsigned int file_inode_num) {
      ext2_inode file_inode;
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
      ext2_inode new_inode = {0};
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

      ext2_inode new_dir_inode = {0};
    new_dir_inode.i_mode = EXT2_S_IFDIR | 0755;
    new_dir_inode.i_size = block_size;
    new_dir_inode.i_links_count = 2;
    new_dir_inode.i_ctime = time(NULL);
    new_dir_inode.i_blocks = block_size / 512;
    new_dir_inode.i_block[0] = new_block_num;
    write_inode(new_inode_num, &new_dir_inode);

    char block_buf[block_size];
    memset(block_buf, 0, block_size);
      ext2_dir_entry_2 *self_entry = (  ext2_dir_entry_2*)block_buf;
    self_entry->inode = new_inode_num; self_entry->rec_len = 12; self_entry->name_len = 1; self_entry->file_type = EXT2_FT_DIR; strcpy(self_entry->name, ".");
      ext2_dir_entry_2 *parent_entry = (  ext2_dir_entry_2*)(block_buf + 12);
    parent_entry->inode = parent_inode_num; parent_entry->rec_len = block_size - 12; parent_entry->name_len = 2; parent_entry->file_type = EXT2_FT_DIR; strcpy(parent_entry->name, "..");
    write_block(new_block_num, block_buf);

    if (add_dir_entry(parent_inode_num, new_inode_num, dirname, EXT2_FT_DIR) != 0) return;
    
      ext2_inode parent_inode;
    get_inode(parent_inode_num, &parent_inode);
    parent_inode.i_links_count++;
    write_inode(parent_inode_num, &parent_inode);
    
    printf("Diretório '%s' criado.\n", dirname);
}

void free_indirect_blocks(uint32_t block_ptr, int level) {
    if (block_ptr == 0 || block_ptr < sb.s_first_data_block) {
        return;  // Bloco inválido
    }

    uint32_t *blocks = malloc(block_size);
    if (read_block(block_ptr, blocks) != 0) {
        free(blocks);
        return;  // Falha na leitura
    }

    if (level == 1) {
        // Nível de dados
        for (int i = 0; i < block_size/sizeof(uint32_t); i++) {
            if (blocks[i] != 0 && blocks[i] >= sb.s_first_data_block) {
                free_block_resource(blocks[i]);
            }
        }
    } else {
        // Níveis de ponteiros
        for (int i = 0; i < block_size/sizeof(uint32_t); i++) {
            if (blocks[i] != 0 && blocks[i] >= sb.s_first_data_block) {
                free_indirect_blocks(blocks[i], level - 1);
            }
        }
    }

    free_block_resource(block_ptr);  // Libera o bloco atual
    free(blocks);
}

void free_all_blocks(ext2_inode *inode) {
    // 1. Libera blocos diretos (0-11)
    for (int i = 0; i < 12 && inode->i_block[i] != 0; i++) {
        free_block_resource(inode->i_block[i]);
    }

    // 2. Libera indireto simples (bloco 12) - nível 1
    if (inode->i_block[12] != 0) {
        free_indirect_blocks(inode->i_block[12], 1);
    }

    // 3. Libera indireto duplo (bloco 13) - nível 2
    if (inode->i_block[13] != 0) {
        free_indirect_blocks(inode->i_block[13], 2);
    }

    // 4. Para sistemas muito grandes: indireto triplo (bloco 14) - nível 3
    // if (inode->i_block[14] != 0) {
    //     free_indirect_blocks(inode->i_block[14], 3);
    // }
}

void do_rm(unsigned int parent_inode_num, const char *filename) {
    unsigned int target_inode_num = find_inode_by_path(filename, parent_inode_num);
    if (target_inode_num == 0) {
        fprintf(stderr, "rm: arquivo '%s' não encontrado\n", filename);
        return;
    }

    ext2_inode target_inode;
    get_inode(target_inode_num, &target_inode);

    if (target_inode.i_mode & EXT2_S_IFDIR) {
        fprintf(stderr, "rm: '%s' é um diretório. Use rmdir.\n", filename);
        return;
    }

    if (remove_dir_entry(parent_inode_num, filename) != 0) {
        fprintf(stderr, "rm: falha ao remover entrada de diretório\n");
        return;
    }

    target_inode.i_links_count--;
    if (target_inode.i_links_count == 0) {
        free_all_blocks(&target_inode);
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
      ext2_inode target_inode;
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

      ext2_inode parent_inode;
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
      ext2_inode target_inode;
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

      ext2_inode source_inode;
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

void cmd_print_superblock() {
    ext2_super_block sb;
    read_superblock(&sb);

    printf("inodes count: %u\n", sb.s_inodes_count);
    printf("blocks count: %u\n", sb.s_blocks_count);
    printf("reserved blocks count: %u\n", sb.s_r_blocks_count);
    printf("free blocks count: %u\n", sb.s_free_blocks_count);
    printf("free inodes count: %u\n", sb.s_free_inodes_count);
    printf("first data block: %u\n", sb.s_first_data_block);
    printf("block size: %u\n", 1024 << sb.s_log_block_size);
    printf("fragment size: %u\n", 1024 << sb.s_log_frag_size);
    printf("blocks per group: %u\n", sb.s_blocks_per_group);
    printf("fragments per group: %u\n", sb.s_frags_per_group);
    printf("inodes per group: %u\n", sb.s_inodes_per_group);
    printf("mount time: %u\n", sb.s_mtime);
    printf("write time: %u\n", sb.s_wtime);
    printf("mount count: %u\n", sb.s_mnt_count);
    printf("max mount count: %u\n", sb.s_max_mnt_count);
    printf("magic signature: 0x%x\n", sb.s_magic);
    printf("file system state: %u\n", sb.s_state);
    printf("errors: %u\n", sb.s_errors);
    printf("minor revision level: %u\n", sb.s_minor_rev_level);

    // last check: formatado em data
    time_t t = sb.s_lastcheck;
    struct tm *lt = localtime(&t);
    printf("time of last check: %02d/%02d/%04d %02d:%02d\n",
           lt->tm_mday, lt->tm_mon + 1, lt->tm_year + 1900,
           lt->tm_hour, lt->tm_min);

    printf("max check interval: %u\n", sb.s_checkinterval);
    printf("creator OS: %u\n", sb.s_creator_os);
    printf("revision level: %u\n", sb.s_rev_level);
    printf("default uid reserved blocks: %u\n", sb.s_def_resuid);
    printf("defautl gid reserved blocks: %u\n", sb.s_def_resgid);
    printf("first non-reserved inode: %u\n", sb.s_first_ino);
    printf("inode size: %u\n", sb.s_inode_size);
    printf("block group number: %u\n", sb.s_block_group_nr);
    printf("compatible feature set: %u\n", sb.s_feature_compat);
    printf("incompatible feature set: %u\n", sb.s_feature_incompat);
    printf("read only comp feature set: %u\n", sb.s_feature_ro_compat);

    printf("volume UUID: ");
    for (int i = 0; i < 16; i++) printf("%02x", sb.s_uuid[i]);
    printf("\n");

    printf("volume name: %.*s\n", 16, sb.s_volume_name);
    printf("volume last mounted: %.*s\n", 64, sb.s_last_mounted);
    printf("algorithm usage bitmap: %u\n", sb.s_algorithm_usage_bitmap);
    printf("blocks to try to preallocate: %u\n", sb.s_prealloc_blocks);
    printf("blocks preallocate dir: %u\n", sb.s_prealloc_dir_blocks);

    printf("journal UUID: ");
    //for (int i = 0; i < 16; i++) printf("%02x", sb.s_journal_uuid[i]);
    printf("\n");

    printf("journal INum: %u\n", sb.s_journal_inum);
    printf("journal Dev: %u\n", sb.s_journal_dev);
    printf("last orphan: %u\n", sb.s_last_orphan);

    printf("hash seed: ");
    for (int i = 0; i < 4; i++) printf("%08x", sb.s_hash_seed[i]);
    printf("\n");

    printf("default hash version: %u\n", sb.s_def_hash_version);
    printf("default mount options: %u\n", sb.s_default_mount_opts);
    printf("first meta: %u\n", sb.s_first_meta_bg);
}


void cmd_print_groups() {
    ext2_super_block sb;
    read_superblock(&sb);

    uint32_t block_size = 1024 << sb.s_log_block_size;
    int group_count = sb.s_blocks_count / sb.s_blocks_per_group;

    uint8_t buf[1024];
    read_block(2, buf);  // Descritores começam no bloco 2 (geralmente)

    for (int i = 0; i < group_count; i++) {
        ext2_group_desc *gd = (ext2_group_desc *)(buf + i * sizeof(ext2_group_desc));
        printf("Block Group Descriptor %d:\n", i);
        printf("block bitmap: %u\n", gd->bg_block_bitmap);
        printf("inode bitmap: %u\n", gd->bg_inode_bitmap);
        printf("inode table: %u\n", gd->bg_inode_table);
        printf("free blocks count: %u\n", gd->bg_free_blocks_count);
        printf("free inodes count: %u\n", gd->bg_free_inodes_count);
        printf("used dirs count: %u\n", gd->bg_used_dirs_count);
    }
}

void cmd_print_inode(uint32_t inode_num) {
    ext2_inode inode;
    read_inode(inode_num, &inode);

    printf("file format and access rights: 0x%x\n", inode.i_mode);
    printf("user id: %u\n", inode.i_uid);
    printf("lower 32-bit file size: %u\n", inode.i_size);
    printf("access time: %u\n", inode.i_atime);
    printf("creation time: %u\n", inode.i_ctime);
    printf("modification time: %u\n", inode.i_mtime);
    printf("deletion time: %u\n", inode.i_dtime);
    printf("group id: %u\n", inode.i_gid);
    printf("link count inode: %u\n", inode.i_links_count);
    printf("512-bytes blocks: %u\n", inode.i_blocks);
    printf("ext2 flags: %u\n", inode.i_flags);
    printf("reserved (Linux): %u\n", inode.i_osd1);

    for (int i = 0; i < EXT2_N_BLOCKS; i++) {
        printf("pointer[%d]: %u\n", i, inode.i_block[i]);
    }

    printf("file version (nfs): %u\n", inode.i_generation);
    printf("block number extended attributes: %u\n", inode.i_file_acl);
    printf("higher 32-bit file size: %u\n", inode.i_dir_acl);
    printf("location file fragment: %u\n", inode.i_faddr);
}
