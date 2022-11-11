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
	int err = readlink(path_to_fd, filename, NAME_MAX);
	if (err < 0)
	{
		return err;
	}

	ntfs_volume *volume;
	volume = ntfs_mount(filename, NTFS_MNT_RDONLY);

	ntfs_inode *inode;
	inode = ntfs_pathname_to_inode(volume, NULL, path);

	ntfs_attr *attr_struct;
	attr_struct = ntfs_attr_open(inode, AT_DATA, AT_UNNAMED, 0);

	u32 block_size = 0;
	s64 offset = 0;
	s64 write_size = 0;
	s64 read_size = 0;
	char buffer[PATH_MAX];

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
				read_size += block_size;
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
