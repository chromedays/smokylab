#include "camera.h"
#include "app.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <imgui.h>
#include <imgui_internal.h>
#ifdef SMK_DIRECTX11
#include <imgui_impl_dx11.h>
#endif
#include <backends/imgui_impl_sdl.h>
#pragma clang diagnostic pop

#include <math.h>

void initCameraLookingAtTarget(Camera *cam, Float3 pos, Float3 target) {
  *cam = {};
  cam->pos = pos;
  Float3 look = target - pos;
  // TODO: Assert when length of look.xz = 0
  // TODO: Use radians for pitch and yaw
  cam->pitch = radToDeg(atan2f(look.y, float2Length(look.xz)));
  cam->yaw = radToDeg(atan2f(look.z, look.x));

  cam->nearZ = 0.1f;
  cam->farZ = 500.f;
  cam->aspectRatio = getWindowAspectRatio(gApp.window);
  cam->verticalFovDeg = 60.f;
}

void moveCameraByInputs(Camera *cam) {
  if (gApp.input.mouseDown && !ImGui::GetIO().WantCaptureMouse) {
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

Mat4 getProjMatrix(const Camera *camera) {
  Mat4 proj = mat4Perspective(camera->verticalFovDeg, camera->aspectRatio,
                              camera->nearZ, camera->farZ);
  return proj;
}