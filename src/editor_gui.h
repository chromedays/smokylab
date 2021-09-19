#pragma once
#include "util.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <SDL2/SDL.h>
#pragma clang diagnostic pop

FORWARD_DECL(Model);
FORWARD_DECL(Camera);

typedef struct _GUI {
  bool openMenu;

  Camera *cam;

  Model **models;
  int numModels;
} GUI;

C_INTERFACE_BEGIN

void initGUI(void);
void destroyGUI(void);
void handleGUIEvent(SDL_Event *event);
void updateGUI(GUI *gui);
void renderGUI(void);
bool guiWantsCaptureMouse(void);

C_INTERFACE_END
