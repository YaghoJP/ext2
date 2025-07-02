#ifndef EXT2_FS_H
#define EXT2_FS_H

#include <stdint.h>

// --- Constantes Importantes ---
#define EXT2_SUPER_MAGIC 0xEF53 // Assinatura mágica para identificar o EXT2
#define EXT2_ROOT_INO 2         // Número do Inode para o diretório raiz

// --- Tipos de Arquivo (em i_mode) ---
#define EXT2_S_IFDIR  0x4000    // Diretório
#define EXT2_S_IFREG  0x8000    // Arquivo Regular

// --- Tipos de Arquivo (em directory entry) ---
#define EXT2_FT_UNKNOWN 0
#define EXT2_FT_REG_FILE 1
#define EXT2_FT_DIR 2

// --- Estrutura do Superbloco (localizado a 1024 bytes do início do volume) ---
struct ext2_super_block {
    uint32_t s_inodes_count;      // Total de inodes
    uint32_t s_blocks_count;      // Total de blocos
    uint32_t s_r_blocks_count;    // Blocos reservados (para o superusuário)
    uint32_t s_free_blocks_count; // Contagem de blocos livres
    uint32_t s_free_inodes_count; // Contagem de inodes livres
    uint32_t s_first_data_block;  // Primeiro bloco de dados (0 ou 1)
    uint32_t s_log_block_size;    // Tamanho do bloco = 1024 << s_log_block_size
    uint32_t s_log_frag_size;     // (Obsoleto) Tamanho do fragmento
    uint32_t s_blocks_per_group;  // Blocos por grupo
    uint32_t s_frags_per_group;   // (Obsoleto) Fragmentos por grupo
    uint32_t s_inodes_per_group;  // Inodes por grupo
    uint32_t s_mtime;             // Última montagem (timestamp)
    uint32_t s_wtime;             // Última escrita (timestamp)
    uint16_t s_mnt_count;         // Contagem de montagens
    uint16_t s_max_mnt_count;     // Contagem máxima de montagens
    uint16_t s_magic;             // Assinatura mágica (0xEF53)
};

// --- Estrutura do Descritor de Grupo de Blocos ---
struct ext2_group_desc {
    uint32_t bg_block_bitmap;      // Bloco do bitmap de blocos
    uint32_t bg_inode_bitmap;      // Bloco do bitmap de inodes
    uint32_t bg_inode_table;       // Bloco inicial da tabela de inodes
    uint16_t bg_free_blocks_count; // Contagem de blocos livres no grupo
    uint16_t bg_free_inodes_count; // Contagem de inodes livres no grupo
    uint16_t bg_used_dirs_count;   // Contagem de diretórios no grupo
    uint16_t bg_pad;
    uint32_t bg_reserved[3];
};

// --- Estrutura do Inode ---
#define EXT2_N_BLOCKS 15
struct ext2_inode {
    uint16_t i_mode;        // Modo do arquivo (tipo e permissões)
    uint16_t i_uid;         // ID do usuário
    uint32_t i_size;        // Tamanho do arquivo em bytes
    uint32_t i_atime;       // Último acesso
    uint32_t i_ctime;       // Criação
    uint32_t i_mtime;       // Modificação
    uint32_t i_dtime;       // Deleção
    uint16_t i_gid;         // ID do grupo
    uint16_t i_links_count; // Contagem de links (hard links)
    uint32_t i_blocks;      // Número de blocos de 512B (não do fs)
    uint32_t i_flags;       // Flags do arquivo
    uint32_t i_osd1;        // Específico do SO
    uint32_t i_block[EXT2_N_BLOCKS]; // Ponteiros para os blocos de dados
    uint32_t i_generation;  // (Para NFS)
    uint32_t i_file_acl;    // ACL do arquivo
    uint32_t i_dir_acl;     // ACL do diretório
    uint32_t i_faddr;       // Endereço do fragmento
    uint8_t  i_osd2[12];    // Específico do SO
};

// --- Estrutura da Entrada de Diretório ---
struct ext2_dir_entry_2 {
    uint32_t inode;         // Número do inode
    uint16_t rec_len;       // Tamanho desta entrada de diretório
    uint8_t  name_len;      // Tamanho do nome do arquivo
    uint8_t  file_type;     // Tipo do arquivo
    char     name[];        // Nome do arquivo (tamanho flexível)
};

#endif // EXT2_FS_H