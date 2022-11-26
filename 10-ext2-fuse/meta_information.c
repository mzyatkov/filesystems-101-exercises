#include "meta_information.h"

int read_super_block(int img, struct ext2_super_block *super_block)
{
    off_t offset = lseek(img, SUPERBLOCK_OFFSET, SEEK_SET);
    if (offset == -1)
    {
        return -errno;
    }
    ssize_t err = read(img, super_block, sizeof(struct ext2_super_block));
    if (err == -1)
    {
        return -errno;
    }
    return 0;
}

//  This would be the third block on a 1KiB block file system, or the second block for 2KiB and larger block file systems.
int read_group_desc(int img, int block_size, int inode_block_group, struct ext2_group_desc *group_desc)
{
    int flag;
    if (block_size > 1024)
    {
        flag = 1;
    }
    else
    {
        flag = 2;
    }
    off_t offset = flag * block_size + inode_block_group * sizeof(struct ext2_group_desc);
    offset = lseek(img, offset, SEEK_SET);
    read(img, group_desc, sizeof(struct ext2_group_desc));
    return 0;
}

int read_inode(int img, int block_size, int inode_size, int inode_offset_within_block_group, struct ext2_group_desc *group_desc, struct ext2_inode *inode)
{
    off_t offset = group_desc->bg_inode_table * block_size + inode_offset_within_block_group * inode_size;
    offset = lseek(img, offset, SEEK_SET);
    read(img, inode, sizeof(struct ext2_inode));
    return 0;
}
int read_block(int img, int block_size, int block_id, char *buffer)
{
    off_t offset = block_id * block_size;
    offset = lseek(img, offset, SEEK_SET);
    read(img, buffer, block_size);
    return 0;
}

int get_inode_struct_by_nr(int img, int inode_nr, struct ext2_inode *inode, struct ext2_super_block *super_block)
{
    // Legacy code of copy by inode_nr
    int block_size = 1024 << super_block->s_log_block_size;

    int inode_block_group = (inode_nr - 1) / super_block->s_inodes_per_group;
    int inode_offset_within_block_group = (inode_nr - 1) % super_block->s_inodes_per_group;

    struct ext2_group_desc group_desc;
    read_group_desc(img, block_size, inode_block_group, &group_desc);

    read_inode(img, block_size, super_block->s_inode_size, inode_offset_within_block_group, &group_desc, inode);
    return 0;
}

int get_stbuf_by_inode(ino_t inode_nr, int block_size, struct ext2_inode *inode, struct stat *stbuf)
{
    stbuf->st_ino = inode_nr;
    stbuf->st_mode = inode->i_mode;
    stbuf->st_nlink = inode->i_links_count;
    stbuf->st_uid = inode->i_uid;
    stbuf->st_gid = inode->i_gid;
    stbuf->st_size = inode->i_size;
    stbuf->st_blksize = block_size;
    stbuf->st_blocks = inode->i_blocks;
    stbuf->st_atime = inode->i_atime;
    stbuf->st_mtime = inode->i_mtime;
    stbuf->st_ctime = inode->i_ctime;
    return 0;
}