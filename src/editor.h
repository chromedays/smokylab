#pragma once
#include "camera.h"

typedef struct _Editor {
  Camera mainCamera;
  Camera debugCamera;

  Camera *focusedCamera;
} Editor;

C_INTERFACE_BEGIN

void initEditor(Editor *editor);
void updateEditor(Editor *editor);

C_INTERFACE_END