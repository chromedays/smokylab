#include "editor.h"
#include "render.h"
#include "app.h"
#include "asset.h"

#define DEFAULT_MODEL "models/Sponza"

void initEditor(Editor *editor) {
  *editor = (Editor){};
  initCameraLookingAtTarget(&editor->sceneCamera, float3(-5, 1, 0),
                            float3(-5, 1.105f, -1));
  initCameraLookingAtTarget(&editor->debugCamera, float3(-5, 1, 0),
                            float3(-5, 1.105f, -1));

  editor->model = MMALLOC(Model);
  loadGLTFModel(DEFAULT_MODEL, editor->model);

  editor->focusedCamera = &editor->sceneCamera;
}

void destroyEditor(Editor *editor) {
  destroyModel(editor->model);
  MFREE(editor->model);
}

void updateEditor(UNUSED Editor *editor) {
  if (gApp.input.numKeysDown[1]) {
    editor->focusedCamera = &editor->sceneCamera;
  }
  if (gApp.input.numKeysDown[2]) {
    editor->focusedCamera = &editor->debugCamera;
  }
  moveCameraByInputs(editor->focusedCamera);
}

void renderEditor(const Editor *editor) {
  // TODO: Render the main camera's frustum
  // TODO: Render the main camera's tile volumes

  setCamera(editor->focusedCamera);
  setViewportByAppWindow();

  if (editor->model) {
    setDefaultModelRenderStates(float4(0, 0, 0, 0));
    renderModel(editor->model);
  }

  renderCameraVolume(&editor->sceneCamera);
}