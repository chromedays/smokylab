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