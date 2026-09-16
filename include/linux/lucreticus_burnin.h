/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_LUCRETICUS_BURNIN_H
#define _LINUX_LUCRETICUS_BURNIN_H

#include <linux/types.h>

#ifdef CONFIG_LUCRETICUS_BURNIN_PROTECTION
unsigned int lucreticus_burnin_limit(unsigned int level);
#else
static inline unsigned int lucreticus_burnin_limit(unsigned int level)
{
	return level;
}
#endif

#endif
