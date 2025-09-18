#include "fs.h"
#include "vfs.h"
#include "mm.h"
#include "string.h"
#include "printk.h"
#include "fat32.h"

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = s1;
    const unsigned char *p2 = s2;

    while (n--) {
        if (*p1 != *p2) {
            return (*p1 - *p2);
        }
        p1++;
        p2++;
    }
    return 0;
}

size_t strlen(const char *s) {
    const char *p = s;
    while (*p) {
        p++;
    }
    return p - s;
}

// 初始化文件描述符
void init_file_descriptor(struct file *fd, int fd_index, int opened, int perms, 
                          int cfo, const char *path, 
                          void (*read_func)(), void (*write_func)()) {
    fd->opened = opened;
    fd->perms = perms;
    fd->cfo = cfo;
    fd->lseek = NULL;
    fd->read = read_func;
    fd->write = write_func;
    memcpy(fd->path, path, strlen(path) + 1);
}

// 初始化stdin文件描述符
void init_stdin(struct file *fd) {
    init_file_descriptor(fd, 0, 1, FILE_READABLE, 0, "stdin", stdin_read, NULL);
}

// 初始化stdout文件描述符
void init_stdout(struct file *fd) {
    init_file_descriptor(fd, 1, 1, FILE_WRITABLE, 0, "stdout", NULL, stdout_write);
}

// 初始化stderr文件描述符
void init_stderr(struct file *fd) {
    init_file_descriptor(fd, 2, 1, FILE_WRITABLE, 0, "stderr", NULL, stderr_write);
}

// 初始化其他未使用的文件描述符
void init_unused_files(struct file *fd) {
    init_file_descriptor(fd, -1, 0, 0, 0, "", NULL, NULL);
}

// 初始化文件结构体
struct files_struct *file_init() {
    struct files_struct *ret = (struct files_struct *)alloc_page();

    init_stdin(&ret->fd_array[0]);
    init_stdout(&ret->fd_array[1]);
    init_stderr(&ret->fd_array[2]);

    int i = 3;
    while (i < MAX_FILE_NUMBER) {
        init_unused_files(&ret->fd_array[i]);
        i++;
    }

    return ret;
}

uint32_t get_fs_type(const char *filename) {
    uint32_t ret;
    if (memcmp(filename, "/fat32/", 7) == 0) {
        ret = FS_TYPE_FAT32;
    } else if (memcmp(filename, "/ext2/", 6) == 0) {
        ret = FS_TYPE_EXT2;
    } else {
        ret = -1;
    }
    return ret;
}

int32_t file_open(struct file* file, const char* path, int flags) {
    file->opened = 1;
    file->perms = flags;
    file->cfo = 0;
    file->fs_type = get_fs_type(path);
    memcpy(file->path, path, strlen(path) + 1);

    if (file->fs_type == FS_TYPE_FAT32) {
        file->lseek = fat32_lseek;
        file->write = fat32_write;
        file->read = fat32_read;
        file->fat32_file = fat32_open_file(path);
        // todo: check if fat32_file is valid (i.e. successfully opened) and return
    } else if (file->fs_type == FS_TYPE_EXT2) {
        printk(RED "Unsupport ext2\n" CLEAR);
        return -1;
    } else {
        printk(RED "Unknown fs type: %s\n" CLEAR, path);
        return -1;
    }
}
