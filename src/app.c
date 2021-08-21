#include "app.h"
#ifdef _WIN32
#include <Windows.h>
#endif

App gApp;

void initApp(const char *title, int windowWidth, int windowHeight) {
#ifdef _WIN32
  SetProcessDPIAware();
#endif

  SDL_Init(SDL_INIT_VIDEO);
  gApp.window =
      SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       windowWidth, windowHeight, SDL_WINDOW_RESIZABLE);
  SDL_SetWindowResizable(gApp.window, SDL_FALSE);
}

void destroyApp(void) {
  SDL_DestroyWindow(gApp.window);

  SDL_Quit();
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