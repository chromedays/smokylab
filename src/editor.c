#include "editor.h"

void initEditor(Editor *editor) {
  editor->mainCamera = (Camera){
      .pos = {-5, 1, 0},
      .yaw = -90,
      .pitch = 6,
  };

  editor->debugCamera = (Camera){
      .pos = {-5, 1, 0},
      .yaw = -90,
      .pitch = 6,
  };
}

void updateEditor(Editor *editor) {}