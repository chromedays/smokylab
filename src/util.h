#pragma once
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#pragma clang diagnostic pop

#ifdef __cplusplus
#define C_INTERFACE_BEGIN extern "C" {
#define C_INTERFACE_END }
#else
#define C_INTERFACE_BEGIN
#define C_INTERFACE_END
#endif

#define MMALLOC(type) (type *)calloc(1, sizeof(type))
#define MMALLOC_ARRAY(type, count)                                             \
  (type *)calloc(castI32Usize(count), sizeof(type))
#define MMALLOC_UNINITIALIZED(type) (type *)malloc(sizeof(type))
#define MMALLOC_ARRAY_UNINITIALIZED(type, count)                               \
  (type *)malloc(castI32Usize(count) * sizeof(type))
#define MFREE(p) free(p)

#define ARRAY_SIZE(arr) ((int)(sizeof(arr) / sizeof((arr)[0])))

#define UNUSED __attribute__((unused))

#ifdef DEBUG
#define ASSERT(exp) assert(exp)
#define VK_ASSERT(exp)                                                         \
  do {                                                                         \
    VkResult vulkanCallResult = exp;                                           \
    ASSERT(vulkanCallResult == VK_SUCCESS);                                    \
  } while (0)

#define HR_ASSERT(exp)                                                         \
  do {                                                                         \
    HRESULT hr__ = (exp);                                                      \
    ASSERT(SUCCEEDED(hr__));                                                   \
  } while (0)
#else
#define ASSERT(exp)                                                            \
  do {                                                                         \
    UNUSED bool notused = (exp);                                               \
  } while (0)
#define VK_ASSERT(exp)                                                         \
  do {                                                                         \
    UNUSED VkResult vulkanCallResult = (exp);                                  \
    ASSERT(vulkanCallResult == VK_SUCCESS);                                    \
  } while (0)

#define HR_ASSERT(exp)                                                         \
  do {                                                                         \
    UNUSED HRESULT hr__ = (exp);                                               \
    ASSERT(SUCCEEDED(hr__));                                                   \
  } while (0)
#endif

#ifdef __cplusplus
#define COM_RELEASE(com)                                                       \
  do {                                                                         \
    if (com) {                                                                 \
      ((IUnknown *)(com))->Release();                                          \
      com = NULL;                                                              \
    }                                                                          \
  } while (0)
#else
#define COM_RELEASE(com)                                                       \
  do {                                                                         \
    if (com) {                                                                 \
      ((IUnknown *)(com))->lpVtbl->Release(com);                               \
      com = NULL;                                                              \
    }                                                                          \
  } while (0)
#endif

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

#ifdef __cplusplus
#define STATIC_ASSERT(exp) static_assert(exp, #exp)
#else
#define STATIC_ASSERT(exp) _Static_assert(exp, #exp)
#endif

#define FORWARD_DECL(name) typedef struct _##name name

#define SSIZEOF32(type) castUsizeI32(sizeof(type))

C_INTERFACE_BEGIN

inline uint32_t castI32U32(int32_t value) {
  ASSERT(value >= 0);
  return (uint32_t)value;
}

inline size_t castI32Usize(int32_t value) {
  ASSERT(value >= 0);
  return (size_t)value;
}

inline int32_t castI64I32(int64_t value) {
  ASSERT(value <= 0x7FFFFFFFll);
  return (int32_t)value;
}

inline uint32_t castI64U32(int64_t value) {
  ASSERT(value >= 0 && value <= 0xFFFFFFFFll);
  return (uint32_t)value;
}

inline int32_t castU32I32(uint32_t value) {
  ASSERT(value <= 0x7FFFFFFFu);
  return (int32_t)value;
}

inline int32_t castU64I32(uint64_t value) {
  ASSERT(value <= 0x7FFFFFFFull);
  return (int32_t)value;
}

inline uint32_t castU64U32(uint64_t value) {
  ASSERT(value <= 0xFFFFFFFFull);
  return (uint32_t)value;
}

inline int32_t castUsizeI32(size_t value) {
  STATIC_ASSERT(sizeof(value) == 8);
  return castU64I32(value);
}

inline uint32_t castUsizeU32(size_t value) {
  STATIC_ASSERT(sizeof(value) == 8);
  return castU64U32(value);
}

inline uint32_t castSsizeU32(ptrdiff_t value) {
  STATIC_ASSERT(sizeof(value) == 8);
  return castI64U32(value);
}

inline int32_t castSsizeI32(ptrdiff_t value) {
  STATIC_ASSERT(sizeof(value) == 8);
  return castI64I32(value);
}

inline uint32_t castFloatToU32(float value) {
  ASSERT(value >= 0.f);
  return (uint32_t)value;
}

C_INTERFACE_END
