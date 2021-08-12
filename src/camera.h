#pragma once
#include "vmath.h"
#include "util.h"

C_INTERFACE_BEGIN

typedef struct _Camera {
  Float3 pos;
  float yaw;
  float pitch;
} Camera;

void initCameraLookingAtTarget(Camera *cam, Float3 pos, Float3 target);

Float3 getLook(const Camera *cam);
Float3 getRight(const Camera *cam);
Mat4 getViewMatrix(const Camera *cam);

C_INTERFACE_END