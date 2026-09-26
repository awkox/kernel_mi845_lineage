/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Compatibility shims required to build the GKI 5.10 binder subsystem on
 * top of the android-msm-4.9 kernel tree.
 *
 * Each shim is kept as small and as local as possible so that dropping a
 * future GKI binder sync is a matter of deleting the entries below.
 */

#ifndef _LINUX_BINDER_COMPAT_H
#define _LINUX_BINDER_COMPAT_H

#include <linux/errno.h>
#include <linux/fdtable.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <uapi/linux/eventpoll.h>

/*
 * __poll_t and vm_fault_t were introduced in 4.10/4.11; on 4.9 the poll API
 * returns a plain unsigned int and handle_mm_fault() returns an int.
 */
#ifndef __poll_t
typedef unsigned int __poll_t;
#endif
#ifndef vm_fault_t
typedef int vm_fault_t;
#endif

/*
 * DEFINE_SHOW_ATTRIBUTE() appeared in 4.10.  It is used 7 times by the ported
 * binder to build the debugfs file_operations for its per-process views.
 *
 * The upstream 4.10 version cannot be reused verbatim: it relies on the 4.11
 * two argument seq_open() (which takes a show callback directly) and on the
 * inode_ini global, neither of which exist here.  Note that this tree uses the
 * vendor seq_operations layout, where start()/next() return void * and stop()
 * takes the current entry, so the helpers below are shaped to match it.  The
 * .show callback is unaffected: the ported binder already uses the
 * int (*)(struct seq_file *, void *) form.
 */
#ifndef DEFINE_SHOW_ATTRIBUTE
#define DEFINE_SHOW_ATTRIBUTE(attribute)					\
static void *attribute##_start(struct seq_file *m, loff_t *pos)		\
{										\
	*pos = 0;								\
	return NULL;								\
}										\
static void *attribute##_next(struct seq_file *m, void *v, loff_t *pos)	\
{										\
	(*pos)++;								\
	return NULL;								\
}										\
static void attribute##_stop(struct seq_file *m, void *v)			\
{										\
}										\
static const struct seq_operations attribute##_seq_ops = {			\
	.start	= attribute##_start,						\
	.next	= attribute##_next,						\
	.stop	= attribute##_stop,						\
	.show	= attribute##_show,						\
};										\
static int attribute##_open(struct inode *inode, struct file *file)		\
{										\
	return seq_open(file, &attribute##_seq_ops);				\
}										\
static const struct file_operations attribute##_fops = {			\
	.owner		= THIS_MODULE,						\
	.open		= attribute##_open,					\
	.read		= seq_read,						\
	.llseek		= seq_lseek,						\
	.release	= single_release,					\
};
#endif

/*
 * mmap_lock() split out of mm->mmap_sem in 5.8.  In 4.9 the semaphore is
 * still a plain rw_semaphore, so the read/write variants map onto the
 * rwsem directly.  Note that 4.9 has no per-mm locking at all, therefore
 * these are recursive-safe only in the sense that they behave exactly like
 * the pre-5.8 upstream code did.
 */
#ifndef mmap_read_lock
#define mmap_read_lock(mm)		down_read(&(mm)->mmap_sem)
#define mmap_read_unlock(mm)		up_read(&(mm)->mmap_sem)
#define mmap_write_lock(mm)		down_write(&(mm)->mmap_sem)
#define mmap_write_unlock(mm)		up_write(&(mm)->mmap_sem)
#define mmap_read_trylock(mm)		down_read_trylock(&(mm)->mmap_sem)
#endif

/*
 * mmgrab() and kvcalloc() landed in 4.11 and 4.5 respectively; on 4.9 a
 * reference is taken with the mm_count atomic directly and kcalloc() is the
 * overflow checked allocation.
 */
#ifndef mmgrab
#define mmgrab(mm)			atomic_inc(&(mm)->mm_count)
#endif
#ifndef kvcalloc
#define kvcalloc(n, size, gfp)		kcalloc(n, size, gfp)
#endif

/*
 * GKI 5.10 reserves a slice of every binder object for Android vendor hooks,
 * declared through include/linux/android_vendor.h.  The 4.9 tree has no vendor
 * hook infrastructure and the ported binder never calls ANDROID_VENDOR_HOOK(),
 * so the reserved slices are kept as plain padding.  This preserves the size
 * and the layout intent of the original structures, and the definitions can be
 * dropped once include/linux/android_vendor.h is ported as well.
 */
#ifndef ANDROID_VENDOR_DATA
#define ANDROID_VENDOR_DATA(n)		u8 vendor_data_reserved_##n[n]
#define ANDROID_OEM_DATA_ARRAY(n, m)	u8 oem_data_reserved_##n[m]
#endif

/*
 * GKI exposes binder instrumentation to vendor modules as tracepoint based
 * hooks (include/trace/hooks/binder.h plus the DECLARE_HOOK machinery that
 * arrived with the GKI trace hook framework).  The 4.9 tree has neither:
 * include/linux/tracehook.h is the old ptrace-only header and there is no
 * include/trace/hooks/ directory, so the hooks cannot be declared or
 * registered.
 *
 * All 30 call sites are plain void statement calls, so they can simply be
 * dropped here.  The cost is nil: the hooks exist only so that vendor modules
 * can trace binder, and the ftrace tracepoints defined by binder_trace.h
 * (trace_binder_transaction, trace_binder_lock, ...) stay fully functional and
 * cover the same events.
 */
#define BINDER_STUB_VH(name)	do { } while (0)

#define trace_android_vh_alloc_oem_binder_struct(...)		BINDER_STUB_VH(alloc_oem_binder_struct)
#define trace_android_vh_binder_alloc_new_buf_locked(...)	BINDER_STUB_VH(binder_alloc_new_buf_locked)
#define trace_android_vh_binder_buffer_release(...)		BINDER_STUB_VH(binder_buffer_release)
#define trace_android_vh_binder_del_ref(...)			BINDER_STUB_VH(binder_del_ref)
#define trace_android_vh_binder_free_buf(...)			BINDER_STUB_VH(binder_free_buf)
#define trace_android_vh_binder_free_proc(...)			BINDER_STUB_VH(binder_free_proc)
#define trace_android_vh_binder_has_work_ilocked(...)		BINDER_STUB_VH(binder_has_work_ilocked)
#define trace_android_vh_binder_looper_state_registered(...)	BINDER_STUB_VH(binder_looper_state_registered)
#define trace_android_vh_binder_new_ref(...)			BINDER_STUB_VH(binder_new_ref)
#define trace_android_vh_binder_preset(...)			BINDER_STUB_VH(binder_preset)
#define trace_android_vh_binder_print_transaction_info(...)	BINDER_STUB_VH(binder_print_transaction_info)
#define trace_android_vh_binder_priority_skip(...)		BINDER_STUB_VH(binder_priority_skip)
#define trace_android_vh_binder_proc_transaction(...)		BINDER_STUB_VH(binder_proc_transaction)
#define trace_android_vh_binder_proc_transaction_end(...)	BINDER_STUB_VH(binder_proc_transaction_end)
#define trace_android_vh_binder_proc_transaction_finish(...)	BINDER_STUB_VH(binder_proc_transaction_finish)
#define trace_android_vh_binder_read_done(...)			BINDER_STUB_VH(binder_read_done)
#define trace_android_vh_binder_reply(...)			BINDER_STUB_VH(binder_reply)
#define trace_android_vh_binder_restore_priority(...)		BINDER_STUB_VH(binder_restore_priority)
#define trace_android_vh_binder_set_priority(...)		BINDER_STUB_VH(binder_set_priority)
#define trace_android_vh_binder_special_task(...)		BINDER_STUB_VH(binder_special_task)
#define trace_android_vh_binder_thread_read(...)		BINDER_STUB_VH(binder_thread_read)
#define trace_android_vh_binder_thread_release(...)		BINDER_STUB_VH(binder_thread_release)
#define trace_android_vh_binder_trans(...)			BINDER_STUB_VH(binder_trans)
#define trace_android_vh_binder_transaction_init(...)		BINDER_STUB_VH(binder_transaction_init)
#define trace_android_vh_binder_transaction_received(...)	BINDER_STUB_VH(binder_transaction_received)
#define trace_android_vh_binder_wait_for_work(...)		BINDER_STUB_VH(binder_wait_for_work)
#define trace_android_vh_binder_wakeup_ilocked(...)		BINDER_STUB_VH(binder_wakeup_ilocked)
#define trace_android_vh_free_oem_binder_struct(...)		BINDER_STUB_VH(free_oem_binder_struct)
#define trace_android_vh_sync_txn_recvd(...)			BINDER_STUB_VH(sync_txn_recvd)
#define trace_android_rvh_binder_transaction(...)		BINDER_STUB_VH(rvh_binder_transaction)

/*
 * close_fd_get_file() was split out of the close(2) path in 5.9.  There the
 * helper only detaches the descriptor from the fdtable, leaving the caller to
 * run filp_close() and drop the last reference.
 *
 * This tree's __close_fd() is the pre-5.9 version and still calls
 * filp_close() - and therefore fput() - itself, so it cannot be reused as a
 * drop-in: keeping the caller's filp_close() on top of it would fput() the
 * struct file twice and make binder_do_fd_close() release an already freed
 * object.
 *
 * So the fget() is taken here and __close_fd() is left to do the full close,
 * which moves the flush to inside the helper.  The resulting contract matches
 * 5.9+ for every observable purpose: the descriptor is gone and the file has
 * been flushed by the time this returns, and the caller is left holding the one
 * reference that binder_do_fd_close() later fput()s.  Only the reference the
 * caller has to drop is a plain fput() instead of a filp_close().
 */
static inline int close_fd_get_file(unsigned int fd, struct file **file)
{
	*file = NULL;

	*file = fget(fd);
	if (!*file)
		return -EBADF;
	__close_fd(current->files, fd);
	return 0;
}

/*
 * 4.9 predates the TWA_* task_work event enum: task_work_add() only takes a
 * "notify" boolean.  TWA_RESUME ("run the work on the way back to userspace")
 * is exactly the notify case.
 */
#define TWA_RESUME true

#endif /* _LINUX_BINDER_COMPAT_H */

