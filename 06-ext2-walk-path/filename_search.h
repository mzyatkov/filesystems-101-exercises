#pragma once
#include <ext2fs/ext2fs.h>
#include <unistd.h>
#include <errno.h>
#include "solution_by_number.h"

struct file_inode {
	const char *name;
	int namelength;
	int inode_nr;
};

int get_file_inode(int img, struct file_inode *file, int block_size, struct ext2_inode *dir_inode);

