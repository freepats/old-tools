#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <trap.h>

FILE *bens_fopen(const char *path, const char *mode)
{
  char mode2[10];
  FILE *retval;
  if (strcmp(path, "-") == 0) {
    if (mode[0] == 'r')
      retval = stdin;
    if (mode[0] == 'w' || mode[0] == 'a')
      retval = stdout;
  } else {
    /* The b is necessary for Windows. */
    snprintf(mode2, 10, "%sb", mode);
    retval = fopen(path, mode2);
    if (retval == NULL)
      trap_exit("sfubar: %s", strerror(errno));
  }

#ifdef __MINGW32__
  /* this was not necessary when cross-compiling from NetBSD using mingw32
     but it is necessary when using -mno-windows on Cygwin. -Ben
   */
  _setmode(_fileno(retval), _O_BINARY);
#endif

  return retval;
}

void bens_fclose(FILE *fp)
{
  if (fp != stdin && fp != stdout)
    fclose(fp);
}

void bens_fread(void *buffer, size_t size, size_t nmemb, FILE * fp)
{
  int result;
  int myerror;
  long position;

  position = ftell(fp);
  result = fread(buffer, size, nmemb, fp);
  if (result != nmemb) {
    myerror = ferror(fp);
    trap_exit("fread(%p,%zd,%zd,%p) = 0\nerror = %d, position = %ld, eof = %d",
      buffer, size, nmemb, fp, myerror, position, feof(fp));
  }
}

void bens_fwrite(void *buffer, size_t size, size_t nmemb, FILE * fp)
{
  int result;
  int myerror;
  long position;

  position = ftell(fp);
  result = fwrite(buffer, size, nmemb, fp);
  if (result != nmemb) {
    myerror = ferror(fp);
    trap_exit("fwrite(%p,%zd,%zd,%p) = 0\nerror = %d, position = %ld, eof = %d",
      buffer, size, nmemb, fp, myerror, position, feof(fp));
  }
}

void buffer_swap(char *buffer, int size)
{
  char *p = buffer;
  int left = size;
  char c;
  while (left > 1) {
    c = p[1];
    p[1] = p[0];
    p[0] = c;
    left -= 2;
    p += 2;
  }
}

#define DCLEN 2048
void data_copy(FILE *ifp, FILE *ofp, int length, int swap)
{
  char buffer[DCLEN];
  int left = length;
  int size;

  while (left > 0) {
    if (left > DCLEN) {
      size = DCLEN;
    } else {
      size = left;
    }
    bens_fread(buffer, size, 1, ifp);
    if (swap)
      buffer_swap(buffer, size);
    bens_fwrite(buffer, size, 1, ofp);
    left -= size;
  }
}
