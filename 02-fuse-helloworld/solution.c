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
static int write_hellofs(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
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
		{return -ENOENT;}
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
	return -ENOENT;
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
	filler(buf, HELLO_PATH+1, NULL, 0, 0);

	return 0;
}

static const struct fuse_operations hellofs_ops = {
	.getattr = getattr_hellofs,
	.readdir = readdir_hellofs,
	.open = open_hellofs,
	.read = read_hellofs,
	.write = write_hellofs

};

int helloworld(const char *mntp)
{
	char *argv[] = {"exercise", "-f", (char *)mntp, NULL};
	return fuse_main(3, argv, &hellofs_ops, NULL);
}
