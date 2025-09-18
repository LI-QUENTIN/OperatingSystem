#include "syscall.h"
#include "fs.h"
#include "fat32.h"

extern struct task_struct *current;

struct task_struct *get_current_task() {
    return current;
}

void sys_getpid(struct pt_regs *regs) {
    regs->x[10] = current->pid;
}

// unsigned int sys_write(unsigned int fd, const char* buf, size_t count) {
//     if (fd == 1) {  // 如果是标准输出 (stdout)
//         for (size_t i = 0; i < count; i++) {
//             printk("%c", buf[i]);
//         }
//     }
//     // 返回写入的字节数
//     return count;
// }

int64_t sys_write(uint64_t fd, const char *buf, uint64_t len) {
    int64_t ret;
    struct file *file = &(current->files->fd_array[fd]);
    if (file->opened == 0) {
        printk("file not opened\n");
        return ERROR_FILE_NOT_OPEN;
    } else {
        // 检查文件是否允许写入
        if (file->perms & FILE_WRITABLE) {
            if (file->write != NULL) {
                // 执行写入操作
                ret = file->write(file, buf, len);
                if (ret < 0) {
                    printk("file write error\n");
                }
            } else {
                return -1;  // 文件没有写入函数
            }
        } else {
            return -1;  // 文件不可写
        }
    }
    return ret;
}

// void syscall(struct pt_regs* regs) {
//     if (regs->x[17] == SYS_WRITE) {
//         unsigned int fd = regs->x[10];
//         const char* buf = (const char*)regs->x[11];
//         size_t count = regs->x[12];
//         regs->x[10] = sys_write(fd, buf, count);  // 调用 sys_write
//     } else if (regs->x[17] == SYS_GETPID) {
//         sys_getpid(regs);  // 调用 sys_getpid
//     } 
    
//     regs->sepc += 4;  // 返回地址更新
// }

uint64_t sys_read(int64_t fd, char *buf, uint64_t len) {
    int64_t ret;
    struct file *file = &(current->files->fd_array[fd]);
    if (fd < 0 || file->opened == 0) {
        Err("File not opened\n");
        return ERROR_FILE_NOT_OPEN;
    }
    if (!((file -> perms) & FILE_READABLE)) {
        Err("File not readable\n");
    }
    ret = (file -> read)(file, buf, len);
    return ret;
}

int64_t sys_openat(int dfd, const char *filename, int flags) {
    int64_t fd = 0;
    // search for an empty fd
    for (int i = 0; i < MAX_FILE_NUMBER; i++) {
        if (current->files->fd_array[i].opened == 0) {
            fd = i;
            break;
        }
    }
    if (fd == 0) return -1; // no empty fd
    // check the path
    if (memcmp(filename, "/fat32/", 7) != 0) return -1; // only support fat32
    // open the file
    struct fat32_file new_file_struct = fat32_open_file(filename);
    if (new_file_struct.cluster == 0) return -1; // file not found
    // set file properties
    current->files->fd_array[fd].opened = 1;
    current->files->fd_array[fd].perms = flags;
    current->files->fd_array[fd].cfo = 0;
    current->files->fd_array[fd].fs_type = FS_TYPE_FAT32;
    current->files->fd_array[fd].fat32_file = new_file_struct;
    if (flags & FILE_WRITABLE) {
        current->files->fd_array[fd].write = fat32_write;
    }
    if (flags & FILE_READABLE) {
        current->files->fd_array[fd].read = fat32_read;
    }
    current->files->fd_array[fd].lseek = fat32_lseek;
    return fd;
}

int64_t sys_close(int64_t fd) {
    struct file *file = &(current->files->fd_array[fd]);
    if (file->opened == 0) {
        Err("File not opened\n");
        return -1;
    }
    file -> opened = 0;
    return 0;
}

int64_t sys_lseek(int64_t fd, uint64_t offset, uint64_t whence) {
    struct file *file = &(current->files->fd_array[fd]);
    if (fd < 0 || file->opened == 0) {
        Err("File not opened\n");
        return -1;
    }
    printk("sys_lseek: fd = %d, offset = %d, whence = %d\n", fd, offset, whence);
    return (file -> lseek)(file, offset, whence);
}