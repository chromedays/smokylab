#pragma once
#include "camera.h"

FORWARD_DECL(Model);
FORWARD_DECL(GPUBuffer);

typedef struct _Editor {
  Camera sceneCamera;
  Camera debugCamera;
  Camera *focusedCamera;

  Model *model;

  GPUBuffer *viewBuffer;
  GPUBuffer *materialBuffer;
  GPUBuffer *drawBuffer;

} Editor;

C_INTERFACE_BEGIN

void initEditor(Editor *editor);
void destroyEditor(Editor *editor);
void updateEditor(Editor *editor);
void renderEditor(const Editor *editor);

C_INTERFACE_END