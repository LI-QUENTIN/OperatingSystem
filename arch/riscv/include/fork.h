#ifndef __FORK_H__
#define __FORK_H__

#include "stdint.h"
#include "syscall.h"
#include "defs.h"
#include "string.h"
#include "mm.h"

uint64_t do_fork(struct pt_regs *regs);

#endif






