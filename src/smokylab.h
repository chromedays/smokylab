#pragma once
#include "util.h"
#include "str.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <SDL2/SDL.h>
#include <d3d11_1.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>
#pragma clang diagnostic pop

C_INTERFACE_BEGIN

extern ID3D11Device *gDevice;
extern ID3D11DeviceContext *gContext;
extern IDXGISwapChain *gSwapChain;
extern ID3D11RenderTargetView *gSwapChainRTV;
extern ID3D11Texture2D *gSwapChainDepthStencilBuffer;
extern ID3D11DepthStencilView *gSwapChainDSV;

DXGI_RATIONAL queryRefreshRate(int ww, int wh, DXGI_FORMAT swapChainFormat);
HWND getWin32WindowHandle(SDL_Window *window);

typedef struct _ShaderProgram {
  ID3D11VertexShader *vert;
  ID3D11InputLayout *inputLayout;
  ID3D11PixelShader *frag;
} ShaderProgram;

void createProgram(const char *baseName, ShaderProgram *program);
void destroyProgram(ShaderProgram *program);
void useProgram(const ShaderProgram *program);

C_INTERFACE_END
