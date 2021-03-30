#include "smokylab.h"
#include "util.h"
#include <SDL2/SDL.h>

int main(UNUSED int argc, UNUSED char **argv) {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window *window =
      SDL_CreateWindow("Smokylab", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_RESIZABLE);

  LOG("Entering main loop.");

  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
        running = false;
        break;
      }
    }
  }

  SDL_DestroyWindow(window);

  SDL_Quit();
  return 0;
}