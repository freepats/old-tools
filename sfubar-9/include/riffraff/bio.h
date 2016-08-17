#ifndef __BIO_H__
#define __BIO_H__
extern FILE *bens_fopen(const char *, const char *);
extern void bens_fclose(FILE *);
extern void bens_fread(void *, size_t, size_t, FILE *);
extern void bens_fwrite(void *, size_t, size_t, FILE *);
extern void data_copy(FILE *, FILE *, int, int);
#endif
