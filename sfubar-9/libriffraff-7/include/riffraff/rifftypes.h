#ifndef __RIFF_TYPES_H__
#define __RIFF_TYPES_H__

#ifdef uint8_t
typedef uint8_t myBYTE;
typedef int8_t myCHAR;
typedef int16_t mySHORT;
typedef uint16_t myWORD;
typedef uint32_t myDWORD;
#else
typedef unsigned char myBYTE;
typedef char myCHAR;
typedef char* myZSTR;
typedef unsigned int myDWORD;
typedef short int mySHORT;
typedef unsigned short int myWORD;
#endif

typedef union {
  char str[4];
  myDWORD dword;
} myFOURCC;

typedef char myIEEE80[10];

#define PSTRING_STR_MAXLEN 256
typedef struct {
  int count; /* pascal string, maximum value for count is 255 */
  char str[PSTRING_STR_MAXLEN]; /* also zero-terminate this string */
} myPSTRING;

extern void *my_get_CHAR(char *, void *, int);
extern void *my_get_ZSTR(char **, void *, int);
extern void *my_get_DWORD_LE(myDWORD *, void *);
extern void *my_get_WORD_LE(myWORD *, void *);
extern void *my_get_SHORT_LE(mySHORT *, void *);
extern void *my_get_DWORD_BE(myDWORD *, void *);
extern void *my_get_WORD_BE(myWORD *, void *);
extern void *my_get_SHORT_BE(mySHORT *, void *);
extern void *my_get_BYTE(myBYTE *, void *);
extern void *my_get_IEEE80(myIEEE80 *, void *);
extern void *my_get_PSTRING(myPSTRING *, void *);
extern void *my_put_CHAR(char *, void *, int);
#define my_put_ZSTR my_put_CHAR
extern void *my_put_DWORD_LE(myDWORD *, void *);
extern void *my_put_WORD_LE(myWORD *, void *);
extern void *my_put_SHORT_LE(mySHORT *, void *);
extern void *my_put_DWORD_BE(myDWORD *, void *);
extern void *my_put_WORD_BE(myWORD *, void *);
extern void *my_put_SHORT_BE(mySHORT *, void *);
extern void *my_put_BYTE(myBYTE *, void *);
extern void *my_put_IEEE80(myIEEE80 *, void *);
extern void *my_put_PSTRING(myPSTRING *, void *);
extern int my_size_PSTRING(myPSTRING *);
#endif
