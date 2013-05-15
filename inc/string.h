#ifndef __STRING__H__
#define __STRING__H__

#define ISHEX(b) (((b) >= 'a' && (b) <= 'f') || ((b) >= 'A' && (b) <= 'F') || ((b) >= '0' && (b) <= '9'))
#define ISDIGIT(x) ((x) >= '0' && (x) <= '9')

#include <stdio.h>

char * strncpy(char *, const char *,uint32_t);
uint32_t strnlen(const char *, uint32_t);
uint32_t strlen(const char *);
long strcmp(const char *, const char *);
long strncmp(const char *, const char *, uint32_t);
char *strcpy(char *, const char *);
char *strcat(char *, const char *);
char *strncat(char *, const char *, uint32_t);
long memcmp(const void*, const void*, uint32_t);
void *memcpy(void *, const void*, uint32_t);
void *memmove(void *, const void*, uint32_t);
void *memset(void *, int, uint32_t);
char *strchr(const char *, uint32_t);
char *strrchr(const char *, uint32_t);
int GuStrToSize(const char *, uint32_t *);

#endif
