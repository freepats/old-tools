#ifndef __BITS_H__
#define __BITS_H__

#include <stdlib.h>

#define MY_LITTLE_ENDIAN 1234
#define MY_BIG_ENDIAN 4321

#if !defined(MY_BYTE_ORDER)
#if defined(__NetBSD__)
#if _BYTE_ORDER == _LITTLE_ENDIAN
#define MY_BYTE_ORDER MY_LITTLE_ENDIAN
#else /* _BIG_ENDIAN */
#define MY_BYTE_ORDER MY_BIG_ENDIAN
#endif
#endif
#endif

#if !defined(MY_BYTE_ORDER)
/* --- uncomment one of these, the 1st for i386, the 2nd for ppc :) */
#define MY_BYTE_ORDER MY_LITTLE_ENDIAN
/* #define MY_BYTE_ORDER MY_BIG_ENDIAN */
#endif

#define XCHG16(x) ((((x)&0xFF)<<8) | (((x)>>8)&0xFF))
#define XCHG32(x) ((((x)&0xFF)<<24) | (((x)&0xFF00)<<8) | \
        (((x)&0xFF0000)>>8) | (((x)>>24)&0xFF))

#if MY_BYTE_ORDER == MY_LITTLE_ENDIAN
#define HLE16(x) (x)
#define HLE32(x) (x)
#define HBE16(x) XCHG16(x)
#define HBE32(x) XCHG32(x)
#define LE16H(x) (x)
#define LE32H(x) (x)
#define BE16H(x) XCHG16(x)
#define BE32H(x) XCHG32(x)
#else /* _BIG_ENDIAN */
#define HBE16(x) (x)
#define HBE32(x) (x)
#define HLE16(x) XCHG16(x)
#define HLE32(x) XCHG32(x)
#define LE16H(x) XCHG16(x)
#define LE32H(x) XCHG32(x)
#define BE16H(x) (x)
#define BE32H(x) (x)
#endif

#define is_even(x) (((x) % 2) == 0)
#define BITS_PER_BYTE 8
#endif
