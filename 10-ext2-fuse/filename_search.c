#include "filename_search.h"


static int process_buffer(struct file_inode *file, char *buffer, int block_size)
{
    struct ext2_dir_entry_2 *dir_entry = (struct ext2_dir_entry_2 *)buffer;
    int remain_to_write = block_size;

    while (remain_to_write && dir_entry->inode != 0)
    {
        if (strncmp(dir_entry->name, file->name, file->namelength) == 0 && dir_entry->name_len == file->namelength)
        {
            file->inode_nr = dir_entry->inode;
            break;
        }

        buffer += dir_entry->rec_len;
        remain_to_write -= dir_entry->rec_len;

        dir_entry = (struct ext2_dir_entry_2 *)buffer;
    }

    return 0;
}

static int search_ndir_block(int img, struct file_inode *file, int block_size, int block_id, int *remain_to_write, char *buffer)
{
    (void)remain_to_write;
    read_block(img, block_size, block_id, buffer);
	process_buffer(file, buffer, block_size);
	return 0;
}
static int search_indir_block(int img, struct file_inode *file, int block_size, int block_id, int *remain_to_write, char *buffer)
{
	read_block(img, block_size, block_id, buffer);
	int *buffer_addr_iter = (int *)buffer;

	char *buffer_indir_block = malloc(block_size * sizeof(char));

	for (unsigned long i = 0; i < block_size / sizeof(int) && *remain_to_write > 0; i++)
	{
		if (buffer_addr_iter[i] == 0)
		{
			break;
		}
		else
		{
			search_ndir_block(img, file, block_size, buffer_addr_iter[i], remain_to_write, buffer_indir_block);
		}
	}
	free(buffer_indir_block);
	return 0;
}
static int search_dndir_block(int img, struct file_inode *file, int block_size, int block_id, int *remain_to_write, char *buffer)
{
	read_block(img, block_size, block_id, buffer);
	int *buffer_addr_iter = (int *)buffer;

	char *buffer_dndir_block = malloc(block_size * sizeof(char));

	for (unsigned long i = 0; i < block_size / sizeof(int) && *remain_to_write > 0; i++)
	{
		if (buffer_addr_iter[i] == 0)
		{
			break;
		}
		else
		{
			search_indir_block(img, file, block_size, buffer_addr_iter[i], remain_to_write, buffer_dndir_block);
		}
	}
	free(buffer_dndir_block);
	return 0;
}

int get_file_inode(int img, struct file_inode *file, int block_size, struct ext2_inode *dir_inode)
{
	char *buffer = malloc(block_size * sizeof(char));
	int remain_to_write = dir_inode->i_size;

	// i_block в индексном дескрипторе файла представляет собой массив из 15 адресов блоков.
	for (int i = 0; i < EXT2_N_BLOCKS && remain_to_write > 0; i++)
	{
		if (dir_inode->i_block[i] == 0)
		{
			break;
		}
		if (i < EXT2_NDIR_BLOCKS)
		{
			search_ndir_block(img, file, block_size, dir_inode->i_block[i], &remain_to_write, buffer);
		}
		else if (i == EXT2_IND_BLOCK)
		{
			search_indir_block(img, file, block_size, dir_inode->i_block[i], &remain_to_write, buffer);
		}
		else if (i == EXT2_DIND_BLOCK)
		{
			search_dndir_block(img, file, block_size, dir_inode->i_block[i], &remain_to_write, buffer);
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
