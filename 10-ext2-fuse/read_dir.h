#pragma once
#include <ext2fs/ext2fs.h>
#include <fuse.h>
#include "meta_information.h"

/**
   Implement this function to parse the content of an inode @inode_nr
   as an ext2 directory. The function must call report_file() for each
   directory entry it finds.

   Directory entries are guaranteed to be regular files or directories.

   If a copy was successful, return 0. If an error occurred during
   a read or a write, return -errno.
*/
int dump_dir(int img, int inode_nr, fuse_fill_dir_t filler, void *filler_buf);

/**
   dump_dir() must call this function to report each directory entry.

   @inode_nr is the inode number of the child,
   @type is 'f' for regular files, and 'd' for directories,
   @name is the name (NULL-terminated) of the entry.
 */
void report_file(int inode_nr, char type, const char *name, fuse_fill_dir_t filler, void *filler_buf);
