#include "read_file_bypath.h"

int get_inode_nr_by_path(int img, const char *path, struct ext2_super_block *super_block)
{
    int cur_inode_nr = EXT2_ROOT_INO;
    int block_size = 1024 << super_block->s_log_block_size;
    int inode_nr = cur_inode_nr;
    const char *cur_path = path + 1;

    while (*cur_path)
    {
        struct ext2_inode cur_inode = {};
        get_inode_struct_by_nr(img, cur_inode_nr, &cur_inode, super_block);
        if ((cur_inode.i_mode & LINUX_S_IFDIR) == 0)
        {
            return -ENOTDIR;
        }

        const char *new_path = strchr(cur_path, '/');

        char cur_filename[EXT2_NAME_LEN] = {};
        int length = new_path - cur_path;
        if (new_path == NULL)
        {
            length = strlen(cur_path);
        }
        strncpy(cur_filename, cur_path, length);

        cur_filename[length] = '\0';
        struct file_inode file = {
            .inode_nr = 0,
            .name = cur_filename,
            .namelength = length};
        get_file_inode(img, &file, block_size, &cur_inode);
        cur_inode_nr = file.inode_nr;
        if (cur_inode_nr == 0)
        {
            return -ENOENT;
        }
        if (new_path == NULL)
        {
            inode_nr = cur_inode_nr;
            break;
        }

        cur_path = new_path + 1;
    }

    return inode_nr;
}
