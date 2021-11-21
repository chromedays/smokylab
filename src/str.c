#include "str.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <SDL2/SDL.h>
#pragma clang diagnostic pop

void destroyString(smkString *str) {
  MFREE(str->buf);
  *str = (smkString){};
}

bool compareString(const smkString *a, const smkString *b) {
  bool result = false;

  if (a->len == b->len) {
    result = (memcmp(a->buf, b->buf, castI32U32(a->len) * sizeof(char)) == 0);
  }

  return result;
}

static void tryExpand(smkString *str, int newLen) {
  if (str->cap < newLen + 1) {
    int newCap = MAX(str->cap, 1);
    do {
      newCap <<= 2;
    } while (newCap < (newLen + 1));

    char *oldBuf = str->buf;
    int oldCap = str->cap;
    str->cap = newCap;
    str->buf = MMALLOC_ARRAY(char, str->cap);
    memcpy(str->buf, oldBuf, castI32U32(oldCap));
    MFREE(oldBuf);
  }
}

void appendCStr(smkString *str, const char *toAppend) {
  int offset = str->len;
  int appendLen = (int)strlen(toAppend);
  int newLen = str->len + appendLen;
  tryExpand(str, newLen);
  str->len = newLen;

  memcpy(str->buf + offset, toAppend, castI32U32(appendLen));
  str->buf[str->len] = 0;
}

void appendString(smkString *str, const smkString *toAppend) {
  int offset = str->len;
  int appendLen = toAppend->len;
  int newLen = str->len + appendLen;
  tryExpand(str, newLen);
  str->len = newLen;

  memcpy(str->buf + offset, toAppend->buf, castI32U32(appendLen));
  str->buf[str->len] = 0;
}

void copyStringFromCStr(smkString *dst, const char *src) {
  int srcLen = (int)strlen(src);
  tryExpand(dst, srcLen);
  memcpy(dst->buf, src, castI32U32(srcLen));
  dst->buf[srcLen] = 0;
  dst->len = srcLen;
}

void copyString(smkString *dst, const smkString *src) {
  tryExpand(dst, src->len);
  memcpy(dst->buf, src->buf, castI32U32(src->len));
  dst->buf[src->len] = 0;
  dst->len = src->len;
}

static bool isSeparator(char ch) {
  bool result = ch == '/' || ch == '\\';
  return result;
}

void copyBasePath(smkString *path) {
  char *tmp = SDL_GetBasePath();
  copyStringFromCStr(path, tmp);
  SDL_free(tmp);
}

void appendPathCStr(smkString *str, const char *path) {
  while (*path && isSeparator(*path)) {
    ++path;
  }

  if (str->len == 0 || !isSeparator(str->buf[str->len - 1])) {
    appendCStr(str, "/");
  }

  appendCStr(str, path);
}

const char *pathBaseName(const smkString *str) {
  int i = str->len - 1;
  while (isSeparator(str->buf[i]) && i >= 0) {
    --i;
  }

  while (!isSeparator(str->buf[i]) && i >= 0) {
    --i;
  }

  return i >= 0 ? &str->buf[i + 1] : "";
}

bool endsWithCString(const smkString *str, const char *ch) {
  int chlen = (int)strlen(ch);

  if (str->len < chlen) {
    return false;
  }

  bool result =
      (strncmp(&str->buf[str->len - chlen], ch, castI32U32(chlen)) == 0);
  return result;
}
