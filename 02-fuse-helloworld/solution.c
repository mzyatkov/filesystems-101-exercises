#include <solution.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fuse.h>

static const char *HELLO_PATH = "/hello";

static int read_hellofs(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	(void)fi;
	struct fuse_context *fuse_context = fuse_get_context();
	pid_t pid = fuse_context->pid;
	if (strcmp(path, "/hello") == 0)
	{
		ssize_t len;
		char filecontent[sizeof("hello, \n") + 10];
		sprintf(filecontent, "hello, %d\n", pid);
		len = strlen(filecontent);
		if (offset >= len)
		{
			return 0;
		}

		if (offset + (ssize_t)size > len)
		{
			memcpy(buf, filecontent + offset, len - offset);
			return len - offset;
		}

		memcpy(buf, filecontent + offset, size);
		return size;
	}

	return -ENOENT;
}
static int write_hellofs(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	(void)path;
	(void)buf;
	(void)size;
	(void)offset;
	(void)fi;
	return -EROFS;
}

static int open_hellofs(const char *path, struct fuse_file_info *fi)
{
	(void)fi;
	if (strcmp(path, HELLO_PATH) != 0)
	{
		return -EROFS;
	}
	return 0;
}
static int getattr_hellofs(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
	(void)fi;
	// printf("\tAttributes of %s requested\n", path);

	memset(stbuf, 0, sizeof(struct stat));
	struct fuse_context *fuse_context = fuse_get_context();
	stbuf->st_uid = fuse_context->uid;
	stbuf->st_gid = fuse_context->gid;
	if (strcmp(path, "/") == 0)
	{
		stbuf->st_mode = S_IFDIR | 0775;
		stbuf->st_nlink = 2;
		return 0;
	}

	if (strcmp(path, "/hello") == 0)
	{
		stbuf->st_mode = S_IFREG | 0400;
		stbuf->st_nlink = 1;
		stbuf->st_size = 19;
		return 0;
	}
	return -EROFS;
}
static int readdir_hellofs(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags rf)
{
	(void)offset;
	(void)fi;
	(void)rf;
	(void)path;

	if (strcmp(path, "/") != 0)
	{
		return -ENOENT;
	}
	filler(buf, ".", NULL, 0, 0);
	filler(buf, "..", NULL, 0, 0);
	filler(buf, HELLO_PATH + 1, NULL, 0, 0);

	return 0;
}

static int access_hellofs(const char *path, int mode) {
	if (mode & W_OK)
        return -EROFS;
	fprintf(stderr, "access\n");
	return access(path, mode);
}
static int setxattr_hellofs(const char *path, const char *name, const char *value, size_t size, int flags)
{
	(void)path;
	(void)name;
	(void)value;
	(void)size;
	(void)flags;
	return -EROFS;
}
static int removexattr_hellofs
(const char *path, const char *name)
{
	(void)path;
  	(void)name;
  	return -EROFS;
}
static int mknod_hellofs(const char *path, mode_t mode, dev_t rdev)
{
  (void)path;
  (void)mode;
  (void)rdev;
  return -EROFS;
}
static int mkdir_hellofs(const char *path, mode_t mode)
{
  (void)path;
  (void)mode;
  return -EROFS;
}

static int unlink_hellofs(const char *path)
{
  (void)path;
  return -EROFS;
}

static int rmdir_hellofs(const char *path)
{
  (void)path;
  return -EROFS;
}

static int symlink_hellofs(const char *from, const char *to)
{
  (void)from;
  (void)to;
  return -EROFS;	
}

static int rename_hellofs(const char *from, const char *to, unsigned int flags)
{
	(void)flags;
  (void)from;
  (void)to;
  return -EROFS;
}

static int link_hellofs(const char *from, const char *to)
{
  (void)from;
  (void)to;
  return -EROFS;
}

static int chmod_hellofs(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	(void)fi;
  (void)path;
  (void)mode;
  return -EROFS;
    
}

static int chown_hellofs(const char *path, uid_t uid, gid_t gid,  struct fuse_file_info *fi)
{
	(void)fi;
  (void)path;
  (void)uid;
  (void)gid;
  return -EROFS;
}

static int truncate_hellofs(const char *path, off_t size, struct fuse_file_info *fi)
{
	(void)fi;
	(void)path;
  	(void)size;
  	return -EROFS;
}
static int callback_utimens (const char *path, const struct timespec tv[2],
			 struct fuse_file_info *fi)
{
	(void)path;
  	(void)tv;
	(void)fi;
  	return -EROFS;	
}
static int create_hellofs(const char *path , mode_t mode, struct fuse_file_info *fi){
	(void)path;
	(void)mode;
	(void)fi;
	return -EROFS;
}
static int write_buf_hellofs(const char *path, struct fuse_bufvec *buf, off_t off,
			  struct fuse_file_info *fi) {
				(void)path;
				(void)buf;
				(void)off;
				(void)fi;
				return -EROFS;
			  }

static const struct fuse_operations hellofs_ops = {
	.getattr = getattr_hellofs,
	.readdir = readdir_hellofs,
	.open = open_hellofs,
	.read = read_hellofs,
	.write = write_hellofs,
	.access = access_hellofs,
	.chmod = chmod_hellofs,
	.chown = chown_hellofs,
	.setxattr = setxattr_hellofs,
	.truncate = truncate_hellofs,
	.mknod = mknod_hellofs,
	.mkdir = mkdir_hellofs,
	.removexattr = removexattr_hellofs,
	.rename = rename_hellofs,
	.symlink = symlink_hellofs,
	.unlink = unlink_hellofs,
	.link = link_hellofs,
	.rmdir = rmdir_hellofs,
	.utimens = callback_utimens,
	.create = create_hellofs,
	.write_buf = write_buf_hellofs
};

int helloworld(const char *mntp)
{
	char *argv[] = {"exercise", "-f", (char *)mntp, NULL};
	return fuse_main(3, argv, &hellofs_ops, NULL);
}
