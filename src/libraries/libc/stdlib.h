#ifndef _LIBC_STDLIB_H
#define _LIBC_STDLIB_H

#include <alloca.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define _PATH_DEV "/dev/"
#define _PATH_TMP "/tmp/"
#define SPECNAMELEN 255 /* max length of devicename */

void *malloc(size_t size);
void *calloc(size_t n, size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);
void *reallocarray(void *ptr, size_t nmemb, size_t size);
int posix_openpt(int flags);
char *ptsname(int fd);
void abort();
void exit(int status);
int atexit(void (*)(void));
int atoi(const char *);
char *getenv(const char *);

#endif
