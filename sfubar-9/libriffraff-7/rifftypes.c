#include <stdio.h>
#include <string.h>

#include <trap.h>
#include "bits.h"
#include "rifftypes.h"

void *my_get_CHAR(char *retval, void *data, int size)
{
  memcpy(retval, data, size);
  return data + size;
}

void *my_get_ZSTR(char **retval, void *data, int size)
{
  char *b = data;
  char *p = data + size - 1;
  *retval = data;
  if (p[0] != '\000') {
    while (*b && b < p)
      b++;
    b[0] = '\000';
    trap_warn("Unterminated ZSTR");
  }
  return data + size;
}

void *my_get_DWORD_LE(myDWORD *retval, void *data)
{
  memcpy(retval, data, sizeof(myDWORD));
  *retval = LE32H(*retval);
  return data + sizeof(myDWORD);
}

void *my_get_WORD_LE(myWORD *retval, void *data)
{
  memcpy(retval, data, sizeof(myWORD));
  *retval = LE16H(*retval);
  return data + sizeof(myWORD);
}

void *my_get_SHORT_LE(mySHORT *retval, void *data)
{
  memcpy(retval, data, sizeof(mySHORT));
  *retval = LE16H(*retval);
  return data + sizeof(mySHORT);
}

void *my_get_DWORD_BE(myDWORD *retval, void *data)
{
  memcpy(retval, data, sizeof(myDWORD));
  *retval = BE32H(*retval);
  return data + sizeof(myDWORD);
}

void *my_get_WORD_BE(myWORD *retval, void *data)
{
  memcpy(retval, data, sizeof(myWORD));
  *retval = BE16H(*retval);
  return data + sizeof(myWORD);
}

void *my_get_SHORT_BE(mySHORT *retval, void *data)
{
  memcpy(retval, data, sizeof(mySHORT));
  *retval = BE16H(*retval);
  return data + sizeof(mySHORT);
}

void *my_get_BYTE(myBYTE *retval, void *data)
{
  memcpy(retval, data, sizeof(myBYTE));
  return data + sizeof(myBYTE);
}

void *my_get_IEEE80(myIEEE80 *retval, void *data)
{
  memcpy(retval, data, sizeof(myIEEE80));
  return data + sizeof(myIEEE80);
}

void *my_get_PSTRING(myPSTRING *retval, void *data)
{
  char c;
  memset(retval->str, 0, sizeof(retval->str));
  memcpy(&c, data, 1);
  retval->count = (int)c;
  data++;
  memcpy(retval->str, data, retval->count);
  data += retval->count;
  if ((1 + retval->count) % 2)
    data++;
  return data;
}

void *my_put_CHAR(char *retval, void *data, int size)
{
  memcpy(data, retval, size);
  return data + size;
}

void *my_put_DWORD_LE(myDWORD *retval, void *data)
{
  myDWORD temp = HLE32(*retval);
  memcpy(data, &temp, sizeof(myDWORD));
  return data + sizeof(myDWORD);
}

void *my_put_WORD_LE(myWORD *retval, void *data)
{
  myWORD temp = HLE16(*retval);
  memcpy(data, &temp, sizeof(myWORD));
  return data + sizeof(myWORD);
}

void *my_put_SHORT_LE(mySHORT *retval, void *data)
{
  mySHORT temp = HLE16(*retval);
  memcpy(data, &temp, sizeof(mySHORT));
  return data + sizeof(mySHORT);
}

void *my_put_DWORD_BE(myDWORD *retval, void *data)
{
  myDWORD temp = HBE32(*retval);
  memcpy(data, &temp, sizeof(myDWORD));
  return data + sizeof(myDWORD);
}

void *my_put_WORD_BE(myWORD *retval, void *data)
{
  myWORD temp = HBE16(*retval);
  memcpy(data, &temp, sizeof(myWORD));
  return data + sizeof(myWORD);
}

void *my_put_SHORT_BE(mySHORT *retval, void *data)
{
  mySHORT temp = HBE16(*retval);
  memcpy(data, &temp, sizeof(mySHORT));
  return data + sizeof(mySHORT);
}

void *my_put_BYTE(myBYTE *retval, void *data)
{
  memcpy(data, retval, sizeof(myBYTE));
  return data + sizeof(myBYTE);
}

void *my_put_IEEE80(myIEEE80 *retval, void *data)
{
  memcpy(data, retval, sizeof(myIEEE80));
  return data + sizeof(myIEEE80);
}

void *my_put_PSTRING(myPSTRING *retval, void *data)
{
  char c;
  c = (char)retval->count;
  memcpy(data, &c, 1);
  data++;
  memcpy(data, retval->str, retval->count);
  data += retval->count;
  if (!is_even(1 + retval->count)) {
    memset(data, 0, 1);
    data++;
  }
  return data;
}

int my_size_PSTRING(myPSTRING *pstr)
{
  int retval = 1;
  retval += pstr->count;
  if ((1 + pstr->count) % 2)
    retval++;
  return retval;
}
