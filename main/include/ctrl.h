#ifndef __CTRL_H
#define __CTRL_H

/**
 * Check the status of the lights controlling task and
 * starts it if it is not running yet.
 */
void start_lights_ctrl_task_if_not_running();

/**
 * Returns text describing current status of the lights controlling task.
 * 
 * @param buf buffer to copy status description into
 * @param buf_len size of the buffer
 * @return length of the status text
 */
size_t get_lights_ctlr_status(char * buf, size_t buf_len);

/**
 * Returns lights controlling task's hight watermark or -1 if the task is not running.
 */
int get_lights_ctrl_task_high_watermark();

#endif