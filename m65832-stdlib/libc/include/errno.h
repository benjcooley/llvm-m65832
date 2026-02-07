/* errno.h - Error number definitions */

#ifndef _ERRNO_H
#define _ERRNO_H

#ifdef __cplusplus
extern "C" {
#endif

extern int errno;

#define EDOM    33
#define ERANGE  34
#define EILSEQ  84
#define EINVAL  22
#define ENOMEM  12
#define ENOSYS  38

#ifdef __cplusplus
}
#endif

#endif /* _ERRNO_H */
