#pragma once
#include "read_file.h"
#include "filename_search.h"
#include "meta_information.h"

/**
   Implement this function to copy the content of a file at @path
   to a file descriptor @out. @path has no symlinks inside it.
   @img is a file descriptor of an open ext2 image.

   It suffices to support single- and double-indirect blocks.

   If a copy was successful, return 0. If an error occurred during
   a read or a write, return -errno.

   Do take care to return -ENOENT, -ENOTDIR and other errors that
   may happen during a path traversal.
*/
int get_inode_nr_by_path(int img, const char *path, struct ext2_super_block *super_block);
// int read_file_bypath(int img, const char *path, int out);
