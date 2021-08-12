#include "smokylab.h"
#include "util.h"
#include "gui.h"
#include "asset.h"
#include "camera.h"
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
  // Initialize asset paths
  {
    String assetConfigPath = {};
    copyBasePath(&assetConfigPath);
    appendPathCStr(&assetConfigPath, "asset_config.txt");
    FILE *assetConfigFile = fopen(assetConfigPath.buf, "r");
    destroyString(&assetConfigPath);
    ASSERT(assetConfigFile);
    char assetBasePath[100] = {};
    char shaderBasePath[100] = {};
    fgets(assetBasePath, 100, assetConfigFile);
    for (int i = 0; i < 100; ++i) {
      if (assetBasePath[i] == '\n') {
        assetBasePath[i] = 0;
        break;
      }
    }
    fgets(shaderBasePath, 100, assetConfigFile);
    for (int i = 0; i < 100; ++i) {
      if (shaderBasePath[i] == '\n') {
        shaderBasePath[i] = 0;
        break;
      }
    }
    LOG("Asset base path: %s", assetBasePath);
    LOG("Shader base path: %s", shaderBasePath);

    fclose(assetConfigFile);
    initAssetLoader(assetBasePath, shaderBasePath);
  }

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

  TextureDesc depthStencilTextureDesc = {
      .width = ww,
      .height = wh,
      .format = DXGI_FORMAT_R32_TYPELESS,
      .viewFormat = DXGI_FORMAT_R32_FLOAT,
      .usage = D3D11_USAGE_DEFAULT,
      .bindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE,
  };
  ID3D11ShaderResourceView *depthTextureView;
  createTexture2D(&depthStencilTextureDesc, &gSwapChainDepthStencilBuffer,
                  &depthTextureView);
  D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {
      .Format = DXGI_FORMAT_D32_FLOAT,
      .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
      .Flags = 0,
      .Texture2D = {.MipSlice = 0},
  };
  HR_ASSERT(gDevice->CreateDepthStencilView(
      gSwapChainDepthStencilBuffer, &depthStencilViewDesc, &gSwapChainDSV));

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

  ID3D11Texture2D *oitAccumTexture;
  ID3D11ShaderResourceView *oitAccumView;
  ID3D11RenderTargetView *oitAccumRTV;
  TextureDesc oitAccumTextureDesc = {
      .width = ww,
      .height = wh,
      .format = DXGI_FORMAT_R16G16B16A16_FLOAT,
      .usage = D3D11_USAGE_DEFAULT,
      .bindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
  };
  createTexture2D(&oitAccumTextureDesc, &oitAccumTexture, &oitAccumView);
  HR_ASSERT(
      gDevice->CreateRenderTargetView(oitAccumTexture, NULL, &oitAccumRTV));

  ID3D11Texture2D *oitRevealTexture;
  ID3D11ShaderResourceView *oitRevealView;
  ID3D11RenderTargetView *oitRevealRTV;
  TextureDesc oitRevealTextureDesc = {
      .width = ww,
      .height = wh,
      .format = DXGI_FORMAT_R32_FLOAT,
      .usage = D3D11_USAGE_DEFAULT,
      .bindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
  };
  createTexture2D(&oitRevealTextureDesc, &oitRevealTexture, &oitRevealView);
  HR_ASSERT(
      gDevice->CreateRenderTargetView(oitRevealTexture, NULL, &oitRevealRTV));

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

  ID3D11DepthStencilState *noDepthStencilState;
  D3D11_DEPTH_STENCIL_DESC noDepthStencilStateDesc = {
      .DepthEnable = FALSE,
      .StencilEnable = FALSE,
  };
  HR_ASSERT(gDevice->CreateDepthStencilState(&noDepthStencilStateDesc,
                                             &noDepthStencilState));

  ID3D11DepthStencilState *oitAccumDepthStencilState;
  D3D11_DEPTH_STENCIL_DESC oitAccumDepthStencilStateDesc = {
      .DepthEnable = TRUE,
      .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO,
      .DepthFunc = D3D11_COMPARISON_GREATER_EQUAL,
      .StencilEnable = FALSE,
  };
  HR_ASSERT(gDevice->CreateDepthStencilState(&oitAccumDepthStencilStateDesc,
                                             &oitAccumDepthStencilState));

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

  ID3D11RasterizerState *wireframeRasterizerState;
  D3D11_RASTERIZER_DESC wireframeRasterizerStateDesc = {
      .FillMode = D3D11_FILL_WIREFRAME,
      .CullMode = D3D11_CULL_FRONT,
      .FrontCounterClockwise = TRUE,
      .DepthBias = 0,
      .DepthBiasClamp = 0.f,
      .SlopeScaledDepthBias = 0.f,
      .DepthClipEnable = TRUE,
      .ScissorEnable = FALSE,
      .MultisampleEnable = FALSE,
      .AntialiasedLineEnable = FALSE,
  };
  HR_ASSERT(gDevice->CreateRasterizerState(&wireframeRasterizerStateDesc,
                                           &wireframeRasterizerState));

  // Sorted blend state for reference
  ID3D11BlendState *refBlendState;
  D3D11_BLEND_DESC refBlendDesc = {
      .AlphaToCoverageEnable = FALSE,
      .IndependentBlendEnable = FALSE,
  };
  refBlendDesc.RenderTarget[0] = {
      .BlendEnable = TRUE,
      .SrcBlend = D3D11_BLEND_SRC_ALPHA,
      .DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
      .BlendOp = D3D11_BLEND_OP_ADD,
      .SrcBlendAlpha = D3D11_BLEND_ONE,
      .DestBlendAlpha = D3D11_BLEND_ZERO,
      .BlendOpAlpha = D3D11_BLEND_OP_ADD,
      .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
  };
  HR_ASSERT(gDevice->CreateBlendState(&refBlendDesc, &refBlendState));

  ID3D11BlendState *oitAccumBlendState;
  D3D11_BLEND_DESC oitAccumBlendDesc = {
      .AlphaToCoverageEnable = FALSE,
      .IndependentBlendEnable = TRUE,
  };
  oitAccumBlendDesc.RenderTarget[0] = {
      .BlendEnable = TRUE,
      .SrcBlend = D3D11_BLEND_ONE,
      .DestBlend = D3D11_BLEND_ONE,
      .BlendOp = D3D11_BLEND_OP_ADD,
      .SrcBlendAlpha = D3D11_BLEND_ONE,
      .DestBlendAlpha = D3D11_BLEND_ONE,
      .BlendOpAlpha = D3D11_BLEND_OP_ADD,
      .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
  };
  oitAccumBlendDesc.RenderTarget[1] = {
      .BlendEnable = TRUE,
      .SrcBlend = D3D11_BLEND_ZERO,
      .DestBlend = D3D11_BLEND_INV_SRC_COLOR,
      .BlendOp = D3D11_BLEND_OP_ADD,
      .SrcBlendAlpha = D3D11_BLEND_ZERO,
      .DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA,
      .BlendOpAlpha = D3D11_BLEND_OP_ADD,
      .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
  };
  HR_ASSERT(gDevice->CreateBlendState(&oitAccumBlendDesc, &oitAccumBlendState));

  ID3D11BlendState *oitCompositeBlendState;
  D3D11_BLEND_DESC oitCompositeBlendDesc = {
      .AlphaToCoverageEnable = FALSE,
      .IndependentBlendEnable = FALSE,
  };
  oitCompositeBlendDesc.RenderTarget[0] = {
      .BlendEnable = TRUE,
      .SrcBlend = D3D11_BLEND_INV_SRC_ALPHA,
      .DestBlend = D3D11_BLEND_SRC_ALPHA,
      .BlendOp = D3D11_BLEND_OP_ADD,
      .SrcBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA,
      .DestBlendAlpha = D3D11_BLEND_SRC_ALPHA,
      .BlendOpAlpha = D3D11_BLEND_OP_ADD,
      .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
  };
  HR_ASSERT(gDevice->CreateBlendState(&oitCompositeBlendDesc,
                                      &oitCompositeBlendState));

  // createSSAOResources(ww, wh);

  ShaderProgram forwardPBRProgram = {};
  loadProgram("forward_pbr", &forwardPBRProgram);

  // ShaderProgram brdfProgram = {};
  // loadProgram("new_brdf", &brdfProgram);

  // ShaderProgram skyProgram = {};
  // loadProgram("sky", &skyProgram);

  // ShaderProgram exposureProgram = {};
  // loadProgram("exposure", &exposureProgram);

  // ShaderProgram wireframeProgram = {};
  // loadProgram("wireframe", &wireframeProgram);

  // ShaderProgram fstProgram = {};
  // loadProgram("fst", &fstProgram);

  // ShaderProgram oitAccumProgram = {};
  // loadProgram("oit_accum", &oitAccumProgram);

  // ShaderProgram oitCompositeProgram = {};
  // loadProgram("oit_composite", &oitCompositeProgram);

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

  int numModels = 0;
  Model *models[10] = {};
  {
    Model **model;
    model = &models[numModels++];
    *model = MMALLOC(Model);
    loadGLTFModel("models/Sponza", *model);
    model = &models[numModels++];
    *model = MMALLOC(Model);
    loadGLTFModel("models/FlightHelmet", *model);
    model = &models[numModels++];
    *model = MMALLOC(Model);
    loadGLTFModel("models/Planes", *model);
  }

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
  loadIBLTexture("ibl/Newport_Loft", &skyWidth, &skyHeight, &skyTex, &irrTex,
                 &skyView, &irrView);

  Camera cam;
  initCameraLookingAtTarget(&cam, float3(-5, 1, 0), float3(-5, 1.105f, -1));

  initGUI(window, gDevice, gContext);
  GUI gui = {
      .exposure = 1,
      .depthVisualizedRangeFar = 15,
      .lightAngle = 30,
      .lightIntensity = 10,
      .globalOpacity = 0.4f,
      .ssaoNumSamples = 10,
      .ssaoRadius = 0.5f,
      .ssaoScaleFactor = 1,
      .ssaoContrastFactor = 1,
      .cam = &cam,
      .models = models,
      .numModels = numModels,
  };

  float nearZ = 0.1f;
  float farZ = 500.f;

  ViewUniforms viewUniforms = {
      .skySize = {skyWidth, skyHeight},
      .exposureNearFar = {1, nearZ, gui.depthVisualizedRangeFar},
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
    viewUniforms.exposureNearFar.xz =
        float2(gui.exposure, gui.depthVisualizedRangeFar);
    viewUniforms.dirLightDirIntensity.xyz = float3Normalize(float3(
        cosf(degToRad(gui.lightAngle)), -1, sinf(degToRad(gui.lightAngle))));
    viewUniforms.dirLightDirIntensity.w = gui.lightIntensity;
    viewUniforms.overrideOpacity.x = gui.overrideOpacity ? 1 : 0;
    viewUniforms.overrideOpacity.y = gui.globalOpacity;
    viewUniforms.ssaoFactors = {(float)gui.ssaoNumSamples, gui.ssaoRadius,
                                gui.ssaoScaleFactor, gui.ssaoContrastFactor};

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
        mat4Perspective(60, (float)ww / (float)wh, nearZ, farZ);

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

    {
      gContext->OMSetRenderTargets(1, &gSwapChainRTV, gSwapChainDSV);
      FLOAT clearColor[4] = {};
      gContext->ClearRenderTargetView(gSwapChainRTV, clearColor);
      gContext->ClearDepthStencilView(gSwapChainDSV, D3D11_CLEAR_DEPTH, 0, 0);

      gContext->RSSetState(rasterizerState);
      gContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      gContext->OMSetDepthStencilState(depthStencilState, 0);
      gContext->OMSetBlendState(NULL, NULL, 0xFFFFFFFF);
      useProgram(&forwardPBRProgram);

      for (int i = 0; i < numModels; ++i) {
        renderModel(models[i], drawUniformBuffer, materialUniformBuffer);
      }
    }

#if 0
    {
      ID3D11RenderTargetView *rts[] = {
          renderedRTV,
          gPositionRTV,
          gNormalRTV,
          gAlbedoRTV,
      };
      gContext->OMSetRenderTargets(ARRAY_SIZE(rts), rts, gSwapChainDSV);
      FLOAT clearColor[4] = {};
      gContext->ClearRenderTargetView(renderedRTV, clearColor);
      gContext->ClearRenderTargetView(gPositionRTV, clearColor);
      gContext->ClearRenderTargetView(gNormalRTV, clearColor);
      gContext->ClearRenderTargetView(gAlbedoRTV, clearColor);
    }

    // gContext->OMSetRenderTargets(1, &renderedRTV, gSwapChainDSV);
    // FLOAT clearColor[4] = {0.1f, 0.1f, 0.1f, 1};
    // gContext->ClearRenderTargetView(renderedRTV, clearColor);
    gContext->ClearDepthStencilView(gSwapChainDSV, D3D11_CLEAR_DEPTH, 0, 0);

    gContext->RSSetState(rasterizerState);
    gContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ID3D11ShaderResourceView *skyResources[] = {skyView, irrView};
    ID3D11SamplerState *skySamplers[] = {gDefaultSampler, gDefaultSampler};
    gContext->PSSetShaderResources(0, ARRAY_SIZE(skyResources), skyResources);
    gContext->PSSetSamplers(0, ARRAY_SIZE(skySamplers), skySamplers);

#if 1
    // Render sky
    gContext->OMSetDepthStencilState(skyDepthStencilState, 0);
    useProgram(&skyProgram);
    UINT stride = sizeof(Float4), offset = 0;
    gContext->IASetVertexBuffers(0, 1, &skyVB, &stride, &offset);
    gContext->IASetIndexBuffer(skyIB, DXGI_FORMAT_R32_UINT, 0);
    gContext->DrawIndexed(ARRAY_SIZE(skyboxIndices), 0, 0);
#endif

#if 1
    gContext->OMSetDepthStencilState(depthStencilState, 0);
    gContext->OMSetBlendState(NULL, NULL, 0xFFFFFFFF);
    useProgram(&brdfProgram);

    for (int i = 0; i < numModels; ++i) {
      renderModel(models[i], drawUniformBuffer, materialUniformBuffer);
    }

    if (gui.renderWireframedBackface) {
      gContext->RSSetState(wireframeRasterizerState);
      useProgram(&wireframeProgram);
      for (int i = 0; i < numModels; ++i) {
        renderModel(models[i], drawUniformBuffer, materialUniformBuffer);
      }
      gContext->RSSetState(rasterizerState);
    }
#endif

    {
      gContext->OMSetDepthStencilState(oitAccumDepthStencilState, 0);
      ID3D11RenderTargetView *renderTargets[] = {
          oitAccumRTV,
          oitRevealRTV,
      };
      gContext->OMSetRenderTargets(ARRAY_SIZE(renderTargets), renderTargets,
                                   gSwapChainDSV);
      FLOAT cc[4] = {};
      gContext->ClearRenderTargetView(oitAccumRTV, cc);
      cc[0] = 1;
      gContext->ClearRenderTargetView(oitRevealRTV, cc);

      useProgram(&oitAccumProgram);
      gContext->OMSetBlendState(oitAccumBlendState, NULL, 0xFFFFFFFF);

#if 1
      for (int i = 0; i < numModels; ++i) {
        renderModel(models[i], drawUniformBuffer, materialUniformBuffer);
      }
#else
      renderModel(models[0], drawUniformBuffer, materialUniformBuffer);
#endif

      renderTargets[0] = renderedRTV;
      renderTargets[1] = NULL;
      gContext->OMSetRenderTargets(ARRAY_SIZE(renderTargets), renderTargets,
                                   gSwapChainDSV);

      gContext->OMSetDepthStencilState(noDepthStencilState, 0);
      useProgram(&oitCompositeProgram);
      gContext->OMSetBlendState(oitCompositeBlendState, NULL, 0xFFFFFFFF);

      ID3D11ShaderResourceView *views[] = {
          oitAccumView,
          oitRevealView,
      };
      gContext->PSSetShaderResources(0, ARRAY_SIZE(views), views);
      gContext->Draw(3, 0);
    }

    gContext->OMSetBlendState(NULL, NULL, 0xFFFFFFFF);

    // Post processing
    if (gui.renderDepthBuffer) {
      gContext->OMSetRenderTargets(1, &gSwapChainRTV, NULL);
      gContext->OMSetDepthStencilState(noDepthStencilState, 0);
      useProgram(&fstProgram);
      gContext->PSSetShaderResources(0, 1, &depthTextureView);
      gContext->Draw(3, 0);
      ID3D11ShaderResourceView *nullView = NULL;
      gContext->PSSetShaderResources(0, 1, &nullView);
    } else {
      {
          // ID3D11RenderTargetView *renderTargets[] = {
          //     gSwapChainRTV,
          // };

          // gContext->OMSetRenderTargets(castI32U32(ARRAY_SIZE(renderTargets)),
          //                              renderTargets, NULL);

          // gContext->OMSetDepthStencilState(noDepthStencilState, 0);
          // useProgram(&gSSAOProgram);
          // ID3D11ShaderResourceView *resources[] = {
          //     gPositionView,
          //     gNormalView,
          //     gAlbedoView,
          // };
          // gContext->PSSetShaderResources(0, ARRAY_SIZE(resources),
          // resources); gContext->PSSetSamplers(0, 1, &postProcessSampler);
          // gContext->Draw(3, 0);
          // resources[0] = NULL;
          // resources[1] = NULL;
          // resources[2] = NULL;
          // ID3D11SamplerState *nullSampler = NULL;
          // gContext->PSSetShaderResources(0, ARRAY_SIZE(resources),
          // resources); gContext->PSSetSamplers(0, 1, &nullSampler);

          // renderTargets[0] = NULL;
          // gContext->OMSetRenderTargets(castI32U32(ARRAY_SIZE(renderTargets)),
          //                              renderTargets, NULL);
      }

      // Exposure tone mapping
      {
        ID3D11RenderTargetView *renderTargets[] = {
            gSwapChainRTV,
        };
        gContext->OMSetRenderTargets(castI32U32(ARRAY_SIZE(renderTargets)),
                                     renderTargets, NULL);
        gContext->OMSetDepthStencilState(noDepthStencilState, 0);
        useProgram(&exposureProgram);
        gContext->PSSetShaderResources(0, 1, &renderedView);
        gContext->PSSetSamplers(0, 1, &postProcessSampler);
        gContext->Draw(3, 0);
        ID3D11ShaderResourceView *nullResource = NULL;
        ID3D11SamplerState *nullSampler = NULL;
        gContext->PSSetShaderResources(0, 1, &nullResource);
        gContext->PSSetSamplers(0, 1, &nullSampler);

        renderTargets[0] = NULL;
        gContext->OMSetRenderTargets(castI32U32(ARRAY_SIZE(renderTargets)),
                                     renderTargets, NULL);
      }
      FLOAT clearColor[4] = {};
      gContext->ClearRenderTargetView(gSwapChainRTV, clearColor);
    }
#endif

    gContext->OMSetRenderTargets(1, &gSwapChainRTV, NULL);
    renderGUI();

    gSwapChain->Present(1, 0);
  }

  destroyGUI();

  destroySSAOResources();

  COM_RELEASE(irrView);
  COM_RELEASE(skyView);
  COM_RELEASE(irrTex);
  COM_RELEASE(skyTex);

  COM_RELEASE(skyIB);
  COM_RELEASE(skyVB);

  for (int i = 0; i < numModels; ++i) {
    destroyModel(models[i]);
    MFREE(models[i]);
  }
  // destroyModel(&model2);
  // destroyModel(&model);

  COM_RELEASE(materialUniformBuffer);
  COM_RELEASE(drawUniformBuffer);
  COM_RELEASE(viewUniformBuffer);

  // destroyProgram(&oitCompositeProgram);
  // destroyProgram(&oitAccumProgram);
  // destroyProgram(&fstProgram);
  // destroyProgram(&wireframeProgram);
  // destroyProgram(&exposureProgram);
  // destroyProgram(&skyProgram);
  // destroyProgram(&brdfProgram);
  destroyProgram(&forwardPBRProgram);

  COM_RELEASE(refBlendState);
  COM_RELEASE(wireframeRasterizerState);
  COM_RELEASE(rasterizerState);

  COM_RELEASE(noDepthStencilState);
  COM_RELEASE(skyDepthStencilState);
  COM_RELEASE(depthStencilState);

  COM_RELEASE(gDefaultSampler);
  COM_RELEASE(gDefaultTextureView);
  COM_RELEASE(gDefaultTexture);

  COM_RELEASE(postProcessSampler);
  COM_RELEASE(renderedRTV);
  COM_RELEASE(renderedView);
  COM_RELEASE(renderedTexture);

  COM_RELEASE(gSwapChainDSV);
  COM_RELEASE(depthTextureView);
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

  destroyAssetLoader();

  return 0;
}
