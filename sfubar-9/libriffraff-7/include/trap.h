#ifndef __TRAP_H__
#define __TRAP_H__
#include <parsetxt.h>
#define TRAP_BUFLEN 1024
extern char trap_message[];
extern void (*trap_global)(int);
extern txt_t *trap_txt;
#define trap_warn(args...) do { \
  snprintf(trap_message, TRAP_BUFLEN, args); \
  trap_global(0); \
} while (0)
#define trap_exit(args...) do { \
  snprintf(trap_message, TRAP_BUFLEN, args); \
  trap_global(1); \
} while (0)
#endif
