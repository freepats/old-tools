#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <trap.h>
#include "bio.h"
#include "bits.h"
#include "riff.h"

#ifndef BUFLEN
#define BUFLEN 256
#endif

void assert_nonull(char *prefix, void *data)
{
  if (data == NULL)
    trap_exit("%s: null data!", prefix);
}

int chunk_size(void *vchunk)
{
  chunk_t *chunk = vchunk;
  int size = CHUNK_HEADER_SIZE + chunk->ckSize;
  if (!is_even(size))
    size++;
  return size;
}

void chunk_header_write(void *vchunk, FILE *fp)
{
  chunk_t *chunk = vchunk;
  myDWORD temp;
  if (chunk->endian == MY_LITTLE_ENDIAN)
    temp = HLE32(chunk->ckSize);
  else
    temp = HBE32(chunk->ckSize);
  chunk->offset = ftell(fp);
  bens_fwrite(chunk->ckID.str, 4, 1, fp);
  bens_fwrite(&temp, 4, 1, fp);
}

void chunk_data_write(void *vchunk, void *data, int length, FILE *fp)
{
  chunk_t *chunk = vchunk;
  assert_nonull("data_write: fp", fp);
  bens_fwrite(data, length, 1, fp);
  chunk->ckSize += length;
}

int chunk_pad_write(void *vchunk, FILE *fp)
{
  chunk_t *chunk = vchunk;
  if (!is_even(chunk->ckSize)) {
    bens_fwrite("\000", 1, 1, fp);
    return 1;
  }
  return 0;
}

int chunk_write(void *vchunk, FILE *fp)
{
  chunk_t *chunk = vchunk;
  int retval = 0;
  assert_nonull("chunk_write: chunk->data", chunk->data);
  assert_nonull("chunk_write: fp", fp);
  if (chunk->location == IN_RAM) {
    chunk->header_write(chunk, fp);
    bens_fwrite(chunk->data, chunk->ckSize, 1, fp);
    retval += chunk->pad_write(chunk, fp);
    retval += CHUNK_HEADER_SIZE + chunk->ckSize;
  }
  return retval;
}

void chunk_header_read(void *vchunk, FILE *fp)
{
  chunk_t *chunk = vchunk;
  myDWORD temp;
  assert_nonull("read: fp", fp);
  if (chunk->location == ON_DISK) {
    chunk->offset = ftell(fp);
    bens_fread(chunk->ckID.str, sizeof(myFOURCC), 1, fp);
    bens_fread(&temp, sizeof(myDWORD), 1, fp);
    if (chunk->endian == MY_LITTLE_ENDIAN)
      chunk->ckSize = LE32H(temp);
    else
      chunk->ckSize = BE32H(temp);
  }
}

void chunk_data_read_bytes(void *vchunk, FILE *fp, char *dst, int count)
{
  chunk_t *chunk = vchunk;

  assert_nonull("data_read_bytes: fp", fp);
  if (count > chunk->ckSize) {
    trap_warn("data_read_bytes: read past end of chunk, truncating");
    count = chunk->ckSize;
  }
  if (chunk->location == ON_DISK) {
    chunk->data_seek(chunk, fp);
    bens_fread(dst, count, 1, fp);
  } else {
    memcpy(dst, chunk->data, count);
  }
}

void *chunk_data_read(void *vchunk, FILE *fp)
{
  chunk_t *chunk = vchunk;
  char pad_byte;

  assert_nonull("data_read: fp", fp);
  if (chunk->location == ON_DISK) {
    if (chunk->data == NULL) {
      chunk->data = malloc(chunk->ckSize);
      chunk->data_read_bytes(chunk, fp, chunk->data, chunk->ckSize);
      if (!is_even(chunk->ckSize))
        bens_fread(&pad_byte, 1, 1, fp);
    }
    chunk->location = IN_RAM;
  }
  return chunk->data;
}

void chunk_destroy(void *vchunk)
{
  chunk_t *chunk = vchunk;
  if (chunk->data != NULL)
    free(chunk->data);
  free(chunk);
}

void chunk_start_seek(void *vchunk, FILE *fp)
{
  chunk_t *chunk = vchunk;
  fseek(fp, chunk->offset, SEEK_SET);
}

void chunk_data_seek(void *vchunk, FILE *fp)
{
  chunk_t *chunk = vchunk;
  fseek(fp, chunk->offset + CHUNK_HEADER_SIZE, SEEK_SET);
}

void chunk_end_seek(void *vchunk, FILE *fp)
{
  chunk_t *chunk = vchunk;
  fseek(fp, chunk->offset + CHUNK_HEADER_SIZE + chunk->ckSize, SEEK_SET);
}

int chunk_is_end(void *vchunk, FILE *fp)
{
  chunk_t *chunk = vchunk;
  int retval = 0;
  long fpos_cur = ftell(fp);
  long fpos_end = chunk->offset + CHUNK_HEADER_SIZE + chunk->ckSize;
  if (fpos_cur >= fpos_end)
    retval = 1;
  return retval;
}

char *chunk_form_get(void *vchunk, FILE *fp)
{
  chunk_t *chunk = vchunk;
  static char form[5];
  memset(form, 0, sizeof(form));
  if (chunk->ckSize > 3)
    chunk->data_read_bytes(chunk, fp, form, 4);
  return form;
}

int chunk_form_check(void *vchunk, char *valid, FILE *fp)
{
  chunk_t *chunk = vchunk;
  char *form;
  int retval;

  if (valid == NULL) {
    /* if the valid form is NULL, then match anything */
    retval = 0;
  } else {
    form = chunk->form_get(chunk, fp);
    retval = strncmp(form, valid, 4);
  }
  return retval;
}

int chunk_count = 0;
chunk_t *chunk_new(void *vparent, chunk_loc_t location)
{
  chunk_t *retval;
  chunk_t *parent = vparent;

  retval = malloc(sizeof(chunk_t));
  retval->count = 0;
  retval->data = NULL;
  retval->ckSize = 0;
  retval->parent = parent;
  retval->location = location;
  retval->order = chunk_count++;
  retval->item = -1;
  retval->size = chunk_size;
  if (parent != NULL)
    retval->endian = parent->endian;
  else
    retval->endian = MY_LITTLE_ENDIAN;

  retval->data_read_bytes = chunk_data_read_bytes;
  retval->data_read = chunk_data_read;
  retval->data_seek = chunk_data_seek;
  retval->data_write = chunk_data_write;
  retval->destroy = chunk_destroy;
  retval->end_seek = chunk_end_seek;
  retval->form_check = chunk_form_check;
  retval->form_get = chunk_form_get;
  retval->header_read = chunk_header_read;
  retval->header_write = chunk_header_write;
  retval->is_end = chunk_is_end;
  retval->pad_write = chunk_pad_write;
  retval->start_seek = chunk_start_seek;

  retval->write = chunk_write;
  return retval;
}

int tree_ckID_find(void *vtree, char *ckID, char *form, int level)
{
  tree_t *tree = vtree;
  int i = -1;
  while (tree->items[++i].level != -1) {
    if (tree->items[i].chunk == NULL)
      continue;
    if (level != -1 && level != tree->items[i].level)
      continue;
    if (ckID != NULL && strncmp(ckID, tree->items[i].ckID, 4) != 0)
      continue;
    if (form != NULL && strncmp(form, tree->items[i].form, 4) != 0)
      continue;
    return i;
  }
  return -1;
}

int tree_chunk_find(void *vtree, chunk_t *chunk, int level, FILE *fp)
{ 
  tree_t *tree = vtree;
  int i;

  for (i = 0; tree->items[i].level != -1; i++) {
    if (tree->items[i].level != level)
      continue;
    if (strncmp(tree->items[i].ckID, chunk->ckID.str, 4) != 0)
      continue;
    if (chunk->form_check(chunk, tree->items[i].form, fp) != 0)
      continue;
    if (tree->items[i].chunk != NULL)
      return -1;
    chunk->item = i;
    tree->items[i].chunk = chunk;
    return 1;
  }
  return 0;
}

int tree_chunk_is_container(void *vtree, char *str)
{
  tree_t *tree = vtree;
  int retval = 0;
  int i;
  for (i = 0; tree->containers[i][0] != '\000'; i++) {
    if (strncmp(str, tree->containers[i], 4) == 0) {
      retval = 1;
      break;
    }
  }
  return retval;
}

void tree_read(void *vtree, chunk_t *chunk, int level, FILE *fp)
{ 
  tree_t *tree = vtree;
  chunk_t *sub_chunk;
  char *form;
  int found;
  long fpos;

  found = tree_chunk_find(tree, chunk, level, fp);
  if (found != 1) {
    fpos = ftell(fp);
    form = chunk->form_get(chunk, fp);
    trap_warn("%s, fpos %ld, ckID %c%c%c%c, form %c%c%c%c, level %d",
      found == 0 ? "Unknown chunk" : "Duplicate chunk", fpos,
      chunk->ckID.str[0], chunk->ckID.str[1],
      chunk->ckID.str[2], chunk->ckID.str[3],
      form[0], form[1], form[2], form[3],
      level);
    chunk->end_seek(chunk, fp);
    chunk->destroy(chunk);
  } else {
    if (tree_chunk_is_container(tree, chunk->ckID.str) != 0) {
      while (!chunk->is_end(chunk, fp)) {
        sub_chunk = chunk_new(chunk, ON_DISK);
        sub_chunk->header_read(sub_chunk, fp);
        tree->read(tree, sub_chunk, level + 1, fp);
      }
    }
    chunk->end_seek(chunk, fp);
  }
}

void tree_destroy(void *vtree)
{
  tree_t *tree = vtree;
  int i = 0;
  while (tree->items[i].level != -1) {
    if (tree->items[i].chunk != NULL) {
      tree->items[i].chunk->destroy(tree->items[i].chunk);
      tree->items[i].chunk = NULL;
    }
    i++;
  }
}

char *tree_ckID_normalize(char *ckID)
{
  static char retval[5];
  retval[0] = ckID[0];
  retval[1] = ckID[1];
  retval[2] = ckID[2];
  retval[3] = ckID[3];
  retval[4] = '\000';
  if (retval[3] == ' ') {
    retval[3] = '\000';
  }
  return retval;
}

void tree_print_recurse(void *vtree, int cursor, char *prefix)
{
  tree_t *tree = vtree;
  int i;
  char *ckID_normal;
  char my_prefix[BUFLEN];
  chunk_t *chunk = tree->items[cursor].chunk;

  ckID_normal = tree_ckID_normalize(tree->items[cursor].ckID);
  if (prefix[0] == '\000') {
    strcpy(my_prefix, ckID_normal);
  } else {
    snprintf(my_prefix, BUFLEN, "%s.%s", prefix, ckID_normal);
  }

  if (tree->items[cursor].print != NULL)
    tree->items[cursor].print(tree, chunk, my_prefix);

  for (i = 0; tree->items[i].level != -1; i++) {
    if (tree->items[i].chunk != NULL) {
      if (tree->items[i].chunk->parent == chunk) {
        tree_print_recurse(tree, i, my_prefix);
      }
    }
  }
}

void tree_print(void *vtree, int cursor, char *prefix, char *wt_prefix,
  FILE *ifp, FILE *ofp)
{
  tree_t *tree = vtree;

  tree->ifp = ifp;
  tree->ofp = ofp;
  tree->prefix = prefix;
  tree->wt_prefix = wt_prefix;

  tree_print_recurse(tree, cursor, prefix);
}

tree_t *tree_new(tree_item *items, tree_container *containers, void *parent)
{
  tree_t *retval;

  retval = malloc(sizeof(tree_t));
  retval->prefix = NULL;
  retval->wt_prefix = NULL;
  retval->items = items;
  retval->containers = containers;
  retval->parent = parent;
  retval->ckID_find = tree_ckID_find;
  retval->print = tree_print;
  retval->read = tree_read;
  retval->destroy = tree_destroy;
  return retval;
}
