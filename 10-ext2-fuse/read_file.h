#pragma once
#include "meta_information.h"
#include <ext2fs/ext2fs.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

/**
   Implement this function to copy the content of an inode @inode_nr
   to a file descriptor @out. A file at @inode_nr may be a sparse file.
   @img is a file descriptor of an open ext2 image.

   It suffices to support single- and double-indirect blocks.

   If a copy was successful, return 0. If an error occurred during
   a read or a write, return -errno.
*/
int copy_inode_content(int img, char *out, int block_size, struct ext2_inode *inode, off_t offset, size_t size);
