#include "read_file.h"
#include "read_dir.h"
#include "filename_search.h"
#include "meta_information.h"
#include <solution.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fuse.h>

struct ext2_super_block glob_super_block = {};
int glob_block_size = -1;
int glob_img = -1;

static int write_ext2(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	(void)path;
	(void)buf;
	(void)size;
	(void)offset;
	(void)fi;
	return -EROFS;
}

static int open_ext2(const char *path, struct fuse_file_info *fi)
{
	int flags = fi->flags;
	
	if ((flags & O_WRONLY) || (flags & O_RDWR) || (flags & O_CREAT) || (flags & O_EXCL) || (flags & O_TRUNC) || (flags & O_APPEND)) {
      return -EROFS;
  	}
	int inode_nr = get_inode_nr_by_path(glob_img, path, &glob_super_block);
	if (inode_nr < 0) {
		return inode_nr;
	}
	return 0;
}

static int read_ext2(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	(void)fi;
	struct ext2_inode inode;
	int inode_nr = get_inode_nr_by_path(glob_img, path, &glob_super_block);
	if (inode_nr < 0) {
		return inode_nr;
	}
	get_inode_struct_by_nr(glob_img, inode_nr, &inode, &glob_super_block);
	int read_size = copy_inode_content(glob_img, buf, glob_block_size, &inode, offset, size);
	return read_size;

}
void report_file(int inode_nr, char type, const char *name, fuse_fill_dir_t filler, void *buf)
{
	(void)type;
	struct stat st;
	int inode_block_group = (inode_nr - 1) / glob_super_block.s_inodes_per_group;
	int inode_offset_within_block_group = (inode_nr - 1) % glob_super_block.s_inodes_per_group;

	struct ext2_group_desc group_desc;
	read_group_desc(glob_img, glob_block_size, inode_block_group, &group_desc);

	struct ext2_inode inode;
	read_inode(glob_img, glob_block_size, glob_super_block.s_inode_size, inode_offset_within_block_group, &group_desc, &inode);

	get_stbuf_by_inode(inode_nr, glob_block_size, &inode, &st);
	filler(buf, name, &st, 0, 0);
	return;
}


static int readdir_ext2(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags rf)
{
	(void)offset;
	(void)fi;
	(void)rf;
	(void)path;

	int inode_nr = get_inode_nr_by_path(glob_img, path, &glob_super_block);
	if (inode_nr  < 0) {
		return inode_nr;
	}
	dump_dir(glob_img, inode_nr, filler, buf);

	return 0;
}
static int getattr_ext2(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
	(void)fi;

	memset(stbuf, 0, sizeof(struct stat));
    
	int inode_nr = get_inode_nr_by_path(glob_img, path, &glob_super_block);
	if (inode_nr < 0) {
		return inode_nr;
	}

	int inode_block_group = (inode_nr - 1) / glob_super_block.s_inodes_per_group;
	int inode_offset_within_block_group = (inode_nr - 1) % glob_super_block.s_inodes_per_group;

	struct ext2_group_desc group_desc;
	read_group_desc(glob_img, glob_block_size, inode_block_group, &group_desc);

	struct ext2_inode inode;
	read_inode(glob_img, glob_block_size, glob_super_block.s_inode_size, inode_offset_within_block_group, &group_desc, &inode);

	get_stbuf_by_inode(inode_nr, glob_block_size, &inode, stbuf);
	return 0;
}

// static int access_ext2(const char *path, int mode) {
// 	if (mode & W_OK)
//         return -EROFS;
// 	fprintf(stderr, "access\n");
// 	return access(path, mode);
// }
// static int setxattr_ext2(const char *path, const char *name, const char *value, size_t size, int flags)
// {
// 	(void)path;
// 	(void)name;
// 	(void)value;
// 	(void)size;
// 	(void)flags;
// 	return -EROFS;
// }
// static int removexattr_ext2
// (const char *path, const char *name)
// {
// 	(void)path;
//   	(void)name;
//   	return -EROFS;
// }
// static int mknod_ext2(const char *path, mode_t mode, dev_t rdev)
// {
//   (void)path;
//   (void)mode;
//   (void)rdev;
//   return -EROFS;
// }
// static int mkdir_ext2(const char *path, mode_t mode)
// {
//   (void)path;
//   (void)mode;
//   return -EROFS;
// }

// static int unlink_ext2(const char *path)
// {
//   (void)path;
//   return -EROFS;
// }

// static int rmdir_ext2(const char *path)
// {
//   (void)path;
//   return -EROFS;
// }

// static int symlink_ext2(const char *from, const char *to)
// {
//   (void)from;
//   (void)to;
//   return -EROFS;	
// }

// static int rename_ext2(const char *from, const char *to, unsigned int flags)
// {
// 	(void)flags;
//   (void)from;
//   (void)to;
//   return -EROFS;
// }

// static int link_ext2(const char *from, const char *to)
// {
//   (void)from;
//   (void)to;
//   return -EROFS;
// }

// static int chmod_ext2(const char *path, mode_t mode, struct fuse_file_info *fi)
// {
// 	(void)fi;
//   (void)path;
//   (void)mode;
//   return -EROFS;
    
// }

// static int chown_ext2(const char *path, uid_t uid, gid_t gid,  struct fuse_file_info *fi)
// {
// 	(void)fi;
//   (void)path;
//   (void)uid;
//   (void)gid;
//   return -EROFS;
// }

// static int truncate_ext2(const char *path, off_t size, struct fuse_file_info *fi)
// {
// 	(void)fi;
// 	(void)path;
//   	(void)size;
//   	return -EROFS;
// }
// static int callback_utimens (const char *path, const struct timespec tv[2],
// 			 struct fuse_file_info *fi)
// {
// 	(void)path;
//   	(void)tv;
// 	(void)fi;
//   	return -EROFS;	
// }
// static int create_ext2(const char *path , mode_t mode, struct fuse_file_info *fi){
// 	(void)path;
// 	(void)mode;
// 	(void)fi;
// 	return -EROFS;
// }
// static int access_ext2(const char* path, int mode)
// {
// 	(void) path;

// 	if ((mode & W_OK) != 0)
// 		return -EROFS;

// 	return 0; 
// }

// static int write_buf_ext2(const char *path, struct fuse_bufvec *buf, off_t off, struct fuse_file_info *fi) {
// 				(void)path;
// 				(void)buf;
// 				(void)off;
// 				(void)fi;
// 				return -EROFS;
// }

static void *init_ext2(struct fuse_conn_info *conn, struct fuse_config *cfg) {
	(void)conn;
	(void)cfg;
	return NULL;
}

static const struct fuse_operations ext2_ops = {
	.open = open_ext2,
	.read = read_ext2,
	.write = write_ext2,
	.getattr = getattr_ext2,
	.readdir = readdir_ext2,
	// .opendir = opendir_ext2,
	// .access = access_ext2,
	// .chmod = chmod_ext2,
	// .chown = chown_ext2,
	// .setxattr = setxattr_ext2,
	// .truncate = truncate_ext2,
	// .mknod = mknod_ext2,
	// .mkdir = mkdir_ext2,
	// .removexattr = removexattr_ext2,
	// .rename = rename_ext2,
	// .symlink = symlink_ext2,
	// .unlink = unlink_ext2,
	// .link = link_ext2,
	// .rmdir = rmdir_ext2,
	// .utimens = callback_utimens,
	// .write_buf = write_buf_ext2,
	// .create = create_ext2,
	.init = init_ext2
};


int ext2fuse(int img, const char *mntp)
{
	(void)mntp;
	(void)ext2_ops;
	glob_img = img;
	read_super_block(img, &glob_super_block);
	glob_block_size = 1024 << glob_super_block.s_log_block_size;

	char *argv[] = {"exercise", "-d",  (char *)mntp, NULL};
	return fuse_main(3, argv, &ext2_ops, NULL);
	return 0;
}
