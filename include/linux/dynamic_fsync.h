/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_DYNAMIC_FSYNC_H
#define _LINUX_DYNAMIC_FSYNC_H

#include <linux/types.h>

struct file;

#ifdef CONFIG_DYNAMIC_FSYNC
bool dynamic_fsync_should_defer(struct file *file);
void dynamic_fsync_screen_event(bool on);
#else
static inline bool dynamic_fsync_should_defer(struct file *file)
{
	return false;
}
static inline void dynamic_fsync_screen_event(bool on)
{
}
#endif

#endif
