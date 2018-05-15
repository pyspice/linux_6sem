#ifndef FS_H_INCLUDED
#define FS_H_INCLUDED

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "superblock.h"
#include "inode.h"
#include "utils.h"
#include "bitmap.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void fs_update_ancestors_size(struct s_superblock* sb, int fd, struct s_inode* node, uint32_t size)
{
    struct s_inode* tmp;
    inode_init(&tmp, 0, 0, "", 0);
    inode_copy(tmp, node);

    uint32_t offset;
    while (tmp->parent_inode != 0)
    {
        offset = get_block_offset(sb, tmp->parent_inode);
        inode_read(tmp, sb, fd, offset);
        tmp->size += size;
        inode_write(tmp, sb, fd, offset);
    }

    inode_del(tmp);
}

void fs_add_ninode(struct s_superblock* sb, int fd, struct s_inode* node, uint32_t nblock)
{
    if (node->iblock == 0 && node->nlast < 12)
    {
        node->blocks[node->nlast] = nblock;
        ++(node->nlast);
    }
    else
    {
        uint32_t* block = (uint32_t*)malloc(sb->block_size);
        uint32_t offset = get_block_offset(sb, node->iblock);
        pread(fd, block, sb->block_size, offset);

        if (node->iblock == 0)
        {
            uint32_t iblock = bitmap_get_available_block(sb, fd);
            bitmap_set_unavailable(sb, fd, iblock);

            node->iblock = iblock;
            node->nlast = 1;

            block[0] = nblock;
        }
        else
        {
            block[node->nlast] = nblock;
            ++(node->nlast);
        }

        pwrite(fd, block, sb->block_size, offset);
        free(block);
    }
}

void fs_mkdir(struct s_superblock* sb, int fd, struct s_inode* parent, const char* name)
{
    if ((sb->blocks_remain == 0) || (sb->blocks_remain == 1 && parent->blocks[11] != 0))
    {
        puts("mkdir: cannot create directory: no available space");
        return;
    }

    if (parent->iblock != 0 && parent->nlast == sb->block_size >> 5)
    {
        puts("mkdir: cannot create directory: max number of subdirectories/files reached");
        return;
    }

    struct s_inode* tmp;
    inode_init(&tmp, 0, 0, "", '\0');

    uint32_t i, len = (parent->iblock ? 12 : parent->nlast);
    int not_found = 1;
    for (i = 0; (i < len) && not_found; ++i)
    {
        inode_read(tmp, sb, fd, get_block_offset(sb, parent->blocks[i]));
        not_found = strncmp(tmp->name, name, 32);
    }

    if (parent->iblock && not_found)
    {
        uint32_t* block = (uint32_t*)malloc(sb->block_size);

        pread(fd, block, sb->block_size, get_block_offset(sb, parent->iblock));
        for (i = 0; (i < parent->nlast) && not_found; ++i)
        {
            inode_read(tmp, sb, fd, get_block_offset(sb, block[i]));
            not_found = strncmp(tmp->name, name, 32);
        }

        free(block);
    }

    inode_del(tmp);
    if (!not_found)
    {
        puts("mkdir: cannot create directory: directory exists");
        return;
    }

    uint32_t nblock = bitmap_get_available_block(sb, fd);
    bitmap_set_unavailable(sb, fd, nblock);

    struct s_inode* dir;
    inode_init(&dir, nblock, parent->ninode, name, 'd');

    inode_write(dir, sb, fd, get_block_offset(sb, nblock));

    if (parent->iblock == 0 && parent->nlast < 12)
    {
        parent->blocks[parent->nlast] = nblock;
        ++(parent->nlast);
    }
    else
    {
        uint32_t* block = (uint32_t*)malloc(sb->block_size);
        uint32_t offset = get_block_offset(sb, parent->iblock);
        pread(fd, block, sb->block_size, offset);

        if (parent->iblock == 0)
        {
            uint32_t iblock = bitmap_get_available_block(sb, fd);
            bitmap_set_unavailable(sb, fd, iblock);

            parent->iblock = iblock;
            parent->nlast = 1;

            block[0] = nblock;
        }
        else
        {
            block[parent->nlast] = nblock;
            ++(parent->nlast);
        }

        pwrite(fd, block, sb->block_size, offset);
        free(block);
    }

    inode_write(parent, sb, fd, get_block_offset(sb, parent->ninode));

    fs_update_ancestors_size(sb, fd, dir, dir->size);

    inode_del(dir);
}

void fs_ls(struct s_superblock* sb, int fd, struct s_inode* node)
{
    if (node->iblock == 0 && node->nlast == 0)
        return;

    struct s_inode* tmp;
    inode_init(&tmp, 0, 0, "", '\0');
    uint32_t i, len = (node->iblock ? 12 : node->nlast);

    for (i = 0; i < len; ++i)
    {
        inode_read(tmp, sb, fd, get_block_offset(sb, node->blocks[i]));

        printf("\t%c %s %10d %s\n", tmp->type, tmp->crtime, tmp->size, tmp->name);
    }

    if (node->iblock)
    {
        uint32_t* block = (uint32_t*)malloc(sb->block_size);

        pread(fd, block, sb->block_size, get_block_offset(sb, node->iblock));
        for (i = 0; i < node->nlast; ++i)
        {
            inode_read(tmp, sb, fd, get_block_offset(sb, block[i]));

            printf("\t%c %s %10d %s\n", tmp->type, tmp->crtime, tmp->size, tmp->name);
        }

        free(block);
    }

    inode_del(tmp);
}

int fs_cd(struct s_superblock* sb, int fd, struct s_inode* node, const char* name)
{
    int up = strncmp(name, "..", 2);
    if (up == 0 && node->parent_inode == 0)
        return 1;

    struct s_inode* tmp;
    inode_init(&tmp, 0, 0, "", '\0');

    int not_found = 1;
    if (up == 0)
    {
        inode_read(tmp, sb, fd, get_block_offset(sb, node->parent_inode));
        not_found = 0;
    }

    uint32_t i, len = (node->iblock ? 12 : node->nlast);
    for (i = 0; (i < len) && not_found; ++i)
    {
        inode_read(tmp, sb, fd, get_block_offset(sb, node->blocks[i]));
        not_found = strncmp(tmp->name, name, 32);
    }

    if (node->iblock && not_found)
    {
        uint32_t* block = (uint32_t*)malloc(sb->block_size);

        pread(fd, block, sb->block_size, get_block_offset(sb, node->iblock));
        for (i = 0; (i < node->nlast) && not_found; ++i)
        {
            inode_read(tmp, sb, fd, get_block_offset(sb, block[i]));
            not_found = strncmp(tmp->name, name, 32);
        }

        free(block);
    }

    if (not_found)
        printf("cd: %s: no such directory\n", name);
    else
        inode_copy(node, tmp);

    inode_del(tmp);

    return ~not_found;
}

void fs_erase_file(struct s_superblock* sb, int fd, struct s_inode* node)
{
    uint32_t i, len = (node->iblock ? 12 : node->nlast);
    for (i = 0; i < len; ++i)
        bitmap_set_available(sb, fd, node->blocks[i]);

    if (node->iblock)
    {
        uint32_t* block = (uint32_t*)malloc(sb->block_size);

        pread(fd, block, sb->block_size, get_block_offset(sb, node->iblock));
        for (i = 0; i < node->nlast; ++i)
            bitmap_set_available(sb, fd, block[i]);

        free(block);
    }
}

void fs_rm(struct s_superblock* sb, int fd, struct s_inode* node, const char* name)
{
    struct s_inode* tmp;
    inode_init(&tmp, 0, 0, "", '\0');

    int not_found = 1;
    char deleted = 0;
    uint32_t size, i, len = (node->iblock ? 12 : node->nlast);
    for (i = 0; (i < len) && not_found; ++i)
    {
        inode_read(tmp, sb, fd, get_block_offset(sb, node->blocks[i]));
        not_found = strncmp(tmp->name, name, 32);
    }

    if (not_found == 0)
    {
        if (tmp->type == 'd' && tmp->size > sb->inode_size)
        {
            puts("rm: cannot remove directory: directory is not empty");
            inode_del(tmp);
            return;
        }

        size = -(tmp->size);

        if (tmp->type == '-')
            fs_erase_file(sb, fd, tmp);

        bitmap_set_available(sb, fd, get_block_offset(sb, tmp->ninode));

        for (--i; i < len - 1; ++i)
            node->blocks[i] = node->blocks[i + 1];

        deleted = 1;
    }

    if (node->iblock)
    {
        uint32_t* block = (uint32_t*)malloc(sb->block_size);
        uint32_t offset = get_block_offset(sb, node->iblock);
        pread(fd, block, sb->block_size, offset);

        if (not_found)
        {
            for (i = 0; (i < node->nlast) && not_found; ++i)
            {
                inode_read(tmp, sb, fd, get_block_offset(sb, block[i]));
                not_found = strncmp(tmp->name, name, 32);
            }

            if (not_found == 0)
            {
                if (tmp->type == 'd' && tmp->size > sb->inode_size)
                {
                    puts("rm: cannot remove directory: directory is not empty");
                    inode_del(tmp);
                    free(block);
                    return;
                }

                size = -(tmp->size);

                if (tmp->type == '-')
                    fs_erase_file(sb, fd, tmp);

                bitmap_set_available(sb, fd, get_block_offset(sb, tmp->ninode));

                for (--i; i < len - 1; ++i)
                    block[i] = block[i + 1];
            }
        }
        else if (deleted)
        {
            node->blocks[11] = block[0];

            for (i = 0; i < node->nlast - 1; ++i)
                block[i] = block[i + 1];
        }

        pwrite(fd, block, sb->block_size, offset);

        free(block);
    }

    if (not_found)
        printf("rm: %s: no such directory\n", name);
    else
    {
        if (node->iblock && node->nlast == 1)
        {
            bitmap_set_available(sb, fd, node->iblock);
            node->iblock = 0;
            node->nlast = 12;
        }
        else
            --(node->nlast);

        fs_update_ancestors_size(sb, fd, tmp, size);
    }

    inode_del(tmp);
}

uint32_t fs_get_available_file_space(struct s_superblock* sb)
{
    uint32_t primary = sb->blocks_remain - 1;
    if (primary > 12)
        primary = 12;

    uint32_t secondary = sb->blocks_remain - primary - 2;
    if (secondary < 0)
        secondary = 0;
    if (secondary > sb->block_size / sizeof(uint32_t))
        secondary = sb->block_size / sizeof(uint32_t);

    return (primary + secondary) * sb->block_size;
}

int fs_pull(struct s_superblock* sb, int fd, struct s_inode* node, const char* from, const char* to)
{
    int ifd = open(from, O_RDONLY);
    if (ifd == -1)
    {
        fprintf(stderr, "pull: cannot open %s file", from);
        return 0;
    }

    struct stat st;
    fstat(ifd, &st);

    uint32_t space = fs_get_available_file_space(sb);
    if (st.st_size > space)
    {
        if (st.st_size > (sb->block_size / sizeof(uint32_t) + 12) * sb->block_size)
            perror("pull: cannot pull file: file too large");
        else
            perror("pull: cannot pull file: no available space");
        close(ifd);
        return 0;
    }

    uint32_t nblock = bitmap_get_available_block(sb, fd);
    bitmap_set_unavailable(sb, fd, nblock);
    --(sb->blocks_remain);

    struct s_inode* tmp;
    inode_init(&tmp, nblock, node->ninode, to, '-');

    char* block = (char*)malloc(sb->block_size);

    int bytes = read(ifd, block, sb->block_size);
    uint32_t i, nlast = (st.st_size > 12 * sb->block_size ? 12 : (st.st_size + sb->block_size - 1) / sb->block_size);
    for (i = 0; i < nlast && bytes > 0; ++i)
    {
        nblock = bitmap_get_available_block(sb, fd);
        bitmap_set_unavailable(sb, fd, nblock);

        pwrite(fd, block, sb->block_size, get_block_offset(sb, nblock));
        tmp->blocks[i] = nblock;

        bytes = read(ifd, block, sb->block_size);
    }

    if (nlast == 12 && bytes)
    {
        uint32_t* iblock = (uint32_t*)malloc(sb->block_size);

        nlast = (st.st_size - 11 * sb->block_size - 1) / sb->block_size;
        for (i = 0; i < nlast && bytes; ++i)
        {
            nblock = bitmap_get_available_block(sb, fd);
            bitmap_set_unavailable(sb, fd, nblock);

            pwrite(fd, block, sb->block_size, get_block_offset(sb, nblock));
            iblock[i] = nblock;

            bytes = read(ifd, block, sb->block_size);
        }

        nblock = bitmap_get_available_block(sb, fd);
        bitmap_set_unavailable(sb, fd, nblock);

        pwrite(fd, block, sb->block_size, get_block_offset(sb, nblock));

        tmp->iblock = nblock;

        free(iblock);
    }

    tmp->nlast = nlast;
    tmp->size += st.st_size;

    fs_update_ancestors_size(sb, fd, tmp, tmp->size);

    free(block);
    close(ifd);

    return 1;
}

int fs_push(struct s_superblock* sb, int fd, struct s_inode* node, const char* from, const char* to)
{

}

#endif // FS_H_INCLUDED
