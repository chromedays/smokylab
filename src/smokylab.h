#pragma once
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <SDL2/SDL.h>
#include <d3d11_1.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>
#pragma clang diagnostic pop

DXGI_RATIONAL queryRefreshRate(int ww, int wh, DXGI_FORMAT swapChainFormat);
HWND getWin32WindowHandle(SDL_Window *window);
