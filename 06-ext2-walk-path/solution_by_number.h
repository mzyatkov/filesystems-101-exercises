#pragma once
#include <ext2fs/ext2fs.h>
#include <unistd.h>
#include <errno.h>


int read_super_block(int img, struct ext2_super_block *super_block);

//  This would be the third block on a 1KiB block file system, or the second block for 2KiB and larger block file systems.
int read_group_desc(int img, int block_size, int inode_block_group, struct ext2_group_desc *group_desc);

int read_inode(int img, int block_size, int inode_size, int inode_offset_within_block_group, struct ext2_group_desc *group_desc, struct ext2_inode *inode);

int read_block(int img, int block_size, int block_id, char *buffer);

int copy_inode_content(int img, int out, int block_size, struct ext2_inode *inode);


