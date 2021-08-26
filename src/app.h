#pragma once
#include "util.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <SDL2/SDL.h>
#pragma clang diagnostic pop

typedef struct _App {
  SDL_Window *window;
  bool running;

  struct {
    float dt;
    int lastCursorX;
    int lastCursorY;
    bool leftDown;
    bool rightDown;
    bool forwardDown;
    bool backDown;
    bool mouseDown;
    bool numKeysDown[10];
    int cursorDeltaX;
    int cursorDeltaY;
  } input;
} App;

C_INTERFACE_BEGIN

extern App gApp;

void initApp(const char *title, int windowWidth, int windowHeight);
void destroyApp(void);
void pollAppEvent(void);
void updateAppInput(void);

float getWindowAspectRatio(SDL_Window *window);
bool processKeyboardEvent(const SDL_Event *event, SDL_Keycode keycode,
                          bool keyDown);

C_INTERFACE_END