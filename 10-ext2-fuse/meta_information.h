#pragma once
#include <ext2fs/ext2fs.h>
#include <unistd.h>
#include <sys/stat.h>

struct file_inode {
	const char *name;
	int namelength;
	int inode_nr;
};


int get_file_inode(int img, struct file_inode *file, int block_size, struct ext2_inode *dir_inode);
int read_super_block(int img, struct ext2_super_block *super_block);
int get_inode_nr_by_path(int img, const char *path, struct ext2_super_block *super_block);
int get_inode_struct_by_nr(int img, int inode_nr, struct ext2_inode *inode, struct ext2_super_block *super_block);
int get_stbuf_by_inode(ino_t inode_nr, int block_size, struct ext2_inode *inode, struct stat *stbuf);
int read_block(int img, int block_size, int block_id, char *buffer);
int read_inode(int img, int block_size, int inode_size, int inode_offset_within_block_group, struct ext2_group_desc *group_desc, struct ext2_inode *inode);
int read_group_desc(int img, int block_size, int inode_block_group, struct ext2_group_desc *group_desc);
