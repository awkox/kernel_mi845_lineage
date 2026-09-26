/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/* binderfs for Android userspace */

#ifndef _UAPI_LINUX_BINDERFS_H
#define _UAPI_LINUX_BINDERFS_H

#include <linux/android/binder.h>
#include <linux/types.h>
#include <linux/ioctl.h>

#define BINDERFS_MAX_NAME 255

struct binderfs_device {
	char name[BINDERFS_MAX_NAME + 1];
	__u32 major;
	__u32 minor;
};

#define BINDER_CTL_ADD _IOWR('b', 1, struct binderfs_device)

#endif /* _UAPI_LINUX_BINDERFS_H */
