#ifndef EXT2_DATA_H
#define EXT2_DATA_H

#include <stdint.h>

// --- Constantes ---
#define EXT2_SUPER_MAGIC 0xEF53
#define EXT2_ROOT_INO    2
#define EXT2_N_BLOCKS    15

// --- Tipos de arquivo (modo do inode) ---
#define EXT2_S_IFREG 0x8000  // Arquivo regular
#define EXT2_S_IFDIR 0x4000  // Diretório

// --- Tipos de entrada de diretório ---
#define EXT2_FT_UNKNOWN   0
#define EXT2_FT_REG_FILE  1
#define EXT2_FT_DIR       2

// --- Estrutura do Superbloco ---
typedef struct {
    uint32_t s_inodes_count;        // Número total de inodes no sistema de arquivos
    uint32_t s_blocks_count;        // Número total de blocos no sistema de arquivos
    uint32_t s_r_blocks_count;      // Número de blocos reservados (para root)
    uint32_t s_free_blocks_count;   // Número de blocos livres
    uint32_t s_free_inodes_count;   // Número de inodes livres
    uint32_t s_first_data_block;    // Número do primeiro bloco de dados (geralmente 1)
    uint32_t s_log_block_size;      // Log2(tamanho do bloco) - 10 (0 = 1KB, 1 = 2KB, etc.)
    uint32_t s_log_frag_size;       // Log2(tamanho do fragmento) - 10 (geralmente igual ao bloco)
    uint32_t s_blocks_per_group;    // Número de blocos por grupo de blocos
    uint32_t s_frags_per_group;     // Número de fragmentos por grupo de blocos
    uint32_t s_inodes_per_group;    // Número de inodes por grupo de blocos
    uint32_t s_mtime;               // Hora da última montagem (timestamp Unix)
    uint32_t s_wtime;               // Hora da última escrita (timestamp Unix)
    uint16_t s_mnt_count;           // Contador de montagens desde a última verificação
    uint16_t s_max_mnt_count;       // Número máximo de montagens antes da verificação
    uint16_t s_magic;               // Assinatura mágica (0xEF53)
    uint16_t s_state;               // Estado do sistema de arquivos (1 = limpo, 2 = com erros)
    uint16_t s_errors;              // Comportamento quando detecta erros
    uint16_t s_minor_rev_level;     // Nível de revisão menor
    uint32_t s_lastcheck;           // Hora da última verificação (timestamp Unix)
    uint32_t s_checkinterval;       // Intervalo máximo entre verificações (segundos)
    uint32_t s_creator_os;          // OS que criou o sistema de arquivos
    uint32_t s_rev_level;           // Nível de revisão
    uint16_t s_def_resuid;          // UID padrão para blocos reservados
    uint16_t s_def_resgid;          // GID padrão para blocos reservados

    // EXT2_DYNAMIC_REV specific
    uint32_t s_first_ino;           // Primeiro inode utilizável (geralmente 11)
    uint16_t s_inode_size;          // Tamanho do inode em bytes
    uint16_t s_block_group_nr;      // Número do grupo de blocos deste superbloco
    uint32_t s_feature_compat;      // Flags de características compatíveis
    uint32_t s_feature_incompat;    // Flags de características incompatíveis
    uint32_t s_feature_ro_compat;   // Flags de características somente leitura
    uint8_t  s_uuid[16];            // Identificador único do volume (UUID)
    char     s_volume_name[16];     // Nome do volume (label)
    char     s_last_mounted[64];    // Caminho onde foi montado pela última vez
    uint32_t s_algorithm_usage_bitmap; // Bitmap de algoritmos de compressão usados

    // Performance Hints
    uint8_t  s_prealloc_blocks;     // Número de blocos para pré-alocação
    uint8_t  s_prealloc_dir_blocks; // Número de blocos para pré-alocação em diretórios
    uint16_t s_padding;             // Alinhamento/padding

    // Journaling Support
    uint8_t  s_journal_uuid[16];    // UUID do journal
    uint32_t s_journal_inum;        // Número do inode do arquivo de journal
    uint32_t s_journal_dev;         // Dispositivo contendo o journal
    uint32_t s_last_orphan;         // Início da lista de inodes órfãos

    // Directory Indexing Support
    uint32_t s_hash_seed[4];        // Semente para algoritmos de hash de diretório
    uint8_t  s_def_hash_version;    // Versão padrão do algoritmo de hash
    uint8_t  s_reserved_char_pad;   // Padding
    uint16_t s_reserved_word_pad;   // Padding

    uint32_t s_default_mount_opts;  // Opções de montagem padrão
    uint32_t s_first_meta_bg;       // Primeiro grupo de blocos de metadados

    uint32_t s_reserved[190];       // Espaço reservado para futuro uso
} __attribute__((packed)) ext2_super_block;

// --- Estrutura do Descritor de Grupo ---
typedef struct {
    uint32_t bg_block_bitmap;       // Bloco contendo o bitmap de blocos livres
    uint32_t bg_inode_bitmap;       // Bloco contendo o bitmap de inodes livres
    uint32_t bg_inode_table;        // Bloco inicial da tabela de inodes
    uint16_t bg_free_blocks_count;  // Contador de blocos livres
    uint16_t bg_free_inodes_count;  // Contador de inodes livres
    uint16_t bg_used_dirs_count;    // Número de diretórios neste grupo
    uint16_t bg_pad;                // Padding para alinhamento
    uint32_t bg_reserved[3];        // Espaço reservado para futuro uso
} __attribute__((packed)) ext2_group_desc;

// --- Estrutura do Inode ---
typedef struct {
    uint16_t i_mode;        // Tipo do arquivo e permissões (ver EXT2_S_IF*)
    uint16_t i_uid;         // User ID do dono (baixos 16 bits)
    uint32_t i_size;        // Tamanho em bytes (para arquivos regulares)
    uint32_t i_atime;       // Hora do último acesso (timestamp Unix)
    uint32_t i_ctime;       // Hora da criação (timestamp Unix)
    uint32_t i_mtime;       // Hora da última modificação (timestamp Unix)
    uint32_t i_dtime;       // Hora da exclusão (timestamp Unix)
    uint16_t i_gid;         // Group ID do dono (baixos 16 bits)
    uint16_t i_links_count; // Contador de links (hard links)
    uint32_t i_blocks;      // Número de blocos de 512B alocados
    uint32_t i_flags;       // Flags do inode
    uint32_t i_osd1;        // Depende do sistema operacional (OS Dependent 1)
    uint32_t i_block[EXT2_N_BLOCKS]; // Ponteiros para blocos de dados/diretórios
    uint32_t i_generation;  // Número de geração (para NFS)
    uint32_t i_file_acl;    // Bloco contendo ACL (Extended Attributes)
    uint32_t i_dir_acl;     // ACL do diretório (tamanho alto para diretórios)
    uint32_t i_faddr;       // Endereço do fragmento
    uint8_t  i_osd2[12];   // Depende do sistema operacional (OS Dependent 2)
} __attribute__((packed)) ext2_inode;

// --- Estrutura de Entrada de Diretório ---
typedef struct {
    uint32_t inode;         // Número do inode associado a esta entrada
    uint16_t rec_len;       // Tamanho total desta entrada (incluindo padding)
    uint8_t  name_len;      // Comprimento do nome do arquivo (em bytes)
    uint8_t  file_type;     // Tipo do arquivo (ver EXT2_FT_*)
    char     name[];        // Nome do arquivo (string sem terminador nulo)
} __attribute__((packed)) ext2_dir_entry_2;

#endif // EXT2_DATA_H