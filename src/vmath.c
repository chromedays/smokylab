#include "vmath.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <math.h>
#pragma clang diagnostic pop

C_INTERFACE_BEGIN

float degToRad(float deg) {
  float rad = deg * MATH_PI / 180.f;
  return rad;
}

float radToDeg(float rad) {
  float deg = rad * 180.f / MATH_PI;
  return deg;
}

float float2Dot(const Float2 a, const Float2 b) {
  float result = a.x * b.x + a.y * b.y;
  return result;
}

Float3 float3(float x, float y, float z) { return (Float3){x, y, z}; }

float float3Dot(const Float3 a, const Float3 b) {
  float result = a.x * b.x + a.y * b.y + a.z * b.z;
  return result;
}

Float3 float3Cross(const Float3 a, const Float3 b) {
  Float3 result = {
      a.y * b.z - a.z * b.y,
      a.z * b.x - a.x * b.z,
      a.x * b.y - a.y * b.x,
  };

  return result;
}

float float3LengthSq(const Float3 v) {
  float result = float3Dot(v, v);
  return result;
}

float float3Length(const Float3 v) {
  float result = sqrtf(float3LengthSq(v));
  return result;
}

Float3 float3Normalize(const Float3 v) {
  float len = float3Length(v);
  Float3 normalized = v;
  if (len > 0) {
    normalized /= len;
  }
  return normalized;
}

Float3 sphericalToCartesian(float r, float theta, float phi) {
  float cosTheta = cosf(theta);

  Float3 cartesian = {
      r * cosTheta * cosf(phi),
      r * sinf(theta),
      r * cosTheta * -sinf(phi),
  };

  return cartesian;
}

Float4 float4(float x, float y, float z, float w) {
  return (Float4){x, y, z, w};
}

float float4Dot(const Float4 a, const Float4 b) {
  float result = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
  return result;
}

Float4 mat4Row(const Mat4 mat, int n) {
  Float4 row = {mat.cols[0][n], mat.cols[1][n], mat.cols[2][n], mat.cols[3][n]};
  return row;
}

Mat4 mat4Multiply(const Mat4 a, const Mat4 b) {
  Float4 rows[4] = {
      mat4Row(a, 0),
      mat4Row(a, 1),
      mat4Row(a, 2),
      mat4Row(a, 3),
  };

  Mat4 result = {0};
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      result.cols[j][i] = float4Dot(rows[i], b.cols[j]);
    }
  }

  return result;
}

static Mat4 mat4Adjugate(Mat4 m) {
  Mat4 adjugate = {{{m.cols[1].y * m.cols[2].z * m.cols[3].w +
                         m.cols[3].y * m.cols[1].z * m.cols[2].w +
                         m.cols[2].y * m.cols[3].z * m.cols[1].w -
                         m.cols[1].y * m.cols[3].z * m.cols[2].w -
                         m.cols[2].y * m.cols[1].z * m.cols[3].w -
                         m.cols[3].y * m.cols[2].z * m.cols[1].w,
                     m.cols[0].y * m.cols[3].z * m.cols[2].w +
                         m.cols[2].y * m.cols[0].z * m.cols[3].w +
                         m.cols[3].y * m.cols[2].z * m.cols[0].w -
                         m.cols[3].y * m.cols[0].z * m.cols[2].w -
                         m.cols[2].y * m.cols[3].z * m.cols[0].w -
                         m.cols[0].y * m.cols[2].z * m.cols[3].w,
                     m.cols[0].y * m.cols[1].z * m.cols[3].w +
                         m.cols[3].y * m.cols[0].z * m.cols[1].w +
                         m.cols[1].y * m.cols[3].z * m.cols[0].w -
                         m.cols[0].y * m.cols[3].z * m.cols[1].w -
                         m.cols[1].y * m.cols[0].z * m.cols[3].w -
                         m.cols[3].y * m.cols[1].z * m.cols[0].w,
                     m.cols[0].y * m.cols[2].z * m.cols[1].w +
                         m.cols[1].y * m.cols[0].z * m.cols[2].w +
                         m.cols[2].y * m.cols[1].z * m.cols[0].w -
                         m.cols[0].y * m.cols[1].z * m.cols[2].w -
                         m.cols[2].y * m.cols[0].z * m.cols[1].w -
                         m.cols[1].y * m.cols[2].z * m.cols[0].w},
                    {m.cols[1].z * m.cols[3].w * m.cols[2].x +
                         m.cols[2].z * m.cols[1].w * m.cols[3].x +
                         m.cols[3].z * m.cols[2].w * m.cols[1].x -
                         m.cols[1].z * m.cols[2].w * m.cols[3].x -
                         m.cols[3].z * m.cols[1].w * m.cols[2].x -
                         m.cols[2].z * m.cols[3].w * m.cols[1].x,
                     m.cols[0].z * m.cols[2].w * m.cols[3].x +
                         m.cols[3].z * m.cols[0].w * m.cols[2].x +
                         m.cols[2].z * m.cols[3].w * m.cols[0].x -
                         m.cols[0].z * m.cols[3].w * m.cols[2].x -
                         m.cols[2].z * m.cols[0].w * m.cols[3].x -
                         m.cols[3].z * m.cols[2].w * m.cols[0].x,
                     m.cols[0].z * m.cols[3].w * m.cols[1].x +
                         m.cols[1].z * m.cols[0].w * m.cols[3].x +
                         m.cols[3].z * m.cols[1].w * m.cols[0].x -
                         m.cols[0].z * m.cols[1].w * m.cols[3].x -
                         m.cols[3].z * m.cols[0].w * m.cols[1].x -
                         m.cols[1].z * m.cols[3].w * m.cols[0].x,
                     m.cols[0].z * m.cols[1].w * m.cols[2].x +
                         m.cols[2].z * m.cols[0].w * m.cols[1].x +
                         m.cols[1].z * m.cols[2].w * m.cols[0].x -
                         m.cols[0].z * m.cols[2].w * m.cols[1].x -
                         m.cols[1].z * m.cols[0].w * m.cols[2].x -
                         m.cols[2].z * m.cols[1].w * m.cols[0].x},
                    {m.cols[1].w * m.cols[2].x * m.cols[3].y +
                         m.cols[3].w * m.cols[1].x * m.cols[2].y +
                         m.cols[2].w * m.cols[3].x * m.cols[1].y -
                         m.cols[1].w * m.cols[3].x * m.cols[2].y -
                         m.cols[2].w * m.cols[1].x * m.cols[3].y -
                         m.cols[3].w * m.cols[2].x * m.cols[1].y,
                     m.cols[0].w * m.cols[3].x * m.cols[2].y +
                         m.cols[2].w * m.cols[0].x * m.cols[3].y +
                         m.cols[3].w * m.cols[2].x * m.cols[0].y -
                         m.cols[0].w * m.cols[2].x * m.cols[3].y -
                         m.cols[3].w * m.cols[0].x * m.cols[2].y -
                         m.cols[2].w * m.cols[3].x * m.cols[0].y,
                     m.cols[0].w * m.cols[1].x * m.cols[3].y +
                         m.cols[3].w * m.cols[0].x * m.cols[1].y +
                         m.cols[1].w * m.cols[3].x * m.cols[0].y -
                         m.cols[0].w * m.cols[3].x * m.cols[1].y -
                         m.cols[1].w * m.cols[0].x * m.cols[3].y -
                         m.cols[3].w * m.cols[1].x * m.cols[0].y,
                     m.cols[0].w * m.cols[2].x * m.cols[1].y +
                         m.cols[1].w * m.cols[0].x * m.cols[2].y +
                         m.cols[2].w * m.cols[1].x * m.cols[0].y -
                         m.cols[0].w * m.cols[1].x * m.cols[2].y -
                         m.cols[2].w * m.cols[0].x * m.cols[1].y -
                         m.cols[1].w * m.cols[2].x * m.cols[0].y},
                    {m.cols[1].x * m.cols[3].y * m.cols[2].z +
                         m.cols[2].x * m.cols[1].y * m.cols[3].z +
                         m.cols[3].x * m.cols[2].y * m.cols[1].z -
                         m.cols[1].x * m.cols[2].y * m.cols[3].z -
                         m.cols[3].x * m.cols[1].y * m.cols[2].z -
                         m.cols[2].x * m.cols[3].y * m.cols[1].z,
                     m.cols[0].x * m.cols[2].y * m.cols[3].z +
                         m.cols[3].x * m.cols[0].y * m.cols[2].z +
                         m.cols[2].x * m.cols[3].y * m.cols[0].z -
                         m.cols[0].x * m.cols[3].y * m.cols[2].z -
                         m.cols[2].x * m.cols[0].y * m.cols[3].z -
                         m.cols[3].x * m.cols[2].y * m.cols[0].z,
                     m.cols[0].x * m.cols[3].y * m.cols[1].z +
                         m.cols[1].x * m.cols[0].y * m.cols[3].z +
                         m.cols[3].x * m.cols[1].y * m.cols[0].z -
                         m.cols[0].x * m.cols[1].y * m.cols[3].z -
                         m.cols[3].x * m.cols[0].y * m.cols[1].z -
                         m.cols[1].x * m.cols[3].y * m.cols[0].z,
                     m.cols[0].x * m.cols[1].y * m.cols[2].z +
                         m.cols[2].x * m.cols[0].y * m.cols[1].z +
                         m.cols[1].x * m.cols[2].y * m.cols[0].z -
                         m.cols[0].x * m.cols[2].y * m.cols[1].z -
                         m.cols[1].x * m.cols[0].y * m.cols[2].z -
                         m.cols[2].x * m.cols[1].y * m.cols[0].z}}};
  return adjugate;
}

static float mat4Determinant(Mat4 m) {
  float det = m.cols[0].x * (m.cols[1].y * m.cols[2].z * m.cols[3].w +
                             m.cols[3].y * m.cols[1].z * m.cols[2].w +
                             m.cols[2].y * m.cols[3].z * m.cols[1].w -
                             m.cols[1].y * m.cols[3].z * m.cols[2].w -
                             m.cols[2].y * m.cols[1].z * m.cols[3].w -
                             m.cols[3].y * m.cols[2].z * m.cols[1].w) +
              m.cols[0].y * (m.cols[1].z * m.cols[3].w * m.cols[2].x +
                             m.cols[2].z * m.cols[1].w * m.cols[3].x +
                             m.cols[3].z * m.cols[2].w * m.cols[1].x -
                             m.cols[1].z * m.cols[2].w * m.cols[3].x -
                             m.cols[3].z * m.cols[1].w * m.cols[2].x -
                             m.cols[2].z * m.cols[3].w * m.cols[1].x) +
              m.cols[0].z * (m.cols[1].w * m.cols[2].x * m.cols[3].y +
                             m.cols[3].w * m.cols[1].x * m.cols[2].y +
                             m.cols[2].w * m.cols[3].x * m.cols[1].y -
                             m.cols[1].w * m.cols[3].x * m.cols[2].y -
                             m.cols[2].w * m.cols[1].x * m.cols[3].y -
                             m.cols[3].w * m.cols[2].x * m.cols[1].y) +
              m.cols[0].w * (m.cols[1].x * m.cols[3].y * m.cols[2].z +
                             m.cols[2].x * m.cols[1].y * m.cols[3].z +
                             m.cols[3].x * m.cols[2].y * m.cols[1].z -
                             m.cols[1].x * m.cols[2].y * m.cols[3].z -
                             m.cols[3].x * m.cols[1].y * m.cols[2].z -
                             m.cols[2].x * m.cols[3].y * m.cols[1].z);
  return det;
}

Mat4 mat4Inverse(Mat4 m) {
  Mat4 adjugate = mat4Adjugate(m);
  float det = mat4Determinant(m);
  Mat4 inv = {{
      adjugate.cols[0] / det,
      adjugate.cols[1] / det,
      adjugate.cols[2] / det,
      adjugate.cols[3] / det,
  }};

  return inv;
}

Mat4 mat4Transpose(Mat4 m) {
  Mat4 transposed = {{
      mat4Row(m, 0),
      mat4Row(m, 1),
      mat4Row(m, 2),
      mat4Row(m, 3),
  }};

  return transposed;
}

Mat4 mat4Identity(void) {
  Mat4 identity = {{
      {1, 0, 0, 0},
      {0, 1, 0, 0},
      {0, 0, 1, 0},
      {0, 0, 0, 1},
  }};

  return identity;
}

Mat4 mat4Translate(Float3 position) {
  Mat4 translation = {{
      {1, 0, 0, 0},
      {0, 1, 0, 0},
      {0, 0, 1, 0},
      {position.x, position.y, position.z, 1},
  }};

  return translation;
}

Mat4 mat4Scale(Float3 scale) {
  Mat4 scaleMat = {{
      {scale.x, 0, 0, 0},
      {0, scale.y, 0, 0},
      {0, 0, scale.z, 0},
      {0, 0, 0, 1},
  }};

  return scaleMat;
}

Mat4 mat4RotateY(float rad) {
  float c = cosf(rad);
  float s = sinf(rad);

  Mat4 rotation = {{
      {c, 0, -s, 0},
      {0, 1, 0, 0},
      {s, 0, c, 0},
      {0, 0, 0, 1},
  }};

  return rotation;
}

Mat4 mat4LookAt(const Float3 eye, const Float3 target, const Float3 upAxis) {
  Float3 forward = eye - target;
  forward = float3Normalize(forward);
  Float3 right = float3Cross(upAxis, forward);
  right = float3Normalize(right);
  Float3 up = float3Cross(forward, right);
  up = float3Normalize(up);

  Mat4 lookat = {{
      {right.x, up.x, forward.x, 0},
      {right.y, up.y, forward.y, 0},
      {right.z, up.z, forward.z, 0},
      {-float3Dot(eye, right), -float3Dot(eye, up), -float3Dot(eye, forward),
       1},
  }};

  return lookat;
}

Mat4 mat4Perspective(float fovyDegs, float aspectRatio, float nearZ,
                     float farZ) {
  float yScale = 1.f / tanf(degToRad(fovyDegs) * 0.5f);
  float xScale = yScale / aspectRatio;

  float zRange = farZ - nearZ;
  float zScale = nearZ / zRange;
  float wzScale = farZ * nearZ / zRange;

  Mat4 persp = {{
      {xScale, 0, 0, 0},
      {0, yScale, 0, 0},
      {0, 0, zScale, -1},
      {0, 0, wzScale, 0},
  }};

  return persp;
}

Mat4 quatToMat4(Float4 q) {
  Mat4 mat = {{
      {1 - 2 * (q.y * q.y + q.z * q.z), 2 * (q.x * q.y + q.w * q.z),
       2 * (q.x * q.z - q.w * q.y), 0},
      {2 * (q.x * q.y - q.w * q.z), 1 - 2 * (q.x * q.x + q.z * q.z),
       2 * (q.y * q.z + q.w * q.x), 0},
      {2 * (q.x * q.z + q.w * q.y), 2 * (q.y * q.z - q.w * q.x),
       1 - 2 * (q.x * q.x + q.y * q.y), 0},
      {0, 0, 0, 1},
  }};

  return mat;
}

Float4 quatRotateAroundAxis(Float3 axis, float angleRad) {
  float halfAngle = angleRad * 0.5f;

  Float4 quat;
  quat.xyz = float3Normalize(axis) * sinf(halfAngle);
  quat.w = cosf(halfAngle);

  return quat;
}

C_INTERFACE_END
