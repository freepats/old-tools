#ifndef __RIFF_H__
#define __RIFF_H__

#include <stdio.h>
#include "rifftypes.h"

typedef enum {
  ON_DISK = 1,
  IN_RAM
} chunk_loc_t;

#define CHUNK_HEADER_SIZE (sizeof(myFOURCC) + sizeof(myDWORD))

typedef struct {
  myFOURCC ckID;
  myDWORD ckSize;
  long offset;
  int order;	/* order read in file, hierarchical not linear */
  int item;	/* order found in riff structure */
  chunk_loc_t location;
  void *parent;
  void *data;
  int endian;
  int count; /* count of "records" w/in "data" */

  void (*data_read_bytes)(void *, FILE *, char *, int); /* fpos change */
  void *(*data_read)(void *, FILE *); /* fpos change */
  void (*data_seek)(void *, FILE *); /* fpos change */
  void (*data_write)(void *, void *, int, FILE *); /* fpos change */
  void (*destroy)(void *);
  void (*end_seek)(void *, FILE *); /* fpos change */
  char *(*form_get)(void *, FILE *); /* fpos change */
  int (*form_check)(void *, char *, FILE *); /* fpos change */
  void (*header_read)(void *, FILE *);
  void (*header_write)(void *, FILE *); /* fpos change */
  int (*is_end)(void *, FILE *);
  int (*pad_write)(void *, FILE *); /* fpos change */
  int (*size)(void *);
  void (*start_seek)(void *, FILE *); /* fpos change */
  int (*write)(void *, FILE *); /* fpos change */
} chunk_t;

typedef void (*print_funcptr)(void *, chunk_t *, char *);

typedef struct {
  int level;
  char ckID[5];
  char *form;
  print_funcptr print;
  chunk_t *chunk;
} tree_item;

typedef char tree_container[5];

typedef struct {
  /* used to describe a RIFF format */
  tree_item *items;
  tree_container *containers;

  /* used to navigate a RIFF format */
  int (*ckID_find)(void *, char *, char *, int);
  void (*print)(void *, int, char *, char *, FILE *, FILE *);
  void (*read)(void *, chunk_t *, int, FILE *);
  void (*destroy)(void *);
  FILE *ifp;
  FILE *ofp;
  char *prefix;
  char *wt_prefix;
  void *parent;
} tree_t;

extern chunk_t *chunk_new(void *, chunk_loc_t);
extern tree_t *tree_new(tree_item *, tree_container *, void *);
#endif
