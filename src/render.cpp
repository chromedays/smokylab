#include "render.h"
#include "util.h"
#include "asset.h"
#include "app.h"
#include "camera.h"
#include "resource.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <cgltf.h>
#include <stb_image.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <d3d11shader.h>
#include <d3d11_1.h>
#include <d3d12.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>
#pragma clang diagnostic pop
#include <random>

C_INTERFACE_BEGIN

Renderer gRenderer;

static ID3D11Device *gDevice;
static ID3D11DeviceContext *gContext;
static IDXGISwapChain *gSwapChain;
static ID3D11RenderTargetView *gSwapChainRTV;
static GPUTexture2D *gSwapChainDepthStencilBuffer;
static GPUTextureView *gSwapChainDepthTextureView;
static ID3D11DepthStencilView *gSwapChainDSV;

static GPUTexture2D *gDefaultTexture;
static GPUTextureView *gDefaultTextureView;
static ID3D11SamplerState *gDefaultSampler;

static ID3D11DepthStencilState *gDefaultDepthStencilState;
static ID3D11RasterizerState *gDefaultRasterizerState;

static const Camera *gViewCamera;
static GPUBuffer *gViewBuffer;
static GPUBuffer *gMaterialBuffer;
static GPUBuffer *gDrawBuffer;

static GPUBuffer *gCameraVolumeBuffer;

static ShaderProgram gForwardPBRProgram;
static ShaderProgram gDebugProgram;

static ID3D12Device *gDummyDeviceForFixedGPUClock;
static ID3D11Query *gProfilerDisjointQuery;
static ID3D11Query *gProfilerTimestampBeginQuery;
static ID3D11Query *gProfilerTimestampEndQuery;

static D3D11_USAGE toNativeUsage(GPUResourceUsage usage) {
  return (D3D11_USAGE)usage;
}

static DXGI_FORMAT toNativeFormat(GPUResourceFormat format) {
  return (DXGI_FORMAT)format;
}

static D3D11_FILTER toNativeFilter(GPUFilter filter) {
  return (D3D11_FILTER)filter;
}

static D3D11_TEXTURE_ADDRESS_MODE
toNativeTextureAddressMode(GPUTextureAddressMode mode) {
  return (D3D11_TEXTURE_ADDRESS_MODE)mode;
}

static DXGI_RATIONAL queryRefreshRate(int ww, int wh,
                                      GPUResourceFormat swapChainFormat) {
  DXGI_RATIONAL refreshRate = {};

  IDXGIFactory *factory;
  HR_ASSERT(CreateDXGIFactory(IID_PPV_ARGS(&factory)));

  IDXGIAdapter *adapter;
  HR_ASSERT(factory->EnumAdapters(0, &adapter));

  IDXGIOutput *adapterOutput;
  HR_ASSERT(adapter->EnumOutputs(0, &adapterOutput));

  UINT numDisplayModes;
  HR_ASSERT(adapterOutput->GetDisplayModeList(toNativeFormat(swapChainFormat),
                                              DXGI_ENUM_MODES_INTERLACED,
                                              &numDisplayModes, NULL));

  DXGI_MODE_DESC *displayModes =
      MMALLOC_ARRAY_UNINITIALIZED(DXGI_MODE_DESC, castU32I32(numDisplayModes));

  HR_ASSERT(adapterOutput->GetDisplayModeList(toNativeFormat(swapChainFormat),
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

static HWND getWin32WindowHandle(SDL_Window *window) {
  SDL_SysWMinfo wmInfo;
  SDL_VERSION(&wmInfo.version)
  SDL_GetWindowWMInfo(window, &wmInfo);
  HWND hwnd = wmInfo.info.win.window;
  return hwnd;
}

static bool isDeveloperModeEnabled(void) {
  HKEY hKey;
  HRESULT err = RegOpenKeyExW(
      HKEY_LOCAL_MACHINE,
      L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock", 0,
      KEY_READ, &hKey);
  if (err != ERROR_SUCCESS) {
    return false;
  }
  DWORD value = 0;
  DWORD dwordSize = sizeof(DWORD);
  err = RegQueryValueExW(hKey, L"AllowDevelopmentWithoutDevLicense", 0, NULL,
                         (LPBYTE)(&value), &dwordSize);
  RegCloseKey(hKey);
  if (err != ERROR_SUCCESS) {
    return false;
  }
  return value != 0;
}

void initRenderer(void) {
  if (isDeveloperModeEnabled()) {
    HR_ASSERT(D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_12_0,
                                IID_PPV_ARGS(&gDummyDeviceForFixedGPUClock)));
    HR_ASSERT(gDummyDeviceForFixedGPUClock->SetStablePowerState(TRUE));
    COM_RELEASE(gDummyDeviceForFixedGPUClock);
  } else {
    LOG("Warning: Developer mode is not enabled. GPU profiler result will be "
        "inconsistent");
  }

  int windowWidth, windowHeight;
  SDL_GetWindowSize(gApp.window, &windowWidth, &windowHeight);

  DXGI_SWAP_CHAIN_DESC swapChainDesc = {
      .BufferDesc =
          {
              .Width = castI32U32(windowWidth),
              .Height = castI32U32(windowHeight),
              .RefreshRate = queryRefreshRate(windowWidth, windowHeight,
                                              GPUResourceFormat_R8G8B8A8_UNORM),
              .Format = toNativeFormat(GPUResourceFormat_R8G8B8A8_UNORM),
          },
      .SampleDesc =
          {
              .Count = 1,
              .Quality = 0,
          },
      .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
      .BufferCount = 2,
      .OutputWindow = getWin32WindowHandle(gApp.window),
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

  gRenderer.device = (GPUDevice *)gDevice;
  gRenderer.deviceContext = (GPUDeviceContext *)gContext;

  ID3D11Texture2D *backBuffer;
  HR_ASSERT(gSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));
  HR_ASSERT(gDevice->CreateRenderTargetView(backBuffer, NULL, &gSwapChainRTV));
  COM_RELEASE(backBuffer);

  TextureDesc depthStencilTextureDesc = {
      .width = windowWidth,
      .height = windowHeight,
      .format = GPUResourceFormat_R32_TYPELESS,
      .viewFormat = GPUResourceFormat_R32_FLOAT,
      .usage = GPUResourceUsage_DEFAULT,
      .bindFlags = GPUResourceBindBits_DEPTH_STENCIL |
                   GPUResourceBindBits_SHADER_RESOURCE,
  };
  createTexture2D(&depthStencilTextureDesc, &gSwapChainDepthStencilBuffer,
                  &gSwapChainDepthTextureView);

  D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {
      .Format = toNativeFormat(GPUResourceFormat_D32_FLOAT),
      .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
      .Flags = 0,
      .Texture2D = {.MipSlice = 0},
  };
  HR_ASSERT(gDevice->CreateDepthStencilView(
      (ID3D11Texture2D *)gSwapChainDepthStencilBuffer, &depthStencilViewDesc,
      &gSwapChainDSV));

  TextureDesc defaultTextureDesc = {
      .width = 16,
      .height = 16,
      .bytesPerPixel = 4,
      .format = GPUResourceFormat_R8G8B8A8_UNORM,
      .usage = GPUResourceUsage_IMMUTABLE,
      .bindFlags = GPUResourceBindBits_SHADER_RESOURCE,
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

  D3D11_DEPTH_STENCIL_DESC depthStencilStateDesc = {
      .DepthEnable = TRUE,
      .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
      .DepthFunc = D3D11_COMPARISON_GREATER_EQUAL,
      .StencilEnable = FALSE,
  };
  HR_ASSERT(gDevice->CreateDepthStencilState(&depthStencilStateDesc,
                                             &gDefaultDepthStencilState));

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
  HR_ASSERT(gDevice->CreateRasterizerState(&rasterizerStateDesc,
                                           &gDefaultRasterizerState));

  // ID3D11RasterizerState *wireframeRasterizerState;
  // D3D11_RASTERIZER_DESC wireframeRasterizerStateDesc = {
  //     .FillMode = D3D11_FILL_WIREFRAME,
  //     .CullMode = D3D11_CULL_FRONT,
  //     .FrontCounterClockwise = TRUE,
  //     .DepthBias = 0,
  //     .DepthBiasClamp = 0.f,
  //     .SlopeScaledDepthBias = 0.f,
  //     .DepthClipEnable = TRUE,
  //     .ScissorEnable = FALSE,
  //     .MultisampleEnable = FALSE,
  //     .AntialiasedLineEnable = FALSE,
  // };
  // HR_ASSERT(gDevice->CreateRasterizerState(&wireframeRasterizerStateDesc,
  //                                          &wireframeRasterizerState));

  BufferDesc bufferDesc;

  bufferDesc = {.size = sizeof(ViewUniforms),
                .usage = GPUResourceUsage_DEFAULT,
                .bindFlags = GPUResourceBindBits_CONSTANT_BUFFER};
  createBuffer(&bufferDesc, &gViewBuffer);

  bufferDesc = {
      .size = sizeof(MaterialUniforms),
      .usage = GPUResourceUsage_DEFAULT,
      .bindFlags = GPUResourceBindBits_CONSTANT_BUFFER,
  };
  createBuffer(&bufferDesc, &gMaterialBuffer);

  bufferDesc = {
      .size = sizeof(DrawUniforms),
      .usage = GPUResourceUsage_DEFAULT,
      .bindFlags = GPUResourceBindBits_CONSTANT_BUFFER,
  };
  createBuffer(&bufferDesc, &gDrawBuffer);

  bufferDesc = {
      .size = sizeof(Float4) * 24,
      .usage = GPUResourceUsage_DEFAULT,
      .bindFlags = GPUResourceBindBits_VERTEX_BUFFER,
  };
  createBuffer(&bufferDesc, &gCameraVolumeBuffer);

  loadProgram("forward_pbr", &gForwardPBRProgram);
  loadProgram("debug", &gDebugProgram);

  D3D11_QUERY_DESC queryDesc = {};
  queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
  HR_ASSERT(gDevice->CreateQuery(&queryDesc, &gProfilerDisjointQuery));

  queryDesc.Query = D3D11_QUERY_TIMESTAMP;
  HR_ASSERT(gDevice->CreateQuery(&queryDesc, &gProfilerTimestampBeginQuery));
  HR_ASSERT(gDevice->CreateQuery(&queryDesc, &gProfilerTimestampEndQuery));
}

void destroyRenderer(void) {
  destroyProgram(&gDebugProgram);
  destroyProgram(&gForwardPBRProgram);

  destroyBuffer(gCameraVolumeBuffer);
  destroyBuffer(gDrawBuffer);
  destroyBuffer(gMaterialBuffer);
  destroyBuffer(gViewBuffer);

  // COM_RELEASE(wireframeRasterizerState);
  COM_RELEASE(gDefaultRasterizerState);
  COM_RELEASE(gDefaultDepthStencilState);

  COM_RELEASE(gDefaultSampler);
  COM_RELEASE(gDefaultTextureView);
  COM_RELEASE(gDefaultTexture);

  COM_RELEASE(gSwapChainDSV);
  COM_RELEASE(gSwapChainDepthTextureView);
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

  if (gDummyDeviceForFixedGPUClock) {
    HR_ASSERT(gDummyDeviceForFixedGPUClock->SetStablePowerState(FALSE));
    COM_RELEASE(gDummyDeviceForFixedGPUClock);
  }
}

void beginRender(void) {
  gContext->Begin(gProfilerDisjointQuery);
  gContext->End(gProfilerTimestampBeginQuery);
}

void endRender(void) {
  gContext->End(gProfilerTimestampEndQuery);
  gContext->End(gProfilerDisjointQuery);

  while (gContext->GetData(gProfilerDisjointQuery, NULL, 0, 0) == S_FALSE) {
    Sleep(1); // Wait a bit, but give other threads a chance to run
  }

  D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint;

  HR_ASSERT(gContext->GetData(gProfilerDisjointQuery, &disjoint,
                              sizeof(disjoint), 0));
  ASSERT(!disjoint.Disjoint);
  UINT64 startTime;
  HR_ASSERT(gContext->GetData(gProfilerTimestampBeginQuery, &startTime,
                              sizeof(startTime), 0));
  UINT64 endTime;
  HR_ASSERT(gContext->GetData(gProfilerTimestampEndQuery, &endTime,
                              sizeof(endTime), 0));
  gRenderer.renderTime =
      (float)(endTime - startTime) / (float)disjoint.Frequency;

  UINT syncInterval = gRenderer.vsync ? 1 : 0;
  gSwapChain->Present(syncInterval, 0);
}

void setViewport(float x, float y, float w, float h) {
  D3D11_VIEWPORT viewport = {
      .TopLeftX = x,
      .TopLeftY = y,
      .Width = w,
      .Height = h,
      .MinDepth = 0,
      .MaxDepth = 1,
  };
  gContext->RSSetViewports(1, &viewport);
}

void setDefaultModelRenderStates(Float4 clearColor) {
  gContext->OMSetRenderTargets(1, &gSwapChainRTV, gSwapChainDSV);
  gContext->ClearRenderTargetView(gSwapChainRTV, (FLOAT *)&clearColor);
  gContext->ClearDepthStencilView(gSwapChainDSV, D3D11_CLEAR_DEPTH, 0, 0);

  gContext->RSSetState(gDefaultRasterizerState);
  gContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  gContext->OMSetDepthStencilState(gDefaultDepthStencilState, 0);
  gContext->OMSetBlendState(NULL, NULL, 0xFFFFFFFF);

  GPUBuffer *uniformBuffers[] = {gViewBuffer, gDrawBuffer, gMaterialBuffer};
  bindBuffers(ARRAY_SIZE(uniformBuffers), uniformBuffers);

  useProgram(&gForwardPBRProgram);
}

void createProgram(ShaderProgram *program, int vertSrcSize, void *vertSrc,
                   int fragSrcSize, void *fragSrc) {

  HR_ASSERT(gDevice->CreateVertexShader(vertSrc, castI32U32(vertSrcSize), NULL,
                                        (ID3D11VertexShader **)&program->vert));

  ID3D11ShaderReflection *refl;
  HR_ASSERT(D3DReflect(vertSrc, castI32U32(vertSrcSize), IID_PPV_ARGS(&refl)));

  D3D11_SHADER_DESC shaderDesc;
  HR_ASSERT(refl->GetDesc(&shaderDesc));

  D3D11_INPUT_ELEMENT_DESC *inputAttribs = MMALLOC_ARRAY(
      D3D11_INPUT_ELEMENT_DESC, castU32I32(shaderDesc.InputParameters));

  for (UINT i = 0; i < shaderDesc.InputParameters; ++i) {
    D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
    HR_ASSERT(refl->GetInputParameterDesc(i, &paramDesc));

    D3D11_INPUT_ELEMENT_DESC *attrib = &inputAttribs[i];

    attrib->SemanticName = paramDesc.SemanticName;
    attrib->SemanticIndex = paramDesc.SemanticIndex;
    attrib->AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    attrib->InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

    if (paramDesc.Mask == 1) // 1
    {
      if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) {
        attrib->Format = DXGI_FORMAT_R32_UINT;
      } else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) {

        attrib->Format = DXGI_FORMAT_R32_SINT;
      } else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) {
        attrib->Format = DXGI_FORMAT_R32_FLOAT;
      }
    } else if (paramDesc.Mask <= 3) // 11
    {
      if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) {
        attrib->Format = DXGI_FORMAT_R32G32_UINT;
      } else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) {
        attrib->Format = DXGI_FORMAT_R32G32_SINT;
      } else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) {
        attrib->Format = DXGI_FORMAT_R32G32_FLOAT;
      }
    } else if (paramDesc.Mask <= 7) // 111
    {
      if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) {
        attrib->Format = DXGI_FORMAT_R32G32B32_UINT;
      } else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) {
        attrib->Format = DXGI_FORMAT_R32G32B32_SINT;
      } else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) {
        attrib->Format = DXGI_FORMAT_R32G32B32_FLOAT;
      }
    } else if (paramDesc.Mask <= 15) // 1111
    {
      if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) {
        attrib->Format = DXGI_FORMAT_R32G32B32A32_UINT;
      } else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) {
        attrib->Format = DXGI_FORMAT_R32G32B32A32_SINT;
      } else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) {
        attrib->Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
      }
    }
  }

  HR_ASSERT(gDevice->CreateInputLayout(
      inputAttribs, shaderDesc.InputParameters, vertSrc,
      castI32U32(vertSrcSize), (ID3D11InputLayout **)&program->inputLayout));

  MFREE(inputAttribs);

  HR_ASSERT(gDevice->CreatePixelShader(fragSrc, castI32U32(fragSrcSize), NULL,
                                       (ID3D11PixelShader **)&program->frag));
}
void destroyProgram(ShaderProgram *program) {
  COM_RELEASE(program->frag);
  COM_RELEASE(program->vert);
  COM_RELEASE(program->inputLayout);
  *program = {};
}

void useProgram(const ShaderProgram *program) {
  gContext->IASetInputLayout((ID3D11InputLayout *)program->inputLayout);
  gContext->VSSetShader((ID3D11VertexShader *)program->vert, NULL, 0);
  gContext->PSSetShader((ID3D11PixelShader *)program->frag, NULL, 0);
}

void createBuffer(const BufferDesc *desc, GPUBuffer **buffer) {
  D3D11_BUFFER_DESC bufferDesc = {
      .ByteWidth = castI32U32(desc->size),
      .Usage = toNativeUsage(desc->usage),
      .BindFlags = desc->bindFlags,
  };

  if (desc->initialData) {
    D3D11_SUBRESOURCE_DATA initialData = {
        .pSysMem = desc->initialData,
    };
    HR_ASSERT(gDevice->CreateBuffer(&bufferDesc, &initialData,
                                    (ID3D11Buffer **)buffer));
  } else {
    HR_ASSERT(
        gDevice->CreateBuffer(&bufferDesc, NULL, (ID3D11Buffer **)buffer));
  }
}

void createImmutableVertexBuffer(int numVertices, const Vertex *vertices,
                                 GPUBuffer **buffer) {
  BufferDesc desc = {
      .size = castUsizeI32(numVertices * sizeof(Vertex)),
      .initialData = vertices,
      .usage = GPUResourceUsage_IMMUTABLE,
      .bindFlags = GPUResourceBindBits_VERTEX_BUFFER,
  };
  createBuffer(&desc, buffer);
}

void createImmutableIndexBuffer(int numIndices, const VertexIndex *indices,
                                GPUBuffer **buffer) {
  BufferDesc desc = {
      .size = castUsizeI32(numIndices * sizeof(VertexIndex)),
      .initialData = indices,
      .usage = GPUResourceUsage_IMMUTABLE,
      .bindFlags = GPUResourceBindBits_INDEX_BUFFER,
  };
  createBuffer(&desc, buffer);
}

void destroyBuffer(GPUBuffer *buffer) { COM_RELEASE(buffer); }

void updateBufferData(GPUBuffer *buffer, void *data) {
  gContext->UpdateSubresource((ID3D11Buffer *)buffer, 0, NULL, data, 0, 0);
}

void bindBuffers(int numBuffers, GPUBuffer **buffers) {
  gContext->VSSetConstantBuffers(0, castI32U32(numBuffers),
                                 (ID3D11Buffer **)buffers);
  gContext->PSSetConstantBuffers(0, castI32U32(numBuffers),
                                 (ID3D11Buffer **)buffers);
}

void createTexture2D(const TextureDesc *desc, GPUTexture2D **texture,
                     GPUTextureView **textureView) {
  D3D11_TEXTURE2D_DESC textureDesc = {
      .Width = castI32U32(desc->width),
      .Height = castI32U32(desc->height),
      .MipLevels = 1,
      .ArraySize = 1,
      .Format = toNativeFormat(desc->format),
      .SampleDesc =
          {
              .Count = 1,
              .Quality = 0,
          },
      .Usage = toNativeUsage(desc->usage),
      .BindFlags = desc->bindFlags,
      .CPUAccessFlags = 0,
  };

  UINT pitch = castI32U32(desc->width) * castI32U32(desc->bytesPerPixel);

  if (desc->generateMipMaps) {
    textureDesc.MipLevels =
        (uint32_t)floorf(log2f((float)MAX(desc->width, desc->height)));
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags |= (GPUResourceBindBits_RENDER_TARGET |
                              GPUResourceBindBits_SHADER_RESOURCE);
    textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    HR_ASSERT(gDevice->CreateTexture2D(&textureDesc, NULL,
                                       (ID3D11Texture2D **)texture));
    if (desc->initialData) {
      gContext->UpdateSubresource((ID3D11Texture2D *)*texture, 0, 0,
                                  desc->initialData, pitch, 0);
    }
  } else {
    if (desc->initialData) {
      D3D11_SUBRESOURCE_DATA initialData = {
          .pSysMem = desc->initialData,
          .SysMemPitch = pitch,
      };
      HR_ASSERT(gDevice->CreateTexture2D(&textureDesc, &initialData,
                                         (ID3D11Texture2D **)texture));
    } else {
      HR_ASSERT(gDevice->CreateTexture2D(&textureDesc, NULL,
                                         (ID3D11Texture2D **)texture));
    }
  }

  if (textureView) {
    if (desc->viewFormat) {
      D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {
          .Format = toNativeFormat(desc->viewFormat),
          .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
          .Texture2D =
              {
                  .MostDetailedMip = 0,
                  .MipLevels = textureDesc.MipLevels,
              },
      };
      HR_ASSERT(gDevice->CreateShaderResourceView(
          (ID3D11Texture2D *)*texture, &viewDesc,
          (ID3D11ShaderResourceView **)textureView));
    } else {
      HR_ASSERT(gDevice->CreateShaderResourceView(
          (ID3D11Texture2D *)*texture, NULL,
          (ID3D11ShaderResourceView **)textureView));
    }

    if (desc->generateMipMaps) {
      gContext->GenerateMips((ID3D11ShaderResourceView *)*textureView);
    }
  }
}

void createSampler(const SamplerDesc *desc, GPUSampler **sampler) {
  D3D11_SAMPLER_DESC d3d11SamplerDesc = {
      .Filter = toNativeFilter(desc->filter),
      .AddressU = toNativeTextureAddressMode(desc->addressMode.u),
      .AddressV = toNativeTextureAddressMode(desc->addressMode.v),
      .AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
      .MaxLOD = D3D11_FLOAT32_MAX,
  };

  HR_ASSERT(gDevice->CreateSamplerState(&d3d11SamplerDesc,
                                        (ID3D11SamplerState **)sampler));
}

void destroyModel(Model *model) {
  for (int i = 0; i < model->numScenes; ++i) {
    Scene *scene = &model->scenes[i];
    MFREE(model->scenes[i].nodes);
    destroyString(&scene->name);
  }
  MFREE(model->scenes);

  for (int i = 0; i < model->numNodes; ++i) {
    SceneNode *node = &model->nodes[i];
    MFREE(node->childNodes);
    destroyString(&node->name);
  }
  MFREE(model->nodes);

  MFREE(model->meshes);

  MFREE(model->materials);

  for (int i = 0; i < model->numSamplers; ++i) {
    COM_RELEASE(model->samplers[i]);
  }
  MFREE(model->samplers);

  for (int i = 0; i < model->numTextures; ++i) {
    COM_RELEASE(model->textureViews[i]);
    COM_RELEASE(model->textures[i]);
  }
  MFREE(model->textures);

  destroyString(&model->name);
}

static void renderMesh(const Model *model, const Mesh *mesh,
                       UNUSED GPUBuffer *drawUniformBuffer,
                       GPUBuffer *materialUniformBuffer) {

  for (int subMeshIndex = 0; subMeshIndex < mesh->numSubMeshes;
       ++subMeshIndex) {
    const SubMesh *subMesh = &mesh->subMeshes[subMeshIndex];
    const Material *material = &model->materials[subMesh->material];
    MaterialUniforms materialUniforms = {
        .baseColorFactor = material->baseColorFactor,
        .metallicRoughnessFactor = {material->metallicFactor,
                                    material->roughnessFactor}};

    ID3D11ShaderResourceView *baseColorTextureView =
        (ID3D11ShaderResourceView *)gDefaultTextureView;
    if (material->baseColorTexture >= 0) {
      baseColorTextureView =
          (ID3D11ShaderResourceView *)
              model->textureViews[material->baseColorTexture];
    }

    ID3D11SamplerState *baseColorSampler = gDefaultSampler;
    if (material->baseColorSampler >= 0) {
      baseColorSampler =
          (ID3D11SamplerState *)model->samplers[material->baseColorSampler];
    }

    ID3D11ShaderResourceView *metallicRoughnessTextureView =
        (ID3D11ShaderResourceView *)gDefaultTextureView;
    if (material->metallicRoughnessTexture >= 0) {
      metallicRoughnessTextureView =
          (ID3D11ShaderResourceView *)
              model->textureViews[material->metallicRoughnessTexture];
    }

    ID3D11SamplerState *metallicRoughnessSampler = gDefaultSampler;
    if (material->metallicRoughnessSampler >= 0) {
      metallicRoughnessSampler =
          (ID3D11SamplerState *)
              model->samplers[material->metallicRoughnessSampler];
    }

    gContext->UpdateSubresource((ID3D11Buffer *)materialUniformBuffer, 0, NULL,
                                &materialUniforms, 0, 0);

    ID3D11ShaderResourceView *textureViews[] = {baseColorTextureView,
                                                metallicRoughnessTextureView};
    ID3D11SamplerState *samplers[] = {baseColorSampler,
                                      metallicRoughnessSampler};
    gContext->PSSetShaderResources(2, ARRAY_SIZE(textureViews), textureViews);
    gContext->PSSetSamplers(2, ARRAY_SIZE(samplers), samplers);
    gContext->DrawIndexed(castI32U32(subMesh->numIndices),
                          subMesh->gpuIndexOffset, subMesh->gpuVertexOffset);
    // model->indexBaseOffset + subMesh->indexOffset
  }
}

static void renderSceneNode(const Model *model, const SceneNode *node,
                            GPUBuffer *drawUniformBuffer,
                            GPUBuffer *materialUniformBuffer) {
  if (node->mesh >= 0) {
    DrawUniforms drawUniforms = {};
    drawUniforms.modelMat = node->worldTransform.matrix;
    drawUniforms.invModelMat = mat4Inverse(drawUniforms.modelMat);
    gContext->UpdateSubresource((ID3D11Buffer *)drawUniformBuffer, 0, NULL,
                                &drawUniforms, 0, 0);
    renderMesh(model, model->meshes[node->mesh], drawUniformBuffer,
               materialUniformBuffer);
  }

  if (node->numChildNodes > 0) {
    for (int i = 0; i < node->numChildNodes; ++i) {
      const SceneNode *childNode = &model->nodes[node->childNodes[i]];
      renderSceneNode(model, childNode, drawUniformBuffer,
                      materialUniformBuffer);
    }
  }
}

void renderModel(const Model *model) {
  GPUBuffer *materialUniformBuffer = gMaterialBuffer;
  GPUBuffer *drawUniformBuffer = gDrawBuffer;

  UINT stride = sizeof(Vertex), offset = 0;
  // TODO: Check if static mesh/buffer
  GPUStaticBufferCache staticBufferCache = getStaticBufferCache();
  gContext->IASetVertexBuffers(
      0, 1, (ID3D11Buffer **)&staticBufferCache.vertexBuffer, &stride, &offset);
  gContext->IASetIndexBuffer((ID3D11Buffer *)staticBufferCache.indexBuffer,
                             DXGI_FORMAT_R32_UINT, 0);
  gContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  for (int sceneIndex = 0; sceneIndex < model->numScenes; ++sceneIndex) {
    const Scene *scene = &model->scenes[sceneIndex];
    for (int nodeIndex = 0; nodeIndex < scene->numNodes; ++nodeIndex) {
      const SceneNode *node = &model->nodes[scene->nodes[nodeIndex]];
      renderSceneNode(model, node, drawUniformBuffer, materialUniformBuffer);
    }
  }
}

void beginGPUProfiler(void) {}

void gpuProfilerTimeStamp(void) {}

void endGPUProfiler(void) {}

void setCamera(const Camera *camera) {
  ViewUniforms viewUniforms = {
      .viewMat = getViewMatrix(camera),
      .projMat = mat4Perspective(camera->verticalFovDeg, camera->aspectRatio,
                                 camera->nearZ, camera->farZ),
  };
  viewUniforms.viewPos.xyz = camera->pos;
  updateBufferData(gViewBuffer, &viewUniforms);
  gViewCamera = camera;
}

void setViewportByAppWindow(void) {
  int windowWidth, windowHeight;
  SDL_GetWindowSize(gApp.window, &windowWidth, &windowHeight);
  setViewport(0, 0, (float)windowWidth, (float)windowHeight);
}

void renderCameraVolume(const Camera *camera) {
  if (gViewCamera == camera) {
    return;
  }

  float halfFovRad = degToRad(camera->verticalFovDeg * 0.5f);

  Float2 tanFov =
      float2(tanf(halfFovRad) * camera->aspectRatio, tanf(halfFovRad));
  float nearZ = camera->nearZ;
  float farZ = camera->farZ * 0.005f;
  // clang-format off
  Float3 vertices[] = {
    float3(-nearZ * tanFov.x, -nearZ * tanFov.y, -nearZ),
    float3( nearZ * tanFov.x, -nearZ * tanFov.y, -nearZ),
    float3( nearZ * tanFov.x, -nearZ * tanFov.y, -nearZ),
    float3( nearZ * tanFov.x,  nearZ * tanFov.y, -nearZ),
    float3( nearZ * tanFov.x,  nearZ * tanFov.y, -nearZ),
    float3(-nearZ * tanFov.x,  nearZ * tanFov.y, -nearZ),
    float3(-nearZ * tanFov.x,  nearZ * tanFov.y, -nearZ),
    float3(-nearZ * tanFov.x, -nearZ * tanFov.y, -nearZ),

    float3(-nearZ * tanFov.x, -nearZ * tanFov.y, -nearZ),
    float3(-farZ * tanFov.x, -farZ * tanFov.y, -farZ),
    float3( nearZ * tanFov.x, -nearZ * tanFov.y, -nearZ),
    float3( farZ * tanFov.x, -farZ * tanFov.y, -farZ),
    float3( nearZ * tanFov.x,  nearZ * tanFov.y, -nearZ),
    float3( farZ * tanFov.x,  farZ * tanFov.y, -farZ),
    float3(-nearZ * tanFov.x,  nearZ * tanFov.y, -nearZ),
    float3(-farZ * tanFov.x,  farZ * tanFov.y, -farZ),

    float3(-farZ * tanFov.x, -farZ * tanFov.y, -farZ),
    float3( farZ * tanFov.x, -farZ * tanFov.y, -farZ),
    float3( farZ * tanFov.x, -farZ * tanFov.y, -farZ),
    float3( farZ * tanFov.x,  farZ * tanFov.y, -farZ),
    float3( farZ * tanFov.x,  farZ * tanFov.y, -farZ),
    float3(-farZ * tanFov.x,  farZ * tanFov.y, -farZ),
    float3(-farZ * tanFov.x,  farZ * tanFov.y, -farZ),
    float3(-farZ * tanFov.x, -farZ * tanFov.y, -farZ),
  };
  // clang-format on
  updateBufferData(gCameraVolumeBuffer, vertices);

  DrawUniforms drawUniforms = {
      .modelMat = mat4Inverse(getViewMatrix(camera)),
      .invModelMat = getViewMatrix(camera),
  };
  updateBufferData(gDrawBuffer, &drawUniforms);

  // TODO: setDebugRenderStates?
  useProgram(&gDebugProgram);

  UINT stride = sizeof(Float4);
  UINT offset = 0;
  gContext->IASetVertexBuffers(0, 1, (ID3D11Buffer **)&gCameraVolumeBuffer,
                               &stride, &offset);
  gContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

  gContext->Draw(24, 0);
}

C_INTERFACE_END
