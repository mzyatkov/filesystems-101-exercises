#include "read_file.h"

struct file_pointer {
	off_t already_written;
	off_t offset;
	off_t size;
	char *out_buffer;
};

static int write_buffer(char *buffer, int block_size, struct file_pointer *file_pointer)
{
    if (file_pointer->offset > 0)
    {
		if (file_pointer->offset < block_size) {
			size_t read_size = (file_pointer->size < block_size - file_pointer->offset) ? file_pointer->size : block_size - file_pointer->offset;
            memcpy(file_pointer->out_buffer + file_pointer->already_written, buffer + file_pointer->offset, read_size);
			file_pointer->offset = 0;
			file_pointer->size -= read_size;
			file_pointer->already_written += read_size;
		}
        else
        {
			file_pointer->offset -= block_size;
        }
    } else {
		size_t read_size = (file_pointer->size < block_size) ? file_pointer->size : block_size;
		memcpy(file_pointer->out_buffer + file_pointer->already_written, buffer, read_size);
		file_pointer->already_written += read_size;
		file_pointer->size -= read_size;
	}
	return 0;
}
static int copy_ndir_block(int img, int block_size, int block_id, char *buffer, struct file_pointer *file_pointer)
{
	read_block(img, block_size, block_id, buffer);
	write_buffer(buffer, block_size, file_pointer);
	return 0;
}
static int copy_indir_block(int img, int block_size, int block_id, char *buffer,struct file_pointer *file_pointer)
{
	read_block(img, block_size, block_id, buffer);
	int *buffer_addr_iter = (int *)buffer;

	char *buffer_indir_block = malloc(block_size * sizeof(char));

	for (unsigned long i = 0; i < block_size / sizeof(int) && file_pointer->already_written < file_pointer->size ; i++)
	{
		if (buffer_addr_iter[i] == 0)
		{
			break;
		}
		else
		{
			copy_ndir_block(img, block_size, buffer_addr_iter[i], buffer_indir_block, file_pointer); 
		}
	}
	free(buffer_indir_block);
	return 0;
}
static int copy_dndir_block(int img, int block_size, int block_id, char *buffer, struct file_pointer *file_pointer)
{
	read_block(img, block_size, block_id, buffer);
	int *buffer_addr_iter = (int *)buffer;

	char *buffer_dndir_block = malloc(block_size * sizeof(char));

	for (unsigned long i = 0; i < block_size / sizeof(int) && file_pointer->already_written < file_pointer->size ; i++)
	{
		if (buffer_addr_iter[i] == 0)
		{
			break;
		}
		else
		{
			copy_indir_block(img, block_size, buffer_addr_iter[i], buffer_dndir_block,  file_pointer);
		}
	}
	free(buffer_dndir_block);
	return 0;
}

int copy_inode_content(int img, char *out_buffer, int block_size, struct ext2_inode *inode, off_t offset, size_t size)
{
	if (inode->i_size < offset)
	{
		return 0;
	} else if (inode->i_size < offset + (off_t)size)
	{
		size = inode->i_size - offset;
	}
	char *buffer = malloc(block_size * sizeof(char));
	struct file_pointer *file_pointer = malloc(sizeof(struct file_pointer));
	file_pointer->already_written = 0;
	file_pointer->offset = offset;
	file_pointer->size = size;
	file_pointer->out_buffer = out_buffer;



	// i_block в индексном дескрипторе файла представляет собой массив из 15 адресов блоков.
	for (int i = 0; i < EXT2_N_BLOCKS && file_pointer->already_written < file_pointer->size; i++)
	{
		if (inode->i_block[i] == 0)
		{
			break;
		}
		if (i < EXT2_NDIR_BLOCKS)
		{
			copy_ndir_block(img, block_size, inode->i_block[i],buffer, file_pointer);
		}
		else if (i == EXT2_IND_BLOCK)
		{
			copy_indir_block(img, block_size, inode->i_block[i],buffer, file_pointer);
		}
		else if (i == EXT2_DIND_BLOCK)
		{
			copy_dndir_block(img, block_size, inode->i_block[i], buffer, file_pointer);
		}
		else
		{
			free(buffer);
			return -1; // TODO
		}
	}
	free(buffer);
	free(file_pointer);
	return size;
}


