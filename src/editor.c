#include "editor.h"
#include "render.h"
#include "app.h"

void initEditor(Editor *editor) {
  initCameraLookingAtTarget(&editor->sceneCamera, float3(-5, 1, 0),
                            float3(-5, 1.105f, -1));
  initCameraLookingAtTarget(&editor->debugCamera, float3(10, 10, 10),
                            float3(0, 0, 0));

  createBuffer(
      &(BufferDesc){
          .size = sizeof(ViewUniforms),
          .usage = GPUResourceUsage_DEFAULT,
          .bindFlags = GPUResourceBindBits_CONSTANT_BUFFER,
      },
      &editor->viewBuffer);
  createBuffer(
      &(BufferDesc){
          .size = sizeof(MaterialUniforms),
          .usage = GPUResourceUsage_DEFAULT,
          .bindFlags = GPUResourceBindBits_CONSTANT_BUFFER,
      },
      &editor->materialBuffer);
  createBuffer(
      &(BufferDesc){
          .size = sizeof(DrawUniforms),
          .usage = GPUResourceUsage_DEFAULT,
          .bindFlags = GPUResourceBindBits_CONSTANT_BUFFER,
      },
      &editor->drawBuffer);
}

void destroyEditor(Editor *editor) {
  destroyBuffer(editor->drawBuffer);
  destroyBuffer(editor->materialBuffer);
  destroyBuffer(editor->viewBuffer);
}

void updateEditor(UNUSED Editor *editor) {}

void renderEditor(const Editor *editor) {
  // TODO: Render the main camera's frustum
  // TODO: Render the main camera's tile volumes

  ViewUniforms viewUniforms = {
      .viewMat = getViewMatrix(editor->focusedCamera),
      .projMat =
          mat4Perspective(60, getWindowAspectRatio(gApp.window), 0.1f, 500.f),
  };
  viewUniforms.viewPos.xyz = editor->focusedCamera->pos;
  updateBufferData(editor->viewBuffer, &viewUniforms);

  int windowWidth, windowHeight;
  SDL_GetWindowSize(gApp.window, &windowWidth, &windowHeight);
  setViewport(0, 0, (float)windowWidth, (float)windowHeight);

  if (editor->model) {
    setDefaultRenderStates(float4(0, 0, 0, 0));
    renderModel(editor->model, editor->drawBuffer, editor->materialBuffer);
  }
}