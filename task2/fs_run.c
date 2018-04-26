#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>
#include "superblock.h"
#include "inode.h"
#include "fs.h"

void run_shell(struct s_superblock* sb, int fd, struct s_inode* root)
{
    uint8_t i, len;
    char path[255] = "/";
    char cmd[100], arg[100];

    puts("MiniFS Shell. Supported commands:\n\
         \tls: display current directory content (with metadata)\n\
         \tmkdir <dir_name>: create new directory with name <dir_name> (in current directory)\n\
         \trm <iname>: remove directory/file with name <iname> (in current directory);\n\
         \t\t\tif directory, remove succeeds if (and only if) the directory was empty\
         \tcd <dir_name>: change current directory to <dir_name>\n\
         \tpull <source> <dest>: copy content from native FS <source> to <dest>\n\
         \tpush <source> <dest>: copy content from <source> to native FS <dest>\n\
         \texit: exit shell");

    for (;;)
    {
        printf("%s$ ", path);
        fgets(cmd, 100, stdin);
        len = strlen(cmd);
        if (strncmp(cmd, "exit", len - 1) == 0)
            break;

        for (i = 0; i < len && !isspace(cmd[i]); ++i);

        if (i == len && len > 3)
        {
            puts("unknown command");
            continue;
        }

        strncpy(arg, cmd + i + 1, len - i);
        len = strlen(cmd);
        cmd[i] = 0;
        arg[len - i - 2] = 0;

        if (strncmp(cmd, "ls", len) == 0)
        {
            puts("ls");
            fs_ls(sb, fd, root);
        }
        else if (strncmp(cmd, "mkdir", len) == 0)
        {
            puts("mkdir");
            fs_mkdir(sb, fd, root, arg);
        }
        else if (strncmp(cmd, "cd", len) == 0)
        {
            puts("cd");
            fs_cd(sb, fd, &root, arg);
        }
        else if (strncmp(cmd, "rm", len) == 0)
        {

        }
        else if (strncmp(cmd, "pull", len) == 0)
        {

        }
        else if (strncmp(cmd, "push", len) == 0)
        {

        }
        else
            puts("unknown command");
    }
}

int main()
{
    int fd = open("fs.img", O_RDWR);
    if (fd == -1)
    {
        perror("cannot open fs.img file");
        return 1;
    }

    struct s_superblock* sb;
    superblock_init(&sb, 0, 0, 0, 0, 0, 0, 0);
    superblock_read(sb, fd);

    struct s_inode* root;
    inode_init(&root, 0, 0, "", 0);
    inode_read(root, sb, fd, sb->root_block * sb->block_size);

    run_shell(sb, fd, root);

    superblock_del(sb);
    inode_del(root);
    close(fd);

    return 0;
}