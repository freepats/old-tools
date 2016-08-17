#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <trap.h>
#include "parsetxt.h"

void txt_destroy(void *vtxt)
{
  int i;
  txt_t *txt = vtxt;
  for (i = 0; i < txt->line_count; i++) {
    free(txt->lines[i]->key);
    free(txt->lines[i]->data);
    free(txt->lines[i]);
  }
  free(txt);
}

int txt_find_key(void *vtxt, char *key)
{
  int i;
  txt_t *txt = vtxt;

  for (i = 0; i < txt->line_count; i++) {
    if (strcmp(txt->lines[i]->key, key) == 0) {
      txt->l = txt->lines[i]->line_number;
      return i;
    }
  }
  return -1;
}

char *txt_get_data(void *vtxt, char *key)
{
  int n;
  txt_t *txt = vtxt;

  n = txt_find_key(txt, key);
  if (n == -1) {
    return NULL;
  } else {
    txt->lines[n]->used = 1;
    return txt->lines[n]->data;
  }
}

void txt_add_data(void *vtxt, char *key, char *data, int line_number)
{
  txt_t *txt = vtxt;

  txt->lines = realloc(txt->lines,
    (txt->line_count + 1) * sizeof(txt_line_t *));
  txt->lines[txt->line_count] = malloc(sizeof(txt_line_t));
  txt->lines[txt->line_count]->line_number = line_number;
  txt->lines[txt->line_count]->key = strdup(key);
  txt->lines[txt->line_count]->data = strdup(data);
  txt->lines[txt->line_count]->used = 0;
  txt->line_count++;
}

void txt_add_line(void *vtxt, char *line_data, int line_len, int line_number)
{
  char *p;
  int n;
  txt_t *txt = vtxt;

  /* skip empty lines and comments */
  if (line_len < 1 || line_data[0] == ';' || line_data[0] == '#')
    return;

  /* require nonempty keys and data */
  p = strchr(line_data, '=');
  if (p == NULL)
    trap_exit("line %d invalid", line_number);
  *p = '\000';
  p++;
  if (strcmp(line_data, "") == 0)
    trap_exit("line %d missing key", line_number);

  /* catch duplicate lines */
  n = txt_find_key(txt, line_data);
  if (n != -1)
    trap_exit("duplicate lines %d and %d",
      line_number, txt->lines[n]->line_number);

  txt_add_data(txt, line_data, p, line_number);
}

void txt_read_lines(void *vtxt, FILE *fp)
{
  char line_data[BUFLEN];
  int len;
  int line_number = 1;
  txt_t *txt = vtxt;

  fgets(line_data, BUFLEN, fp);
  line_data[BUFLEN - 1] = '\000';
  len = strlen(line_data);
  while (!feof(fp)) {
    if (len > 0) {
      if (line_data[len - 1] != '\n') {
        trap_warn("line %d lacks a newline, perhaps it is too long",
          line_number);
      } else {
        /* remove \n at end of line */
        len--;
        line_data[len] = '\000';
        /* deal with DOS newlines */
        if (len > 0 && line_data[len - 1] == '\r') {
          len--;
          line_data[len] = '\000';
        }
      }
    }
    txt_add_line(txt, line_data, len, line_number);
    fgets(line_data, BUFLEN, fp);
    line_data[BUFLEN - 1] = '\000';
    len = strlen(line_data);
    line_number++;
  }
}

void txt_warn_unused(void *vtxt)
{
  txt_t *txt = vtxt;
  int i;

  for (i = 0; i < txt->line_count; i++) {
    if (txt->lines[i]->used != 1) {
      txt->l = txt->lines[i]->line_number;
      trap_warn("unrecognized line");
    }
  }
}

txt_t *txt_new(void)
{
  txt_t *retval;

  retval = malloc(sizeof(txt_t));
  retval->line_count = 0;
  retval->lines = NULL;
  retval->destroy = txt_destroy;
  retval->find_key = txt_find_key;
  retval->get_data = txt_get_data;
  retval->add_data = txt_add_data;
  retval->add_line = txt_add_line;
  retval->read_lines = txt_read_lines;
  retval->warn_unused = txt_warn_unused;
  return retval;
}
