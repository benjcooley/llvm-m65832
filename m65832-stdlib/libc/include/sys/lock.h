#ifndef _SYS_LOCK_H
#define _SYS_LOCK_H

/* Minimal lock support for single-threaded M65832 */
typedef int __lock_t;
typedef __lock_t _LOCK_RECURSIVE_T;
typedef __lock_t _LOCK_T;

#define __LOCK_INIT(class, lock) static __lock_t lock = 0
#define __LOCK_INIT_RECURSIVE(class, lock) static __lock_t lock = 0
#define __lock_init(lock) (*(lock) = 0)
#define __lock_init_recursive(lock) (*(lock) = 0)
#define __lock_close(lock) ((void)0)
#define __lock_close_recursive(lock) ((void)0)
#define __lock_acquire(lock) ((void)0)
#define __lock_acquire_recursive(lock) ((void)0)
#define __lock_try_acquire(lock) 1
#define __lock_try_acquire_recursive(lock) 1
#define __lock_release(lock) ((void)0)
#define __lock_release_recursive(lock) ((void)0)

#endif /* _SYS_LOCK_H */
