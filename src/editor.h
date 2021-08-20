#pragma once
#include "camera.h"

FORWARD_DECL(Model);

typedef struct _Editor {
  Camera sceneCamera;
  Camera debugCamera;

  Model *model;

  Camera *focusedCamera;
} Editor;

C_INTERFACE_BEGIN

void initEditor(Editor *editor);
void updateEditor(Editor *editor);
void renderEditor(const Editor *editor);

C_INTERFACE_END