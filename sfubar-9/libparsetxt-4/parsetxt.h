#ifndef __PARSETXT_H__
#define __PARSETXT_H__
#include <stdio.h>
#define BUFLEN 256

typedef struct {
  int line_number;
  char *key;
  char *data;
  int used;
} txt_line_t;

typedef struct {
  int line_count;
  txt_line_t **lines;
  int l; /* number of last line found by find_key */
  void (*destroy)(void *);
  int (*find_key)(void *, char *);
  char *(*get_data)(void *, char *);
  void (*add_data)(void *, char *, char *, int);
  void (*add_line)(void *, char *, int, int);
  void (*read_lines)(void *, FILE *);
  void (*warn_unused)(void *);
} txt_t;

extern txt_t *txt_new(void);
#endif
