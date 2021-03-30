#pragma once
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#pragma clang diagnostic pop

#ifdef __cplusplus
#define C_INTERFACE_BEGIN extern "C" {
#define C_INTERFACE_END }
#else
#define C_INTERFACE_BEGIN
#define C_INTERFACE_END
#endif

#define MMALLOC(type) (type *)calloc(1, sizeof(type))
#define MMALLOC_ARRAY(type, count) (type *)calloc(count, sizeof(type))
#define MMALLOC_UNINITIALIZED(type) (type *)malloc(sizeof(type))
#define MMALLOC_ARRAY_UNINITIALIZED(type, count)                               \
  (type *)malloc((count) * sizeof(type))
#define MFREE(p) free(p)

#define ARRAY_SIZE(arr) ((int)(sizeof(arr) / sizeof((arr)[0])))

#define ASSERT(exp) assert(exp)
#define VK_ASSERT(exp)                                                         \
  do {                                                                         \
    VkResult vulkanCallResult = exp;                                           \
    ASSERT(vulkanCallResult == VK_SUCCESS);                                    \
  } while (0)

#define LOG(...)                                                               \
  do {                                                                         \
    printf(__VA_ARGS__);                                                       \
    printf("\n");                                                              \
  } while (0)

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef CLAMP
#define CLAMP(x, lo, hi) MIN(hi, MAX(lo, x))
#endif

#define UNUSED __attribute__((unused))

inline uint32_t castI32U32(int32_t value) {
  ASSERT(value >= 0);
  return (uint32_t)value;
}
