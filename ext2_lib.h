#ifndef _EXT2_LIB_H_
#define _EXT2_LIB_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "ext2_fs.h"
#include <stdbool.h>


// Variáveis globais
extern FILE *disk_image;
extern ext2_super_block sb;
extern ext2_group_desc *gd;
extern unsigned int block_size;
extern unsigned int inodes_per_block;
extern unsigned int group_count;

/*
function: Escreve um bloco de dados no disco.
param:
  - block_num: Número do bloco a ser escrito.
  - buffer: Ponteiro para os dados a serem escritos.
return: void (erros são tratados via perror).
*/
void write_block(unsigned int block_num, const void *buffer);

/*
function: Lê um bloco de dados do disco.
param:
  - block_num: Número do bloco a ser lido.
  - buffer: Ponteiro para armazenar os dados lidos.
return: 
  - 0 em caso de sucesso, -1 em caso de erro.
*/
int read_block(unsigned int block_num, void *buffer);

/*
function: Escreve o superbloco EXT2 no disco (offset fixo de 1024 bytes).
param: void (usa a variável global `sb`).
return: void.
*/
void write_superblock();


/*
function: Escreve o superbloco EXT2 no disco (offset fixo de 1024 bytes).
param: void (usa a variável global `sb`).
return: void.
*/
void write_superblock(); 


/*
function: Inicializa o acesso ao sistema de arquivos EXT2.
param:
  - image_path: Caminho para a imagem do disco.
return: 
  - 0 em sucesso, -1 em erro (ex: magic number inválido).
*/
int ext2_init(const char *image_path);

/*
function: Libera recursos e fecha a imagem do disco.
param: void.
return: void.
*/
void ext2_exit();

/*
function: Lê um inode do disco.
param:
  - inode_num: Número do inode (1-based).
  - inode_buf: Ponteiro para armazenar o inode lido.
return: 
  - 0 em sucesso, -1 se o inode for inválido.
*/
int get_inode(unsigned int inode_num, ext2_inode *inode_buf);

/*
function: Escreve um inode no disco.
param:
  - inode_num: Número do inode (1-based).
  - inode_buf: Ponteiro para os dados do inode.
return: void.
*/
void write_inode(unsigned int inode_num, const ext2_inode *inode_buf);

/*
function: Aloca um inode livre.
param: void.
return: 
  - Número do inode alocado (1-based) ou 0 se não houver espaço.
*/
unsigned int alloc_inode();

/*
function: Libera um inode (marca como livre no bitmap).
param:
  - inode_num: Número do inode (1-based).
return: void.
*/
void free_inode_resource(unsigned int inode_num);


/*
function: Aloca um bloco livre.
param: void.
return: 
  - Número do bloco alocado ou 0 se não houver espaço.
*/
unsigned int alloc_block();

/*
function: Libera um bloco (marca como livre no bitmap).
param:
  - block_num: Número do bloco.
return: void.
*/
void free_block_resource(unsigned int block_num);

/*
function: Busca um arquivo/diretório em um diretório.
param:
  - dir_inode_num: Inode do diretório pai.
  - name: Nome do arquivo/diretório a ser buscado.
return: 
  - Número do inode do arquivo/diretório encontrado ou 0 se não existir.
*/
unsigned int search_directory(unsigned int dir_inode_num, const char *name);


/*
function: Resolve um caminho (ex: "/home/user") para um inode.
param:
  - path: Caminho absoluto ou relativo.
  - start_inode_num: Inode inicial para caminhos relativos.
return: 
  - Número do inode correspondente ou 0 se não encontrado.
*/
unsigned int find_inode_by_path(const char *path, unsigned int start_inode_num);

/*
function: Lê o superbloco EXT2 do disco.
param:
  - sb: Ponteiro para armazenar o superbloco lido.
return: 
  - 0 em sucesso, -1 em erro (ex: magic inválido).
*/
int read_superblock(ext2_super_block *sb);

/*
function: Lê um descritor de grupo do disco.
param:
  - group_num: Número do grupo.
  - desc: Ponteiro para armazenar o descritor lido.
return: 
  - 0 em sucesso, -1 em erro.
*/
int read_group_desc(uint32_t group_num, ext2_group_desc *desc);

/*
function: Adiciona uma entrada a um diretório.
param:
  - parent_inode_num: Inode do diretório pai.
  - new_inode_num: Inode a ser vinculado à entrada.
  - name: Nome da nova entrada.
  - file_type: Tipo (arquivo, diretório, etc.).
return: 
  - 0 em sucesso, -1 em erro (ex: sem espaço).
*/
int add_dir_entry(unsigned int parent_inode_num, unsigned int new_inode_num, const char *name, uint8_t file_type);


/*
function: Lê um inode específico do sistema de arquivos EXT2.
param:
  - inode_num: Número do inode a ser lido (1-based).
  - inode_out: Ponteiro para a estrutura onde os dados do inode serão armazenados.
return:
  - 0 em caso de sucesso.
  - -1 em caso de erro (inode inválido, falha de leitura, etc.).
*/
int read_inode(uint32_t inode_num, ext2_inode *inode_out);

/*
function: Remove uma entrada de um diretório.
param:
  - parent_inode_num: Inode do diretório pai.
  - name_to_remove: Nome da entrada a ser removida.
return: 
  - 0 em sucesso, -1 se a entrada não for encontrada.
*/
int remove_dir_entry(unsigned int parent_inode_num, const char *name_to_remove);

/*
função: Copia dados de um bloco do sistema de arquivos para um arquivo externo.
parâmetros:
  - block_num: Número do bloco de origem para leitura.
  - dest_file: Ponteiro para o arquivo de destino.
  - bytes_remaining: Ponteiro para bytes restantes a copiar (atualizado após operação).
  - block_buf: Buffer temporário para dados do bloco.
retorno: void
observações:
  - Interrompe se não houver bytes restantes.
  - Lida automaticamente com gravações parciais de blocos.
  - Atualiza bytes_remaining durante a cópia.
*/
void copy_block_to_file(uint32_t block_num, FILE *dest_file,
                      unsigned int *bytes_remaining, char *block_buf);

/*
função: Libera recursivamente blocos indiretos e seus blocos referenciados.
parâmetros:
  - block_ptr: Bloco inicial a ser liberado.
  - level: Nível de indireção (1=simples, 2=duplo, etc.).
retorno: void
comportamento:
  - Nível 1: Libera blocos de dados diretamente.
  - Nível >1: Processa blocos de ponteiros recursivamente.
  - Sempre libera o block_ptr após processamento.
observações:
  - Ignora blocos inválidos (< s_first_data_block).
  - Aloca buffer temporário para leitura de blocos.
*/
void free_indirect_blocks(uint32_t block_ptr, int level);

/*
função: Libera todos os blocos associados a um inode.
parâmetros:
  - inode: Ponteiro para a estrutura inode contendo blocos.
retorno: void
etapas:
  1. Libera 12 blocos diretos (i_block[0-11])
  2. Processa bloco indireto simples (i_block[12])
  3. Processa bloco indireto duplo (i_block[13])
  4. (Opcional) Suporte para bloco indireto triplo (i_block[14])
observações:
  - Usa free_indirect_blocks() para blocos indiretos.
  - Ignora ponteiros de bloco nulos com segurança.
*/
void free_all_blocks(ext2_inode *inode);

bool is_directory_empty(unsigned int dir_inode_num);

#endif