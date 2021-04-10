#pragma once
#include "util.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <SDL2/SDL.h>
#include <d3d11_1.h>
#pragma clang diagnostic pop

typedef struct _GUI {
  ID3D11ShaderResourceView *renderedView;
  float exposure;

  bool isHoveringScene;
} GUI;

C_INTERFACE_BEGIN

void initGUI(SDL_Window *window, ID3D11Device *device,
             ID3D11DeviceContext *deviceContext);
void destroyGUI(void);
void handleGUIEvent(SDL_Event *event);
void updateGUI(SDL_Window *window, GUI *gui);
void renderGUI(void);
bool guiWantsCaptureMouse(void);

C_INTERFACE_END
