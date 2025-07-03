#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "ext2_fs.h"

// Variáveis globais
FILE *disk_image = NULL;
ext2_super_block sb;
ext2_group_desc *gd = NULL;
unsigned int block_size = 0;
unsigned int inodes_per_block = 0;
unsigned int group_count = 0;

// === Funções de Leitura/Escrita de Baixo Nível ===

void write_block(unsigned int block_num, const void *buffer) {
    if (fseek(disk_image, block_num * block_size, SEEK_SET) != 0) {
        perror("fseek write");
        return;
    }
    if (fwrite(buffer, block_size, 1, disk_image) != 1) {
        perror("fwrite block");
    }
}

int read_block(unsigned int block_num, void *buffer) {
    if (fseek(disk_image, block_num * block_size, SEEK_SET) != 0) {
        perror("fseek read");
        return -1;
    }
    if (fread(buffer, block_size, 1, disk_image) != 1) {
        // EOF pode ser normal
        return -1;
    }
    return 0;
}

void write_superblock() {
    fseek(disk_image, 1024, SEEK_SET);
    fwrite(&sb, sizeof(  ext2_super_block), 1, disk_image);
}

void write_group_descriptors() {
    unsigned int gd_block = (sb.s_first_data_block == 0) ? 2 : sb.s_first_data_block + 1;
    fseek(disk_image, gd_block * block_size, SEEK_SET);
    fwrite(gd, sizeof(  ext2_group_desc) * group_count, 1, disk_image);
}

int ext2_init(const char *image_path) {
    disk_image = fopen(image_path, "r+b");
    if (!disk_image) {
        perror("Falha ao abrir a imagem do disco");
        return -1;
    }
    fseek(disk_image, 1024, SEEK_SET);
    fread(&sb, sizeof(  ext2_super_block), 1, disk_image);

    if (sb.s_magic != EXT2_SUPER_MAGIC) {
        fprintf(stderr, "Não é um sistema de arquivos EXT2 (magic: 0x%x)\n", sb.s_magic);
        fclose(disk_image);
        return -1;
    }

    block_size = 1024 << sb.s_log_block_size;
    inodes_per_block = block_size / sizeof(  ext2_inode);
    group_count = (sb.s_blocks_count + sb.s_blocks_per_group - 1) / sb.s_blocks_per_group;

    gd = malloc(group_count * sizeof(  ext2_group_desc));
    unsigned int gd_block = (sb.s_first_data_block == 0) ? 2 : sb.s_first_data_block + 1;
    fseek(disk_image, gd_block * block_size, SEEK_SET);
    fread(gd, sizeof(  ext2_group_desc), group_count, disk_image);

    return 0;
}

void ext2_exit() {
    if (gd) free(gd);
    if (disk_image) {
        fflush(disk_image);
        fclose(disk_image);
    }
}


// === Funções de Inode ===

int get_inode(unsigned int inode_num,   ext2_inode *inode_buf) {
    if (inode_num == 0 || inode_num > sb.s_inodes_count) return -1;
    
    inode_num--;
    unsigned int group = inode_num / sb.s_inodes_per_group;
    unsigned int index = inode_num % sb.s_inodes_per_group;
    unsigned int block = gd[group].bg_inode_table + (index / inodes_per_block);
    unsigned int offset = (index % inodes_per_block) * sizeof(ext2_inode);
    
    char buffer[block_size];
    read_block(block, buffer);
    memcpy(inode_buf, buffer + offset, sizeof(ext2_inode));
    return 0;
}

void write_inode(unsigned int inode_num, const   ext2_inode *inode_buf) {
    inode_num--;
    unsigned int group = inode_num / sb.s_inodes_per_group;
    unsigned int index = inode_num % sb.s_inodes_per_group;
    unsigned int block = gd[group].bg_inode_table + (index / inodes_per_block);
    unsigned int offset = (index % inodes_per_block) * sizeof(  ext2_inode);
    
    char buffer[block_size];
    read_block(block, buffer);
    memcpy(buffer + offset, inode_buf, sizeof(  ext2_inode));
    write_block(block, buffer);
}

// === Funções de Alocação e Liberação ===

unsigned int alloc_inode() {
    char bitmap[block_size];
    for (unsigned int group = 0; group < group_count; group++) {
        if (gd[group].bg_free_inodes_count > 0) {
            read_block(gd[group].bg_inode_bitmap, bitmap);
            for (unsigned int i = 0; i < sb.s_inodes_per_group; i++) {
                if (!((bitmap[i / 8] >> (i % 8)) & 1)) {
                    bitmap[i / 8] |= (1 << (i % 8));
                    write_block(gd[group].bg_inode_bitmap, bitmap);
                    gd[group].bg_free_inodes_count--;
                    sb.s_free_inodes_count--;
                    write_group_descriptors();
                    write_superblock();
                    return (group * sb.s_inodes_per_group) + i + 1;
                }
            }
        }
    }
    return 0;
}

void free_inode_resource(unsigned int inode_num) {
    inode_num--;
    unsigned int group = inode_num / sb.s_inodes_per_group;
    unsigned int index = inode_num % sb.s_inodes_per_group;
    char bitmap[block_size];
    read_block(gd[group].bg_inode_bitmap, bitmap);
    bitmap[index / 8] &= ~(1 << (index % 8)); // Limpa o bit
    write_block(gd[group].bg_inode_bitmap, bitmap);
    gd[group].bg_free_inodes_count++;
    sb.s_free_inodes_count++;
    write_group_descriptors();
    write_superblock();
}


unsigned int alloc_block() {
    char bitmap[block_size];
    for (unsigned int group = 0; group < group_count; group++) {
        if (gd[group].bg_free_blocks_count > 0) {
            read_block(gd[group].bg_block_bitmap, bitmap);
            for (unsigned int i = 0; i < sb.s_blocks_per_group; i++) {
                if (!((bitmap[i / 8] >> (i % 8)) & 1)) {
                    bitmap[i / 8] |= (1 << (i % 8));
                    write_block(gd[group].bg_block_bitmap, bitmap);
                    gd[group].bg_free_blocks_count--;
                    sb.s_free_blocks_count--;
                    write_group_descriptors();
                    write_superblock();
                    return (group * sb.s_blocks_per_group) + i + sb.s_first_data_block;
                }
            }
        }
    }
    return 0;
}

void free_block_resource(unsigned int block_num) {
    if (block_num == 0) return;
    block_num -= sb.s_first_data_block;
    unsigned int group = block_num / sb.s_blocks_per_group;
    unsigned int index = block_num % sb.s_blocks_per_group;
    char bitmap[block_size];
    read_block(gd[group].bg_block_bitmap, bitmap);
    bitmap[index / 8] &= ~(1 << (index % 8));
    write_block(gd[group].bg_block_bitmap, bitmap);
    gd[group].bg_free_blocks_count++;
    sb.s_free_blocks_count++;
    write_group_descriptors();
    write_superblock();
}

// === Funções de Diretório ===

unsigned int search_directory(unsigned int dir_inode_num, const char *name) {
      ext2_inode dir_inode;
    if (get_inode(dir_inode_num, &dir_inode) != 0 || !(dir_inode.i_mode & EXT2_S_IFDIR)) return 0;
    
    char block_buf[block_size];
    for (int i = 0; i < 12 && dir_inode.i_block[i] != 0; ++i) {
        read_block(dir_inode.i_block[i], block_buf);
          ext2_dir_entry_2 *entry = (  ext2_dir_entry_2 *)block_buf;
        unsigned int offset = 0;
        while (offset < block_size && entry->rec_len > 0) {
            if (entry->inode != 0 && strncmp(name, entry->name, entry->name_len) == 0 && strlen(name) == entry->name_len) {
                return entry->inode;
            }
            offset += entry->rec_len;
            entry = (  ext2_dir_entry_2 *)((char *)block_buf + offset);
        }
    }
    return 0;
}

unsigned int find_inode_by_path(const char *path, unsigned int start_inode_num) {
    if (path == NULL || strlen(path) == 0) return 0;
    char path_copy[1024];
    strcpy(path_copy, path);
    
    unsigned int current_inode_num = start_inode_num;
    if (path_copy[0] == '/') {
        current_inode_num = EXT2_ROOT_INO;
    }

    char *token = strtok(path_copy, "/");
    while (token != NULL) {
        current_inode_num = search_directory(current_inode_num, token);
        if (current_inode_num == 0) return 0;
        token = strtok(NULL, "/");
    }
    return current_inode_num;
}


int read_superblock(ext2_super_block *sb) {
    uint8_t buf[1024];

    // O superbloco sempre está no offset 1024 (ou bloco 1 se block_size == 1024)
    if (read_block(1, buf) != 0) {
        fprintf(stderr, "Erro ao ler o bloco do superbloco.\n");
        return -1;
    }

    memcpy(sb, buf, sizeof(ext2_super_block));

    if (sb->s_magic != EXT2_SUPER_MAGIC) {
        fprintf(stderr, "Sistema de arquivos inválido. Magic: 0x%x\n", sb->s_magic);
        return -1;
    }

    return 0;
}

int add_dir_entry(unsigned int parent_inode_num, unsigned int new_inode_num, const char *name, uint8_t file_type) {
      ext2_inode parent_inode;
    get_inode(parent_inode_num, &parent_inode);
    char block_buf[block_size];

    unsigned char name_len = strlen(name);
    unsigned short needed_len = 8 + name_len;
    if (needed_len % 4 != 0) needed_len = (needed_len / 4 + 1) * 4;

    for (int i = 0; i < 12 && parent_inode.i_block[i] != 0; i++) {
        read_block(parent_inode.i_block[i], block_buf);
          ext2_dir_entry_2 *entry = (  ext2_dir_entry_2 *)block_buf;
        unsigned int offset = 0;
        
        while (offset < block_size && entry->rec_len > 0) {
            unsigned short ideal_len = 8 + entry->name_len;
            if (ideal_len % 4 != 0) ideal_len = (ideal_len / 4 + 1) * 4;

            if (offset + entry->rec_len >= block_size && entry->rec_len >= ideal_len + needed_len) {
                unsigned short old_rec_len = entry->rec_len;
                entry->rec_len = ideal_len;
                
                offset += entry->rec_len;
                  ext2_dir_entry_2 *new_entry = (  ext2_dir_entry_2 *)(block_buf + offset);
                new_entry->inode = new_inode_num;
                new_entry->rec_len = old_rec_len - entry->rec_len;
                new_entry->name_len = name_len;
                new_entry->file_type = file_type;
                memcpy(new_entry->name, name, name_len);

                write_block(parent_inode.i_block[i], block_buf);
                return 0;
            }
            offset += entry->rec_len;
            entry = (  ext2_dir_entry_2 *)(block_buf + offset);
        }
    }
    fprintf(stderr, "Erro: Sem espaço no diretório para criar nova entrada.\n");
    return -1;
}

int read_group_desc(uint32_t group_num, ext2_group_desc *desc) {
    if (desc == NULL) return -1;

    uint32_t group_desc_table_block = (block_size == 1024) ? 2 : 1;
    uint32_t offset = group_num * sizeof(ext2_group_desc);
    uint32_t block_offset = offset / block_size;
    uint32_t offset_in_block = offset % block_size;

    uint8_t buf[1024];
    if (read_block(group_desc_table_block + block_offset, buf) != 0) {
        fprintf(stderr, "Erro ao ler descritor do grupo %u\n", group_num);
        return -1;
    }

    memcpy(desc, buf + offset_in_block, sizeof(ext2_group_desc));
    return 0;
}

int read_inode(uint32_t inode_num, ext2_inode *inode_out) {
    if (inode_num == 0) return -1;

    ext2_super_block sb;
    if (read_superblock(&sb) != 0) return -1;

    // Quantos inodes por grupo existem
    uint32_t inodes_per_group = sb.s_inodes_per_group;
    uint32_t inode_size = sb.s_inode_size;

    // Qual grupo contém esse inode?
    uint32_t group = (inode_num - 1) / inodes_per_group;
    uint32_t index_in_group = (inode_num - 1) % inodes_per_group;

    // Ler o descritor do grupo
    ext2_group_desc gd;
    if (read_group_desc(group, &gd) != 0) return -1;

    // Bloco onde está a tabela de inodes
    uint32_t inode_table_block = gd.bg_inode_table;

    // Offset em bytes dentro da tabela de inodes
    uint32_t offset_bytes = index_in_group * inode_size;

    // Quantos inodes cabem por bloco?
    uint32_t inodes_per_block = block_size / inode_size;

    // Qual bloco dentro da tabela de inodes contém o inode?
    uint32_t block_offset = offset_bytes / block_size;
    uint32_t block_number = inode_table_block + block_offset;

    // Offset dentro do bloco
    uint32_t offset_in_block = offset_bytes % block_size;

    // Ler o bloco onde está o inode
    uint8_t buf[1024];
    if (read_block(block_number, buf) != 0) {
        fprintf(stderr, "Erro ao ler bloco do inode\n");
        return -1;
    }

    // Copiar os bytes para a estrutura de saída
    memcpy(inode_out, buf + offset_in_block, sizeof(ext2_inode));

    return 0;
}

int remove_dir_entry(unsigned int parent_inode_num, const char *name_to_remove) {
      ext2_inode parent_inode;
    get_inode(parent_inode_num, &parent_inode);
    char block_buf[block_size];
    
    for (int i = 0; i < 12 && parent_inode.i_block[i] != 0; i++) {
        read_block(parent_inode.i_block[i], block_buf);
          ext2_dir_entry_2 *entry = (  ext2_dir_entry_2 *)block_buf;
          ext2_dir_entry_2 *prev_entry = NULL;
        unsigned int offset = 0;
        
        while (offset < block_size && entry->rec_len > 0) {
            if (entry->inode != 0 && strncmp(name_to_remove, entry->name, entry->name_len) == 0 && strlen(name_to_remove) == entry->name_len) {
                if (prev_entry) {
                    prev_entry->rec_len += entry->rec_len;
                } else {
                    entry->inode = 0; // Invalida a entrada se for a primeira
                }
                write_block(parent_inode.i_block[i], block_buf);
                return 0; // Sucesso
            }
            prev_entry = entry;
            offset += entry->rec_len;
            entry = (  ext2_dir_entry_2 *)((char *)block_buf + offset);
        }
    }
    return -1; // Não encontrado
}