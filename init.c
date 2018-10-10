#include <stdlib.h>
#include <time.h>
#include "config.h"
#include "stat.h"
#include "blink.h"
#include <tftp_vfs.h>
#include <ota.h>
#include <esp/uart.h>

#define TOSTRING(x) __STRINGIFY(x)

void user_init(void)
{
    uart_set_baud(0, 115200);

    setenv("TZ", TOSTRING(TIMEZONE), 1);
    tzset();

    blink_init();
    main_task_init();

    static struct tftp_context const * vfs[] = {
        &OTA_VFS,
        &STAT_VFS,
        NULL
    };
    tftp_vfs_init(vfs);
}