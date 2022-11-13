#include <solution.h>

#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#define _STRUCT_TIMESPEC 1
#include <stdlib.h>

#include <ntfs-3g/types.h>
#include <ntfs-3g/attrib.h>
#include <ntfs-3g/volume.h>
#include <ntfs-3g/dir.h>

// rewrite libntfs-3g function
ntfs_inode *ntfs_pathname_to_inode(ntfs_volume *vol, ntfs_inode *parent,
								   const char *pathname)
{
	u64 inum;
	int len, err = 0;
	char *p, *q;
	ntfs_inode *ni;
	ntfs_inode *result = NULL;
	ntfschar *unicode = NULL;
	char *ascii = NULL;

	if (!vol || !pathname)
	{
		errno = EINVAL;
		return NULL;
	}

	ntfs_log_trace("path: '%s'\n", pathname);

	ascii = strdup(pathname);
	if (!ascii)
	{
		ntfs_log_error("Out of memory.\n");
		err = ENOMEM;
		goto out;
	}

	p = ascii;
	/* Remove leading /'s. */
	while (p && *p && *p == PATH_SEP)
		p++;

	if (parent)
	{
		ni = parent;
	}
	else
	{

		ni = ntfs_inode_open(vol, FILE_root);
		if (!ni)
		{
			ntfs_log_debug("Couldn't open the inode of the root "
						   "directory.\n");
			err = EIO;
			result = (ntfs_inode *)NULL;
			goto out;
		}
	}

	while (p && *p)
	{
		/* Find the end of the first token. */
		q = strchr(p, PATH_SEP);
		if (q != NULL)
		{
			*q = '\0';
		}

		len = ntfs_mbstoucs(p, &unicode);
		if (len < 0)
		{
			ntfs_log_perror("Could not convert filename to Unicode:"
							" '%s'",
							p);
			err = errno;
			goto close;
		}
		else if (len > NTFS_MAX_NAME_LEN)
		{
			err = ENAMETOOLONG;
			goto close;
		}
		inum = ntfs_inode_lookup_by_name(ni, unicode, len);

		if (inum == (u64)-1)
		{
			ntfs_log_debug("Couldn't find name '%s' in pathname "
						   "'%s'.\n",
						   p, pathname);
			err = ENOENT;
			goto close;
		}

		if (ni != parent)
			if (ntfs_inode_close(ni))
			{
				err = errno;
				goto out;
			}

		inum = MREF(inum);
		ni = ntfs_inode_open(vol, inum);
		if (!ni)
		{
			ntfs_log_debug("Cannot open inode %llu: %s.\n",
						   (unsigned long long)inum, p);
			err = EIO;
			goto close;
		}

		// if ni is directory
		if (q)
		{
			if (!(ni->mrec->flags & MFT_RECORD_IS_DIRECTORY))
			{
				err = ENOTDIR;
				goto close;
			}
		}

		free(unicode);
		unicode = NULL;

		if (q)
			*q++ = PATH_SEP; /* JPA */
		p = q;
		while (p && *p && *p == PATH_SEP)
			p++;
	}

	result = ni;
	ni = NULL;
close:
	if (ni && (ni != parent))
		if (ntfs_inode_close(ni) && !err)
			err = errno;
out:
	free(ascii);
	free(unicode);
	if (err)
		errno = err;
	return result;
}

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
	if (!volume)
	{
		return -errno;
	}

	ntfs_inode *inode = NULL;
	inode = ntfs_pathname_to_inode(volume, NULL, path);

	if (!inode)
	{
		printf("%d\n", errno);
		int err = -errno;
		ntfs_umount(volume, FALSE);
		return err;
	}

	ntfs_attr *attr_struct = NULL;
	attr_struct = ntfs_attr_open(inode, AT_DATA, AT_UNNAMED, 0);
	if (!attr_struct)
	{
		int err = -errno;
		ntfs_inode_close(inode);
		ntfs_umount(volume, FALSE);
		return err;
	}

	u32 block_size = 0;
	if (inode->mft_no < 2)
	{
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
