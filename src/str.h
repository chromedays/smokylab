#pragma once
#include "util.h"
#include <stdbool.h>

C_INTERFACE_BEGIN

typedef struct _smkString {
  int len;
  int cap;
  char *buf;
} smkString;

void destroyString(smkString *str);
bool compareString(const smkString *a, const smkString *b);
void appendCStr(smkString *str, const char *toAppend);
void appendString(smkString *str, const smkString *toAppend);
void copyStringFromCStr(smkString *dst, const char *src);
void copyString(smkString *dst, const smkString *src);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-macros"
#define FORMAT_STRING(dst, ...)                                                \
  do {                                                                         \
    char *buf = MMALLOC_ARRAY(char, 1000);                                     \
    sprintf(buf, __VA_ARGS__);                                                 \
    copyStringFromCStr(dst, buf);                                              \
    MFREE(buf);                                                                \
  } while (0)
#pragma clang diagnostic pop

void copyBasePath(smkString *path);

void appendPathCStr(smkString *str, const char *path);
const char *pathBaseName(const smkString *str);

bool endsWithCString(const smkString *str, const char *ch);

C_INTERFACE_END
