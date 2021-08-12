#include "camera.h"
#include <math.h>

void initCameraLookingAtTarget(Camera *cam, Float3 pos, Float3 target) {
  *cam = (Camera){.pos = pos};
  Float3 look = target - pos;
  // TODO: Assert when length of look.xz = 0
  // TODO: Use radians for pitch and yaw
  cam->pitch = radToDeg(atan2f(look.y, float2Length(look.xz)));
  cam->yaw = radToDeg(atan2f(look.z, look.x));
}

Float3 getLook(const Camera *cam) {
  float yawRadian = degToRad(cam->yaw);
  float pitchRadian = degToRad(cam->pitch);
  float cosPitch = cosf(pitchRadian);
  return float3(-sinf(yawRadian) * cosPitch, sinf(pitchRadian),
                cosf(yawRadian) * cosPitch);
}

Float3 getRight(const Camera *cam) {
  return float3Normalize(float3Cross(getLook(cam), DEFAULT_UP));
}

Mat4 getViewMatrix(const Camera *cam) {
  return mat4LookAt(cam->pos, cam->pos + getLook(cam), DEFAULT_UP);
}