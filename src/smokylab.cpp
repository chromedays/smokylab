#include "smokylab.h"
#include "util.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <SDL2/SDL_syswm.h>
#pragma clang diagnostic pop

DXGI_RATIONAL queryRefreshRate(int ww, int wh, DXGI_FORMAT swapChainFormat) {
  DXGI_RATIONAL refreshRate = {};

  IDXGIFactory *factory;
  HR_ASSERT(CreateDXGIFactory(IID_PPV_ARGS(&factory)));
  //   HR_ASSERT(CreateDXGIFactory(&IID_IDXGIFactory, (void **)&factory));

  IDXGIAdapter *adapter;
  HR_ASSERT(factory->EnumAdapters(0, &adapter));

  IDXGIOutput *adapterOutput;
  HR_ASSERT(adapter->EnumOutputs(0, &adapterOutput));

  UINT numDisplayModes;
  HR_ASSERT(adapterOutput->GetDisplayModeList(
      swapChainFormat, DXGI_ENUM_MODES_INTERLACED, &numDisplayModes, NULL));

  DXGI_MODE_DESC *displayModes =
      MMALLOC_ARRAY_UNINITIALIZED(DXGI_MODE_DESC, numDisplayModes);

  HR_ASSERT(adapterOutput->GetDisplayModeList(swapChainFormat,
                                              DXGI_ENUM_MODES_INTERLACED,
                                              &numDisplayModes, displayModes));

  for (UINT i = 0; i < numDisplayModes; ++i) {
    DXGI_MODE_DESC *mode = &displayModes[i];
    if ((int)mode->Width == ww && (int)mode->Height == wh) {
      refreshRate = mode->RefreshRate;
    }
  }

  MFREE(displayModes);
  COM_RELEASE(adapterOutput);
  COM_RELEASE(adapter);
  COM_RELEASE(factory);

  return refreshRate;
}

HWND getWin32WindowHandle(SDL_Window *window) {
  SDL_SysWMinfo wmInfo;
  SDL_VERSION(&wmInfo.version)
  SDL_GetWindowWMInfo(window, &wmInfo);
  HWND hwnd = wmInfo.info.win.window;
  return hwnd;
}
