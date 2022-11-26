#pragma once
#include <ext2fs/ext2fs.h>
#include <unistd.h>
#include <errno.h>
#include "read_file.h"
#include "meta_information.h"


int get_file_inode(int img, struct file_inode *file, int block_size, struct ext2_inode *dir_inode);

