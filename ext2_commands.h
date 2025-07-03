#ifndef _EXT2_COMMANDS_H_
#define _EXT2_COMMANDS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ext2_fs.h"
#include "ext2_lib.h"

extern ext2_super_block sb;
extern unsigned int block_size;

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
void cmd_print_superblock(void);
void cmd_print_groups(void);
void cmd_print_inode(uint32_t inode_num);

#endif