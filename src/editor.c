#include "editor.h"

void initEditor(Editor *editor) {
  initCameraLookingAtTarget(&editor->sceneCamera, float3(-5, 1, 0),
                            float3(-5, 1.105f, -1));
  initCameraLookingAtTarget(&editor->debugCamera, float3(10, 10, 10),
                            float3(0, 0, 0));
}

void updateEditor(UNUSED Editor *editor) {}

void renderEditor(const Editor *editor) {
  // TODO: Render the main camera's frustum
  // TODO: Render the main camera's tile volumes
}