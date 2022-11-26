#include "read_file.h"

static int write_buffer(char *out_buffer, char *buffer, int block_size, off_t *already_written, off_t offset, size_t size)
{

    if (*already_written >= offset && *already_written < offset + (off_t)size)
    {
        if (*already_written + block_size < offset + (off_t)size)
        {
            memcpy(out_buffer + *already_written - offset, buffer, block_size);
            *already_written += block_size;
        }
        else
        {
            memcpy(out_buffer + *already_written - offset, buffer, offset + size - *already_written);
            *already_written = offset + size;
        }
    }
	return 0;
}
static int copy_ndir_block(int img, char *out_buffer, int block_size, int block_id, off_t *already_written, char *buffer, off_t offset, size_t size)
{
	read_block(img, block_size, block_id, buffer);
	write_buffer(out_buffer, buffer, block_size, already_written, offset, size);
	return 0;
}
static int copy_indir_block(int img, char *out_buffer, int block_size, int block_id, off_t *already_written, char *buffer, off_t offset, size_t size)
{
	read_block(img, block_size, block_id, buffer);
	int *buffer_addr_iter = (int *)buffer;

	char *buffer_indir_block = malloc(block_size * sizeof(char));

	for (unsigned long i = 0; i < block_size / sizeof(int) && *already_written < offset + (off_t)size; i++)
	{
		if (buffer_addr_iter[i] == 0)
		{
			break;
		}
		else
		{
			copy_ndir_block(img, out_buffer, block_size, buffer_addr_iter[i], already_written, buffer_indir_block, offset, size);
		}
	}
	free(buffer_indir_block);
	return 0;
}
static int copy_dndir_block(int img, char *out_buffer, int block_size, int block_id, off_t *already_written, char *buffer, off_t offset, size_t size)
{
	read_block(img, block_size, block_id, buffer);
	int *buffer_addr_iter = (int *)buffer;

	char *buffer_dndir_block = malloc(block_size * sizeof(char));

	for (unsigned long i = 0; i < block_size / sizeof(int) && *already_written < offset + (off_t)size ; i++)
	{
		if (buffer_addr_iter[i] == 0)
		{
			break;
		}
		else
		{
			copy_indir_block(img, out_buffer, block_size, buffer_addr_iter[i], already_written, buffer_dndir_block, offset, size);
		}
	}
	free(buffer_dndir_block);
	return 0;
}

int copy_inode_content(int img, char *out_buffer, int block_size, struct ext2_inode *inode, off_t offset, size_t size)
{
	char *buffer = malloc(block_size * sizeof(char));
	off_t already_written = 0;
    assert(inode->i_size >= offset + size);


	// i_block в индексном дескрипторе файла представляет собой массив из 15 адресов блоков.
	for (int i = 0; i < EXT2_N_BLOCKS && already_written < offset + (off_t)size; i++)
	{
		if (inode->i_block[i] == 0)
		{
			break;
		}
		if (i < EXT2_NDIR_BLOCKS)
		{
			copy_ndir_block(img, out_buffer, block_size, inode->i_block[i], &already_written, buffer, offset, size);
		}
		else if (i == EXT2_IND_BLOCK)
		{
			copy_indir_block(img, out_buffer, block_size, inode->i_block[i], &already_written, buffer, offset, size);
		}
		else if (i == EXT2_DIND_BLOCK)
		{
			copy_dndir_block(img, out_buffer, block_size, inode->i_block[i], &already_written, buffer, offset, size);
		}
		else
		{
			free(buffer);
			return -1; // TODO
		}
	}
	free(buffer);
	return 0;
}


