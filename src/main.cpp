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
  SetProcessDPIAware();

  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window *window =
      SDL_CreateWindow("Smokylab", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_RESIZABLE);
  SDL_SetWindowResizable(window, SDL_FALSE);

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

  ID3D11Texture2D *renderedTexture;
  ID3D11ShaderResourceView *renderedView;
  ID3D11RenderTargetView *renderedRTV;
  ID3D11SamplerState *postProcessSampler;
  TextureDesc renderedTextureDesc = {
      .width = ww,
      .height = wh,
      .format = DXGI_FORMAT_R16G16B16A16_FLOAT,
      .usage = D3D11_USAGE_DEFAULT,
      .bindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
  };
  createTexture2D(&renderedTextureDesc, &renderedTexture, &renderedView);
  HR_ASSERT(
      gDevice->CreateRenderTargetView(renderedTexture, NULL, &renderedRTV));

  D3D11_SAMPLER_DESC postProcessSamplerDesc = {
      .Filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
      .AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
      .AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
      .AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
  };
  HR_ASSERT(gDevice->CreateSamplerState(&postProcessSamplerDesc,
                                        &postProcessSampler));

  ID3D11Texture2D *guiDisplayTexture;
  ID3D11ShaderResourceView *guiDisplayView;
  ID3D11RenderTargetView *guiDisplayRTV;
  TextureDesc guiDisplayTextureDesc = {
      .width = ww,
      .height = wh,
      .format = DXGI_FORMAT_R8G8B8A8_UNORM,
      .usage = D3D11_USAGE_DEFAULT,
      .bindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
  };
  createTexture2D(&guiDisplayTextureDesc, &guiDisplayTexture, &guiDisplayView);
  HR_ASSERT(
      gDevice->CreateRenderTargetView(guiDisplayTexture, NULL, &guiDisplayRTV));

  TextureDesc defaultTextureDesc = {
      .width = 16,
      .height = 16,
      .bytesPerPixel = 4,
      .format = DXGI_FORMAT_R8G8B8A8_UNORM,
      .usage = D3D11_USAGE_IMMUTABLE,
      .bindFlags = D3D11_BIND_SHADER_RESOURCE,
  };
  defaultTextureDesc.initialData = MMALLOC_ARRAY(
      uint32_t, defaultTextureDesc.width * defaultTextureDesc.height);
  for (int i = 0; i < defaultTextureDesc.width * defaultTextureDesc.height;
       ++i) {
    ((uint32_t *)defaultTextureDesc.initialData)[i] = 0xffffffff;
  }
  createTexture2D(&defaultTextureDesc, &gDefaultTexture, &gDefaultTextureView);
  MFREE(defaultTextureDesc.initialData);

  D3D11_SAMPLER_DESC defaultSamplerDesc = {
      .Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
      .AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
      .AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
      .AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
      .MaxLOD = D3D11_FLOAT32_MAX,
  };
  gDevice->CreateSamplerState(&defaultSamplerDesc, &gDefaultSampler);

  ID3D11DepthStencilState *depthStencilState;
  D3D11_DEPTH_STENCIL_DESC depthStencilStateDesc = {
      .DepthEnable = TRUE,
      .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
      .DepthFunc = D3D11_COMPARISON_GREATER_EQUAL,
      .StencilEnable = FALSE,
  };
  HR_ASSERT(gDevice->CreateDepthStencilState(&depthStencilStateDesc,
                                             &depthStencilState));

  ID3D11DepthStencilState *skyDepthStencilState;
  D3D11_DEPTH_STENCIL_DESC skyDepthStencilStateDesc = {
      .DepthEnable = TRUE,
      .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO,
      .DepthFunc = D3D11_COMPARISON_GREATER_EQUAL,
      .StencilEnable = FALSE,
  };
  HR_ASSERT(gDevice->CreateDepthStencilState(&skyDepthStencilStateDesc,
                                             &skyDepthStencilState));

  ID3D11DepthStencilState *postProcessDetphStencilState;
  D3D11_DEPTH_STENCIL_DESC postProcessDetphStencilStateDesc = {
      .DepthEnable = FALSE,
      .StencilEnable = FALSE,
  };
  HR_ASSERT(gDevice->CreateDepthStencilState(&postProcessDetphStencilStateDesc,
                                             &postProcessDetphStencilState));

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

  ShaderProgram brdfProgram = {};
  createProgram("brdf", &brdfProgram);

  ShaderProgram skyProgram = {};
  createProgram("sky", &skyProgram);

  ShaderProgram exposureProgram = {};
  createProgram("exposure", &exposureProgram);

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

  ID3D11Buffer *materialUniformBuffer;
  BufferDesc materialUniformBufferDesc = {
      .size = sizeof(MaterialUniforms),
      .usage = D3D11_USAGE_DEFAULT,
      .bindFlags = D3D11_BIND_CONSTANT_BUFFER,
  };
  createBuffer(&materialUniformBufferDesc, &materialUniformBuffer);

  Model model = {};
  // loadGLTFModel("../assets/models/EnvironmentTest", &model);
  //   loadGLTFModel("../assets/models/CesiumMilkTruck", &model);
  loadGLTFModel("../assets/models/Planes", &model);
  // loadGLTFModel("../assets/models/Sponza", &model);

  // clang-format off
  Float4 skyboxVertices[8] = {
      {-1, -1, -1},
      {-1, -1,  1},
      { 1, -1,  1},
      { 1, -1, -1},

      {-1,  1, -1},
      {-1,  1,  1},
      { 1,  1,  1},
      { 1,  1, -1},
  };

  uint32_t skyboxIndices[36] = {
      // Top
      4, 7, 6, 6, 5, 4,
      // Bottom
      1, 2, 3, 3, 0, 1,
      // Front
      7, 4, 0, 0, 3, 7,
      // Back
      5, 6, 2, 2, 1, 5,
      // Left
      4, 5, 1, 1, 0, 4,
      // Right
      6, 7, 3, 3, 2, 6,
  };
  // clang-format on
  ID3D11Buffer *skyVB, *skyIB;
  BufferDesc skyVBDesc = {
      .size = sizeof(skyboxVertices),
      .initialData = skyboxVertices,
      .usage = D3D11_USAGE_IMMUTABLE,
      .bindFlags = D3D11_BIND_VERTEX_BUFFER,
  };
  createBuffer(&skyVBDesc, &skyVB);
  BufferDesc skyIBDesc = {
      .size = sizeof(skyboxIndices),
      .initialData = skyboxIndices,
      .usage = D3D11_USAGE_IMMUTABLE,
      .bindFlags = D3D11_BIND_INDEX_BUFFER,
  };
  createBuffer(&skyIBDesc, &skyIB);

  int skyWidth, skyHeight;
  ID3D11Texture2D *skyTex, *irrTex;
  ID3D11ShaderResourceView *skyView, *irrView;
  createIBLTexture("Newport_Loft", &skyWidth, &skyHeight, &skyTex, &irrTex,
                   &skyView, &irrView);

  FreeLookCamera cam = {
      .pos = {-1.753f, 3.935f, 7.364f},
      .yaw = -176.4f,
      .pitch = 1.6f,
  };

  initGUI(window, gDevice, gContext);
  GUI gui = {
      .renderedView = guiDisplayView,
      .exposure = 1,
      .model = &model,
  };

  ViewUniforms viewUniforms = {
      .skySize = {skyWidth, skyHeight},
      .exposure = {1},
  };
  generateHammersleySequence(NUM_SAMPLES, viewUniforms.randomPoints);

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
    viewUniforms.exposure.x = gui.exposure;

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

    viewUniforms.viewPos.xyz = cam.pos;
    viewUniforms.viewMat = getViewMatrix(&cam);
    viewUniforms.projMat =
        mat4Perspective(60, (float)ww / (float)wh, 0.1f, 500);

    gContext->UpdateSubresource(viewUniformBuffer, 0, NULL, &viewUniforms, 0,
                                0);

    DrawUniforms drawUniforms = {
        .modelMat = mat4Identity(),
        .invModelMat = mat4Identity(),
    };
    gContext->UpdateSubresource(drawUniformBuffer, 0, NULL, &drawUniforms, 0,
                                0);

    ID3D11Buffer *uniformBuffers[] = {viewUniformBuffer, drawUniformBuffer,
                                      materialUniformBuffer};
    gContext->VSSetConstantBuffers(0, ARRAY_SIZE(uniformBuffers),
                                   uniformBuffers);
    gContext->PSSetConstantBuffers(0, ARRAY_SIZE(uniformBuffers),
                                   uniformBuffers);

    D3D11_VIEWPORT viewport = {
        .TopLeftX = 0,
        .TopLeftY = 0,
        .Width = (float)ww,
        .Height = (float)wh,
        .MinDepth = 0,
        .MaxDepth = 1,
    };
    gContext->RSSetViewports(1, &viewport);

    gContext->OMSetRenderTargets(1, &renderedRTV, gSwapChainDSV);
    FLOAT clearColor[4] = {0.1f, 0.1f, 0.1f, 1};
    gContext->ClearRenderTargetView(renderedRTV, clearColor);
    gContext->ClearDepthStencilView(gSwapChainDSV, D3D11_CLEAR_DEPTH, 0, 0);

    gContext->RSSetState(rasterizerState);
    gContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ID3D11ShaderResourceView *skyResources[] = {skyView, irrView};
    ID3D11SamplerState *skySamplers[] = {gDefaultSampler, gDefaultSampler};
    gContext->PSSetShaderResources(0, ARRAY_SIZE(skyResources), skyResources);
    gContext->PSSetSamplers(0, ARRAY_SIZE(skySamplers), skySamplers);

    // Render sky
    gContext->OMSetDepthStencilState(skyDepthStencilState, 0);
    useProgram(&skyProgram);
    UINT stride = sizeof(Float4), offset = 0;
    gContext->IASetVertexBuffers(0, 1, &skyVB, &stride, &offset);
    gContext->IASetIndexBuffer(skyIB, DXGI_FORMAT_R32_UINT, 0);
    gContext->DrawIndexed(ARRAY_SIZE(skyboxIndices), 0, 0);

    gContext->OMSetDepthStencilState(depthStencilState, 0);
    useProgram(&brdfProgram);

    renderModel(&model, drawUniformBuffer, materialUniformBuffer);

    // Post processing
    {
      ID3D11RenderTargetView *renderTargets[] = {
          gSwapChainRTV,
          guiDisplayRTV,
      };
      gContext->OMSetRenderTargets(castI32U32(ARRAY_SIZE(renderTargets)),
                                   renderTargets, NULL);
      gContext->OMSetDepthStencilState(postProcessDetphStencilState, 0);
      useProgram(&exposureProgram);
      gContext->PSSetShaderResources(0, 1, &renderedView);
      gContext->PSSetSamplers(0, 1, &postProcessSampler);
      gContext->Draw(3, 0);
      ID3D11ShaderResourceView *nullResource = NULL;
      ID3D11SamplerState *nullSampler = NULL;
      gContext->PSSetShaderResources(0, 1, &nullResource);
      gContext->PSSetSamplers(0, 1, &nullSampler);

      renderTargets[0] = gSwapChainRTV;
      renderTargets[1] = NULL;
      gContext->OMSetRenderTargets(castI32U32(ARRAY_SIZE(renderTargets)),
                                   renderTargets, NULL);
      // gContext->ClearRenderTargetView(gSwapChainRTV, clearColor);
    }

    renderGUI();

    gSwapChain->Present(1, 0);
  }

  destroyGUI();

  COM_RELEASE(irrView);
  COM_RELEASE(skyView);
  COM_RELEASE(irrTex);
  COM_RELEASE(skyTex);

  COM_RELEASE(skyIB);
  COM_RELEASE(skyVB);

  destroyModel(&model);

  COM_RELEASE(materialUniformBuffer);
  COM_RELEASE(drawUniformBuffer);
  COM_RELEASE(viewUniformBuffer);

  destroyProgram(&exposureProgram);
  destroyProgram(&skyProgram);
  destroyProgram(&brdfProgram);

  COM_RELEASE(rasterizerState);
  COM_RELEASE(postProcessDetphStencilState);
  COM_RELEASE(skyDepthStencilState);
  COM_RELEASE(depthStencilState);

  COM_RELEASE(gDefaultSampler);
  COM_RELEASE(gDefaultTextureView);
  COM_RELEASE(gDefaultTexture);

  COM_RELEASE(guiDisplayRTV);
  COM_RELEASE(guiDisplayView);
  COM_RELEASE(guiDisplayTexture);

  COM_RELEASE(postProcessSampler);
  COM_RELEASE(renderedRTV);
  COM_RELEASE(renderedView);
  COM_RELEASE(renderedTexture);

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
