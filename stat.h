#ifndef __STAT_H
#define __STAT_H

#include <lwip/apps/tftp_server.h>
#include "main.h"

void stat_vfs_init(main_task_state_t * main_task_state);

extern struct tftp_context const STAT_VFS;

#endif