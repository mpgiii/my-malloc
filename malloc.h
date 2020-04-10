#ifndef MALLOC_H
#define MALLOC_H

#include "header.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_DEBUG_LEN 64

void* malloc(size_t size);

void* calloc(size_t nmemb, size_t size);

void free(void* ptr);

void* realloc(void* ptr, size_t size);

#endif
