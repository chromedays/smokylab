#pragma once
#include "util.h"

#define MATH_PI 3.141592f
#ifdef __cplusplus
#define DEFAULT_UP                                                             \
  Float3 { 0, 1, 0 }
#else
#define DEFAULT_UP                                                             \
  (Float3) { 0, 1, 0 }
#endif

C_INTERFACE_BEGIN

float degToRad(float deg);
float radToDeg(float rad);

typedef float Float2 __attribute__((ext_vector_type(2)));
typedef float Float3 __attribute__((ext_vector_type(3)));
typedef float Float4 __attribute__((ext_vector_type(4)));
typedef int Int2 __attribute__((ext_vector_type(2)));
typedef int Int3 __attribute__((ext_vector_type(3)));
typedef int Int4 __attribute__((ext_vector_type(4)));

float float2Dot(const Float2 a, const Float2 b);

float float3Dot(const Float3 a, const Float3 b);
Float3 float3Cross(const Float3 a, const Float3 b);
float float3LengthSq(const Float3 v);
float float3Length(const Float3 v);
Float3 float3Normalize(const Float3 v);
Float3 sphericalToCartesian(float r, float theta, float phi);

float float4Dot(const Float4 a, const Float4 b);

typedef struct _Mat4 {
  Float4 cols[4];
} Mat4;

Float4 mat4Row(const Mat4 mat, int n);
Mat4 mat4Multiply(const Mat4 a, const Mat4 b);
Mat4 mat4Inverse(Mat4 m);
Mat4 mat4Transpose(Mat4 m);
Mat4 mat4Identity(void);
Mat4 mat4Translate(Float3 position);
Mat4 mat4Scale(Float3 scale);
Mat4 mat4RotateY(float rad);
Mat4 mat4LookAt(const Float3 eye, const Float3 target, const Float3 upAxis);
Mat4 mat4Perspective(float fov, float aspectRatio, float nearZ, float farZ);

Mat4 quatToMat4(Float4 q);
Float4 quatRotateAroundAxis(Float3 axis, float angleRad);

C_INTERFACE_END
