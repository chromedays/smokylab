#include "smokylab.h"
#include "util.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <SDL2/SDL.h>
#include <d3d11_1.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>
#pragma clang diagnostic pop

static ID3D11Device *gDevice;
static ID3D11DeviceContext *gContext;
static IDXGISwapChain *gSwapChain;
static ID3D11RenderTargetView *gSwapChainRTV;
static ID3D11Texture2D *gSwapChainDepthStencilBuffer;
static ID3D11DepthStencilView *gSwapChainDSV;

int main(UNUSED int argc, UNUSED char **argv) {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window *window =
      SDL_CreateWindow("Smokylab", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_RESIZABLE);

  int ww, wh;
  SDL_GetWindowSize(window, &ww, &wh);
  DXGI_SWAP_CHAIN_DESC swapChainDesc = {
      .BufferDesc =
          {
              .Width = castI32U32(ww),
              .Height = castI32U32(wh),
              .RefreshRate =
                  queryRefreshRate(ww, wh, DXGI_FORMAT_R8G8B8A8_UNORM),
              .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
          },
      .SampleDesc =
          {
              .Count = 1,
              .Quality = 0,
          },
      .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
      .BufferCount = 2,
      .OutputWindow = getWin32WindowHandle(window),
      .Windowed = TRUE,
      .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
  };

  UINT createDeviceFlags
#ifdef DEBUG
      = D3D11_CREATE_DEVICE_DEBUG;
#else
      = 0;
#endif
  D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;
  HR_ASSERT(D3D11CreateDeviceAndSwapChain(
      NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, &featureLevel, 1,
      D3D11_SDK_VERSION, &swapChainDesc, &gSwapChain, &gDevice, NULL,
      &gContext));

  ID3D11Texture2D *backBuffer;
  HR_ASSERT(gSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));
  HR_ASSERT(gDevice->CreateRenderTargetView(backBuffer, NULL, &gSwapChainRTV));
  COM_RELEASE(backBuffer);

  D3D11_TEXTURE2D_DESC depthStencilBufferDesc = {
      .Width = castI32U32(ww),
      .Height = castI32U32(wh),
      .MipLevels = 1,
      .ArraySize = 1,
      .Format = DXGI_FORMAT_D32_FLOAT,
      .SampleDesc =
          {
              .Count = 1,
              .Quality = 0,
          },
      .Usage = D3D11_USAGE_DEFAULT,
      .BindFlags = D3D11_BIND_DEPTH_STENCIL,
      .CPUAccessFlags = 0,
  };
  HR_ASSERT(gDevice->CreateTexture2D(&depthStencilBufferDesc, NULL,
                                     &gSwapChainDepthStencilBuffer));
  HR_ASSERT(gDevice->CreateDepthStencilView(gSwapChainDepthStencilBuffer, NULL,
                                            &gSwapChainDSV));

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

    gSwapChain->Present(1, 0);
  }

  COM_RELEASE(gSwapChainDSV);
  COM_RELEASE(gSwapChainDepthStencilBuffer);
  COM_RELEASE(gSwapChainRTV);
  COM_RELEASE(gSwapChain);
  COM_RELEASE(gContext);
  COM_RELEASE(gDevice);

  SDL_DestroyWindow(window);

  SDL_Quit();
  return 0;
}
