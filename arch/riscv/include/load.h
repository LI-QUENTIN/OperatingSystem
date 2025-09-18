#ifndef __LOAD_H__
#define __LOAD_H__

#include "stdint.h"

void load_program(struct task_struct *task);
void load_binary(struct task_struct *task);
void load_program_map(struct task_struct *task);
void setup_task_status(struct task_struct *task);
void init_user_stack(struct task_struct *task);
void create_user_stack(struct task_struct *task);

#endif