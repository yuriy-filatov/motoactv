/*
 * dumpext3fs.c --- dump the contents of an ext3 image into native filesystem.
 *
 * This file may be redistributed
 * under the terms of the GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "ext2fs/ext2_fs.h"
#include "ext2fs/ext2fs.h"

#include "et/com_err.h"

static ext2_filsys current_fs = NULL;

static void print_usage(void)
{
    fprintf(stderr, "Usage: dumpext3fs -i <image_filaname> -t <dump target path>\n");
}

static int open_filesystem(char *image_filename)
{
    int retval;
    int superblock = 0;
    int blocksize = 0;
    int open_flags = 0;
    io_channel data_io = 0;

    retval = ext2fs_open(image_filename,open_flags, superblock, blocksize,
                         unix_io_manager, &current_fs);
    if(retval){
        com_err(image_filename, retval, "while opening filesystem");
        current_fs = NULL;
        return 1;
    }

    return 0;
}

static void close_filesystem(void)
{
    int retval;
    retval = ext2fs_close(current_fs);
    if(retval) {
        com_err("ext2fs_close", retval, 0);
        current_fs = NULL;
    }
}

static void dump_file(ext2_ino_t ino, int fd, int preserve, char *outname)
{
    errcode_t retval;
    struct ext2_inode       inode;
    char            buf[8192];
    ext2_file_t     e2_file;
    int             nbytes;
    unsigned int    got;


    if (ext2fs_read_inode(current_fs, ino, &inode))
        return;

    retval = ext2fs_file_open(current_fs, ino, 0, &e2_file);
    if (retval) {
            com_err( "dump_file", retval, "while opening ext2 file");
            return;
    }

    while (1) {
        retval = ext2fs_file_read(e2_file, buf, sizeof(buf), &got);
        if (retval)
            com_err("dump_file", retval, "while reading ext2 file");
        if (got == 0)
            break;
        nbytes = write(fd, buf, got);
        if ((unsigned) nbytes != got)
            com_err("dump_file", errno, "while writing file");
    }

    retval = ext2fs_file_close(e2_file);
    if (retval) {
        com_err("dump_file", retval, "while closing ext2 file");
        return;
    }

    return;
}

static void dump_symlink(ext2_ino_t ino, struct ext2_inode *inode,const char *fullname)
{
    ext2_file_t e2_file;
    char *buf;
    errcode_t retval;

    buf = malloc(inode->i_size + 1);
    if (!buf) {
        com_err("dump_symlink", errno, "while allocating for symlink");
        free(buf);
    }

    /* Apparently, this is the right way to detect and handle fast
     * symlinks; see do_stat() in debugfs.c. */
    if (inode->i_blocks == 0)
        strcpy(buf, (char *) inode->i_block);
    else {
        unsigned bytes = inode->i_size;
        char *p = buf;
        retval = ext2fs_file_open(current_fs, ino, 0, &e2_file);
        if (retval) {
            com_err("dump_symlink", retval, "while opening symlink");
            free(buf);
        }
        for (;;) {
            unsigned int got;
            retval = ext2fs_file_read(e2_file, p, bytes, &got);
            if (retval) {
                com_err("dump_symlink", retval, "while reading symlink");
                free(buf);
            }

            bytes -= got;
            p += got;
            if (got == 0 || bytes == 0)
                break;
        }

        buf[inode->i_size] = 0;
        retval = ext2fs_file_close(e2_file);
        if (retval)
            free(buf);
            com_err("dump_symlink", retval, "while closing symlink");

    }

    if (symlink(buf, fullname) == -1) {
        free(buf);
        com_err("rdump", errno, "while creating symlink %s -> %s", buf, fullname);
    }

}

static int dump_dirent(struct ext2_dir_entry *dirent,
                       int offset EXT2FS_ATTR((unused)),
                       int blocksize EXT2FS_ATTR((unused)),
                       char *buf EXT2FS_ATTR((unused)), void *private);

static void dump_inode(ext2_ino_t ino, struct ext2_inode *inode,
                      const char *name, const char *dump_root)
{
    char *fullname;
    fullname = malloc(strlen(dump_root) + strlen(name) + 2);
    if(!fullname){
        com_err("dump_inode", errno, "while allocating memory");
        return ;
    }

    sprintf(fullname, "%s/%s",dump_root, name);

    if(LINUX_S_ISLNK(inode->i_mode)){
        dump_symlink(ino, inode, fullname);
    }
    else if(LINUX_S_ISREG(inode->i_mode)){
        int fd;
        fd = open(fullname, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
        if( fd == -1){
            com_err("dump_inode", errno, "while opening %s", fullname);
            free(fullname);
            return;
        }
        dump_file(ino, fd, 1, fullname);
        close(fd);
    }
    else if(LINUX_S_ISDIR(inode->i_mode) && strcmp(name, ".") && strcmp(name, "..")){
        errcode_t retval;

        /* Create the directory with 0700 permissions, because we
         * expect to have to create entries it.  Then fix its perms
         * once we've done the traversal. */

        if (mkdir(fullname, S_IRWXU) == -1) {
            com_err("dump_inode", errno, "while making directory %s", fullname);
            free(fullname);
            return;
        }

        retval = ext2fs_dir_iterate(current_fs, ino, 0, 0,dump_dirent, (void *) fullname);
        if (retval)
            com_err("dump_inode", retval, "while dumping %s", fullname);

    }
    return ;
}


static int dump_dirent(struct ext2_dir_entry *dirent,
                       int offset EXT2FS_ATTR((unused)),
                       int blocksize EXT2FS_ATTR((unused)),
                       char *buf EXT2FS_ATTR((unused)), void *private)
{
    char name[EXT2_NAME_LEN + 1];
    int thislen;
    int retval;
    const char * dump_root = private;
    struct ext2_inode inode;

    thislen = (dirent->name_len & 0xFF) < EXT2_NAME_LEN ?
                        (dirent->name_len & 0xFF) : EXT2_NAME_LEN;
    strncpy(name, dirent->name, thislen);
    name[thislen] = 0;

    retval = ext2fs_read_inode(current_fs, dirent->inode, &inode);
    if( retval ){
        com_err("dump_dirent", retval, "unable to read inode");
        return 1;
    }

    dump_inode(dirent->inode, &inode, name, dump_root);
    return 0;
}


int main(int argc, char **argv)
{
    char *image_filename = NULL;
    char *dump_target_path = NULL;

    int c = 0;
    int retval = 0;
    int i;

    struct stat st;

    ext2_ino_t root = EXT2_ROOT_INO;
    struct ext2_inode inode;


    if(argc == 1){
        print_usage();
        return 1;
    }

    while(( c = getopt(argc, argv, "i:t:")) != EOF) {
        switch(c) {
            case 'i':
                image_filename = optarg;
                break;

            case 't':
                dump_target_path = optarg;
                break;
            default:
                print_usage();
                return 1;
        }
    }

    if(dump_target_path == NULL) {
        fprintf(stderr, "No target path specified \n");
        return 1;
    }

    /* ensure that target path is a directory */
    i = stat(dump_target_path, &st);
    if( i == -1) {
        fprintf(stderr, "Error while stating target directory \n");
        return 1;
    }

    if(!S_ISDIR(st.st_mode)){
        fprintf(stderr, "Target path is not a directory \n");
        return 1;
    }

    retval = open_filesystem(image_filename);
    if(retval){
        fprintf(stderr, "Unable to open filesystem\n");
        return 1;
    }

    retval = ext2fs_dir_iterate(current_fs,root,0,0,dump_dirent,(void*)dump_target_path);
    if(retval) {
        com_err("dump", retval, "unable to read inode\n");
        return 1;
    }

    close_filesystem();
    return 0;
}
