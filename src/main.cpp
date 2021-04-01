#include "smokylab.h"
#include "util.h"
#include "gui.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define CUTE_TIME_IMPLEMENTATION
#include <cute_time.h>
#include <SDL2/SDL.h>
#include <d3d11_1.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>
#pragma clang diagnostic pop

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

  ID3D11DepthStencilState *depthStencilState;
  D3D11_DEPTH_STENCIL_DESC depthStencilStateDesc = {
      .DepthEnable = TRUE,
      .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
      .DepthFunc = D3D11_COMPARISON_GREATER_EQUAL,
      .StencilEnable = FALSE,
  };
  HR_ASSERT(gDevice->CreateDepthStencilState(&depthStencilStateDesc,
                                             &depthStencilState));

  ID3D11RasterizerState *rasterizerState;
  D3D11_RASTERIZER_DESC rasterizerStateDesc = {
      .FillMode = D3D11_FILL_SOLID,
      .CullMode = D3D11_CULL_BACK,
      .FrontCounterClockwise = TRUE,
      .DepthBias = 0,
      .DepthBiasClamp = 0.f,
      .SlopeScaledDepthBias = 0.f,
      .DepthClipEnable = TRUE,
      .ScissorEnable = FALSE,
      .MultisampleEnable = FALSE,
      .AntialiasedLineEnable = FALSE,
  };
  HR_ASSERT(
      gDevice->CreateRasterizerState(&rasterizerStateDesc, &rasterizerState));

  ShaderProgram fstProgram = {};
  createProgram("fst", &fstProgram);

  ShaderProgram brdfProgram = {};
  createProgram("brdf", &brdfProgram);

  ID3D11Buffer *viewUniformBuffer;
  BufferDesc viewUniformBufferDesc = {
      .size = sizeof(ViewUniforms),
      .usage = D3D11_USAGE_DEFAULT,
      .bindFlags = D3D11_BIND_CONSTANT_BUFFER,
  };
  createBuffer(&viewUniformBufferDesc, &viewUniformBuffer);

  ID3D11Buffer *drawUniformBuffer;
  BufferDesc drawUniformBufferDesc = {
      .size = sizeof(DrawUniforms),
      .usage = D3D11_USAGE_DEFAULT,
      .bindFlags = D3D11_BIND_CONSTANT_BUFFER,
  };
  createBuffer(&drawUniformBufferDesc, &drawUniformBuffer);

  Model model = {};
  loadGLTFModel("../assets/models/CesiumMilkTruck", &model);

  FreeLookCamera cam = {};

  initGUI(window, gDevice, gContext);
  GUI gui = {};

  LOG("Entering main loop.");

  int lastCursorX = 0;
  int lastCursorY = 0;
  bool leftDown = false;
  bool rightDown = false;
  bool forwardDown = false;
  bool backDown = false;
  bool mouseDown = false;
  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      handleGUIEvent(&event);
      switch (event.type) {
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
        mouseDown = (event.button.state == SDL_PRESSED);
        break;
      case SDL_KEYDOWN:
      case SDL_KEYUP:
        forwardDown = processKeyboardEvent(&event, SDLK_w, forwardDown);
        backDown = processKeyboardEvent(&event, SDLK_s, backDown);
        leftDown = processKeyboardEvent(&event, SDLK_a, leftDown);
        rightDown = processKeyboardEvent(&event, SDLK_d, rightDown);
        break;
      case SDL_QUIT:
        running = false;
        break;
      }
    }

    updateGUI(window, &gui);

    float dt = ct_time();

    SDL_GetWindowSize(window, &ww, &wh);

    int cursorX;
    int cursorY;
    SDL_GetMouseState(&cursorX, &cursorY);

    int cursorDeltaX = 0;
    int cursorDeltaY = 0;
    if (lastCursorX || lastCursorY) {
      cursorDeltaX = cursorX - lastCursorX;
      cursorDeltaY = cursorY - lastCursorY;
    }
    lastCursorX = cursorX;
    lastCursorY = cursorY;

    if (mouseDown && !guiWantsCaptureMouse()) {
      cam.yaw += (float)cursorDeltaX * 0.6f;
      cam.pitch -= (float)cursorDeltaY * 0.6f;
      cam.pitch = CLAMP(cam.pitch, -88.f, 88.f);
    }

    int dx = 0, dy = 0;
    if (leftDown) {
      dx -= 1;
    }
    if (rightDown) {
      dx += 1;
    }
    if (forwardDown) {
      dy += 1;
    }
    if (backDown) {
      dy -= 1;
    }

    float camMovementSpeed = 4.f;
    Float3 camMovement = (getRight(&cam) * (float)dx * camMovementSpeed +
                          getLook(&cam) * (float)dy * camMovementSpeed) *
                         dt;
    cam.pos += camMovement;

    ViewUniforms viewUniforms = {
        .viewMat = getViewMatrix(&cam),
        .projMat = mat4Perspective(75, (float)ww / (float)wh, 0.1f, 500),
    };
    gContext->UpdateSubresource(viewUniformBuffer, 0, NULL, &viewUniforms, 0,
                                0);

    DrawUniforms drawUniforms = {
        .modelMat = mat4Identity(),
        .invModelMat = mat4Identity(),
    };
    gContext->UpdateSubresource(drawUniformBuffer, 0, NULL, &drawUniforms, 0,
                                0);

    D3D11_VIEWPORT viewport = {
        .TopLeftX = 0,
        .TopLeftY = 0,
        .Width = (float)ww,
        .Height = (float)wh,
        .MinDepth = 0,
        .MaxDepth = 1,
    };
    gContext->RSSetViewports(1, &viewport);

    gContext->OMSetRenderTargets(1, &gSwapChainRTV, gSwapChainDSV);
    FLOAT clearColor[4] = {0, 0, 0, 1};
    gContext->ClearRenderTargetView(gSwapChainRTV, clearColor);
    gContext->ClearDepthStencilView(gSwapChainDSV, D3D11_CLEAR_DEPTH, 0, 0);

    gContext->OMSetDepthStencilState(depthStencilState, 0);
    gContext->RSSetState(rasterizerState);
    gContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    useProgram(&brdfProgram);
    ID3D11Buffer *uniformBuffers[] = {viewUniformBuffer, drawUniformBuffer};
    gContext->VSSetConstantBuffers(0, ARRAY_SIZE(uniformBuffers),
                                   uniformBuffers);
    gContext->PSSetConstantBuffers(0, ARRAY_SIZE(uniformBuffers),
                                   uniformBuffers);

    renderModel(&model, drawUniformBuffer);

    renderGUI();

    gSwapChain->Present(1, 0);
  }

  destroyGUI();

  destroyModel(&model);

  COM_RELEASE(drawUniformBuffer);
  COM_RELEASE(viewUniformBuffer);

  destroyProgram(&brdfProgram);
  destroyProgram(&fstProgram);

  COM_RELEASE(rasterizerState);
  COM_RELEASE(depthStencilState);
  COM_RELEASE(gSwapChainDSV);
  COM_RELEASE(gSwapChainDepthStencilBuffer);
  COM_RELEASE(gSwapChainRTV);
  COM_RELEASE(gSwapChain);
  COM_RELEASE(gContext);
  COM_RELEASE(gDevice);

#ifdef DEBUG
  {
    IDXGIDebug *debug;
    HMODULE dxgidebugLibrary = LoadLibraryA("dxgidebug.dll");

    typedef HRESULT(WINAPI * DXGIGetDebugInterfaceFn)(REFIID riid,
                                                      void **ppDebug);
    DXGIGetDebugInterfaceFn DXGIGetDebugInterface =
        (DXGIGetDebugInterfaceFn)GetProcAddress(dxgidebugLibrary,
                                                "DXGIGetDebugInterface");
    DXGIGetDebugInterface(IID_PPV_ARGS(&debug));
    HR_ASSERT(debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL));

    FreeLibrary(dxgidebugLibrary);
    COM_RELEASE(debug);
  }
#endif

  SDL_DestroyWindow(window);

  SDL_Quit();
  return 0;
}
