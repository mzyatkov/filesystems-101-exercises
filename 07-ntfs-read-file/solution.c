#include <solution.h>

#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#define _STRUCT_TIMESPEC 1
#include <stdlib.h>

#include <ntfs-3g/types.h>
#include <ntfs-3g/attrib.h>
#include <ntfs-3g/volume.h>
#include <ntfs-3g/dir.h>

int dump_file(int img, const char *path, int out)
{
	char path_to_fd[PATH_MAX];
	sprintf(path_to_fd, "/proc/self/fd/%d", img);

	char filename[NAME_MAX];
	int count = readlink(path_to_fd, filename, NAME_MAX);
	if (count < 0)
	{
		return count;
	}
	filename[count] = 0;

	ntfs_volume *volume = NULL;
	volume = ntfs_mount(filename, NTFS_MNT_RDONLY);
	if (!volume) {
		return -errno;
	}
	
	ntfs_inode *inode = NULL;
	inode = ntfs_pathname_to_inode(volume, NULL, path);
	if (!inode) {
		ntfs_umount(volume, FALSE);
		return -errno;
	}

	ntfs_attr *attr_struct = NULL;
	attr_struct = ntfs_attr_open(inode, AT_DATA, AT_UNNAMED, 0);
	if (!attr_struct) {
		ntfs_inode_close(inode);
		ntfs_umount(volume, FALSE);
		return -errno;
	}

	u32 block_size = 0;
	if (inode->mft_no < 2) {
		block_size = volume->mft_record_size;
	}

	s64 offset = 0;
	s64 write_size = 0;
	s64 read_size = 0;
	char buffer[PATH_MAX] = {};


	while (1)
	{
		if (block_size == 0)
		{
			read_size = ntfs_attr_pread(attr_struct, offset, PATH_MAX, buffer);
		}
		else
		{
			read_size = ntfs_attr_mst_pread(attr_struct, offset, 1, block_size, buffer);
			if (read_size > 0)
			{
				read_size = block_size * read_size;
			}
		}

		if (read_size < 0)
		{
			return -errno;
		}
		if (read_size == 0)
		{
			break;
		}
		write_size = write(out, buffer, read_size);
		if (write_size != read_size)
		{
			return -errno;
		}
		
		offset += read_size;
	}

	ntfs_attr_close(attr_struct);
	ntfs_inode_close(inode);
	ntfs_umount(volume, FALSE);

	/* implement me */
	return 0;
}
