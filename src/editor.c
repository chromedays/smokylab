#include "editor.h"
#include "render.h"
#include "app.h"
#include "asset.h"

void initEditor(Editor *editor) {
  *editor = (Editor){};
  initCameraLookingAtTarget(&editor->sceneCamera, float3(-5, 1, 0),
                            float3(-5, 1.105f, -1));
  initCameraLookingAtTarget(&editor->debugCamera, float3(-5, 1, 0),
                            float3(-5, 1.105f, -1));

  editor->models[0] = MMALLOC(Model);
  loadGLTFModel("models/Sponza", editor->models[0]);
  editor->models[1] = MMALLOC(Model);
  loadGLTFModel("models/FlightHelmet", editor->models[1]);

  editor->focusedCamera = &editor->sceneCamera;
}

void destroyEditor(Editor *editor) {
  for (int i = 0; i < ARRAY_SIZE(editor->models); ++i) {
    destroyModel(editor->models[i]);
    MFREE(editor->models[i]);
  }
}

void updateEditor(Editor *editor) {
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

  setDefaultModelRenderStates(float4(0, 0, 0, 0));
  for (int i = 0; i < ARRAY_SIZE(editor->models); ++i) {
    if (editor->models[i]) {
      renderModel(editor->models[i]);
    }
  }

  renderCameraVolume(&editor->sceneCamera);
}