#pragma once
#include "util.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <SDL2/SDL.h>
#pragma clang diagnostic pop

typedef struct _GUI {
  float exposure;
} GUI;

C_INTERFACE_BEGIN

void initGUI(SDL_Window *window, SDL_GLContext glContext);
void destroyGUI(void);
void handleGUIEvent(SDL_Event *event);
void updateGUI(SDL_Window *window, GUI *gui);
void renderGUI(void);
bool guiWantsCaptureMouse(void);

C_INTERFACE_END
