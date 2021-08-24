#include "camera.h"
#include "app.h"
#include "editor_gui.h"
#include <math.h>

void initCameraLookingAtTarget(Camera *cam, Float3 pos, Float3 target) {
  *cam = (Camera){.pos = pos};
  Float3 look = target - pos;
  // TODO: Assert when length of look.xz = 0
  // TODO: Use radians for pitch and yaw
  cam->pitch = radToDeg(atan2f(look.y, float2Length(look.xz)));
  cam->yaw = radToDeg(atan2f(look.z, look.x));
}

void moveCameraByInputs(Camera *cam) {
  if (gApp.input.mouseDown && !guiWantsCaptureMouse()) {
    cam->yaw += (float)gApp.input.cursorDeltaX * 0.6f;
    cam->pitch -= (float)gApp.input.cursorDeltaY * 0.6f;
    cam->pitch = CLAMP(cam->pitch, -88.f, 88.f);
  }

  int dx = 0, dy = 0;
  if (gApp.input.leftDown) {
    dx -= 1;
  }
  if (gApp.input.rightDown) {
    dx += 1;
  }
  if (gApp.input.forwardDown) {
    dy += 1;
  }
  if (gApp.input.backDown) {
    dy -= 1;
  }

  float camMovementSpeed = 4.f;
  Float3 camMovement = (getRight(cam) * (float)dx * camMovementSpeed +
                        getLook(cam) * (float)dy * camMovementSpeed) *
                       gApp.input.dt;
  cam->pos += camMovement;
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