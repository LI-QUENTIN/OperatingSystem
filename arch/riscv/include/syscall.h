#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include "stdint.h"
#include "stddef.h"
#include "printk.h"
#include "proc.h"
#include "fork.h"

#define SYS_WRITE 64
#define SYS_GETPID 172
#define SYS_CLONE 220
#define SYS_READ 63
#define SYS_OPENAT  56
#define SYS_CLOSE   57
#define SYS_LSEEK   62

//void syscall(struct pt_regs* regs);
//unsigned int sys_write(unsigned int fd, const char* buf, size_t count);
int64_t sys_write(uint64_t fd, const char *buf, uint64_t len);
void sys_getpid(); 
int64_t sys_openat(int dfd, const char *filename, int flags); // dfd will not be used
int64_t sys_close(int64_t fd);
int64_t sys_lseek(int64_t fd, uint64_t offset, uint64_t whence);

#endif