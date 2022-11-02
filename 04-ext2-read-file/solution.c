#include <solution.h>
#include <ext2fs/ext2fs.h>
#include <unistd.h>
#include <errno.h>

#define check(err) 

int read_super_block(int img, struct ext2_super_block *super_block) {
	off_t offset = lseek(img, SUPERBLOCK_OFFSET, SEEK_SET);
	if (offset == -1) {
		return -errno;
	}
	ssize_t err = read(img, super_block, sizeof(struct ext2_super_block));
	if (err == -1) {
		return -errno;
	}
	return 0;
}

//  This would be the third block on a 1KiB block file system, or the second block for 2KiB and larger block file systems. 
int read_group_desc(int img,int block_size, int inode_block_group, struct ext2_group_desc *group_desc) {
	int flag;
	if (block_size > 1024) {
		flag = 1;
	} else {
		flag = 2;
	}
	off_t offset = flag * block_size + inode_block_group * sizeof(struct ext2_group_desc);
	offset = lseek(img, offset, SEEK_SET);
	read(img, group_desc, sizeof(struct ext2_group_desc));
	return 0;
}

int read_inode(int img, int block_size, int inode_size, int inode_offset_within_block_group, struct ext2_group_desc *group_desc) {
	off_t offset = group_desc->bg_inode_table * block_size + inode_offset_within_block_group * inode_size;
	offset = lseek(img, offset, SEEK_SET);
	return 0;
}
int read_block(int img, int block_size, int block_id, char *buffer) {
	off_t offset = block_id * block_size;
	offset = lseek(img, offset, SEEK_SET);
	read(img, buffer, block_size);
	return 0;
}
int write_buffer(int out, char *buffer, int block_size,int *remain_to_write) {
	if (*remain_to_write < block_size) {
		write(out, buffer, *remain_to_write);
		*remain_to_write = 0;
	} else {
		write(out, buffer, block_size);
		*remain_to_write -= block_size;
	}
	return 0;
}
int copy_ndir_block(int img, int out, int block_size, int block_id, int *remain_to_write, char *buffer) {
	read_block(img, block_size, block_id, buffer);
	write_buffer(out, buffer, block_size, remain_to_write);	
	return 0;
}
int copy_indir_block(int img, int out, int block_size, int block_id, int *remain_to_write, char *buffer) {
	read_block(img, block_size, block_id, buffer);
	int *buffer_addr_iter = (int *)buffer;

	char *buffer_indir_block = malloc(block_size * sizeof(char));

	for (unsigned long i = 0; i < block_size / sizeof(int) && *remain_to_write > 0; i++)
	{
		if (buffer_addr_iter[i] == 0) {
			break;
		} else {
			copy_ndir_block(img, out, block_size, buffer_addr_iter[i], remain_to_write, buffer_indir_block);
		}
	}
	free(buffer_indir_block);
	return 0;
}
int copy_dndir_block(int img, int out, int block_size, int block_id, int *remain_to_write, char *buffer) {
	read_block(img, block_size, block_id, buffer);
	int *buffer_addr_iter = (int *)buffer;

	char *buffer_dndir_block = malloc(block_size * sizeof(char));

	for (unsigned long i = 0; i < block_size / sizeof(int) && *remain_to_write > 0 ; i++)
	{
		if (buffer_addr_iter[i] == 0) {
			break;
		} else {
			copy_indir_block(img, out, block_size, buffer_addr_iter[i], remain_to_write, buffer_dndir_block);
		}
	}
	free(buffer_dndir_block);
	return 0;
}

int copy_inode_content(int img, int out, int block_size, struct ext2_inode *inode){
	char *buffer = malloc(block_size * sizeof(char));
	int remain_to_write = inode->i_size;

	// i_block в индексном дескрипторе файла представляет собой массив из 15 адресов блоков. 
	for (int i = 0; i < EXT2_N_BLOCKS && remain_to_write > 0; i++) {
		if (inode->i_block[i] == 0){
			break; 
		}
		if (i < EXT2_NDIR_BLOCKS)
		{
			copy_ndir_block(img, out, block_size, inode->i_block[i], &remain_to_write, buffer); 
		}
		else if (i == EXT2_IND_BLOCK)
		{
			copy_indir_block(img, out, block_size, inode->i_block[i], &remain_to_write, buffer); 
		}
		else if (i == EXT2_DIND_BLOCK)
		{
			copy_dndir_block(img, out, block_size, inode->i_block[i], &remain_to_write, buffer); 
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





int dump_file(int img, int inode_nr, int out)
{
	struct ext2_super_block super_block;
	read_super_block(img, &super_block);

	int block_size = 1024 << super_block.s_log_block_size;
	int inode_block_group = (inode_nr - 1) / super_block.s_inodes_per_group;
	int inode_offset_within_block_group = (inode_nr - 1) % super_block.s_inodes_per_group;

	struct ext2_group_desc group_desc;
	read_group_desc(img, block_size, inode_block_group, &group_desc);

	struct ext2_inode inode;
	read_inode(img, block_size,super_block.s_inode_size, inode_offset_within_block_group, &group_desc);

	copy_inode_content(img, out, block_size, &inode);

	/* implement me */
	return 0;
}