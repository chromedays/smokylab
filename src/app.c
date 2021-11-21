#include "app.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#ifdef _WIN32
#include <Windows.h>
#endif
#define CUTE_TIME_IMPLEMENTATION
#include <cute_time.h>
#pragma clang diagnostic pop

App gApp;

void initApp(const char *title, int windowWidth, int windowHeight) {
#ifdef _WIN32
  // SetProcessDPIAware();
#endif

  SDL_Init(SDL_INIT_VIDEO);
  gApp.window =
      SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       windowWidth, windowHeight, SDL_WINDOW_RESIZABLE);
  gApp.running = true;
  SDL_SetWindowResizable(gApp.window, SDL_FALSE);
}

void destroyApp(void) {
  SDL_DestroyWindow(gApp.window);

  SDL_Quit();
}

void pollAppEvent(void) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    // handleGUIEvent(&event);
    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
      gApp.input.mouseDown = (event.button.state == SDL_PRESSED);
      break;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      gApp.input.forwardDown =
          processKeyboardEvent(&event, SDLK_w, gApp.input.forwardDown);
      gApp.input.backDown =
          processKeyboardEvent(&event, SDLK_s, gApp.input.backDown);
      gApp.input.leftDown =
          processKeyboardEvent(&event, SDLK_a, gApp.input.leftDown);
      gApp.input.rightDown =
          processKeyboardEvent(&event, SDLK_d, gApp.input.rightDown);
      for (int i = 0; i < 10; ++i) {
        gApp.input.numKeysDown[i] =
            processKeyboardEvent(&event, SDLK_0 + i, gApp.input.numKeysDown[i]);
      }
      break;
    case SDL_QUIT:
      gApp.running = false;
      break;
    }
  }
}

void updateAppInput(void) {
  gApp.input.dt = ct_time();

  int ww, wh;
  SDL_GetWindowSize(gApp.window, &ww, &wh);

  int cursorX;
  int cursorY;
  SDL_GetMouseState(&cursorX, &cursorY);

  gApp.input.cursorDeltaX = 0;
  gApp.input.cursorDeltaY = 0;
  if (gApp.input.lastCursorX || gApp.input.lastCursorY) {
    gApp.input.cursorDeltaX = cursorX - gApp.input.lastCursorX;
    gApp.input.cursorDeltaY = cursorY - gApp.input.lastCursorY;
  }
  gApp.input.lastCursorX = cursorX;
  gApp.input.lastCursorY = cursorY;
}

float getWindowAspectRatio(SDL_Window *window) {
  int w, h;
  SDL_GetWindowSize(window, &w, &h);
  float a = (float)w / (float)h;
  return a;
}

bool processKeyboardEvent(const SDL_Event *event, SDL_Keycode keycode,
                          bool keyDown) {
  if ((event->type == SDL_KEYDOWN || event->type == SDL_KEYUP) &&
      event->key.keysym.sym == keycode) {
    keyDown = (event->key.state == SDL_PRESSED);
  }
  return keyDown;
}