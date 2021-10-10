#include "smk.h"
#include "asset.h"
#include "app.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_impl_sdl.h>
#include <cgltf.h>
#include <stb_image.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>
#pragma clang diagnostic pop

C_INTERFACE_BEGIN

static bool gIsRendererInitialized;

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

static DXGI_RATIONAL queryRefreshRate(int ww, int wh,
                                      DXGI_FORMAT swapChainFormat) {
  DXGI_RATIONAL refreshRate = {};

  IDXGIFactory *factory;
  HR_ASSERT(CreateDXGIFactory(IID_PPV_ARGS(&factory)));

  IDXGIAdapter *adapter;
  HR_ASSERT(factory->EnumAdapters(0, &adapter));

  IDXGIOutput *adapterOutput;
  HR_ASSERT(adapter->EnumOutputs(0, &adapterOutput));

  UINT numDisplayModes;
  HR_ASSERT(adapterOutput->GetDisplayModeList(
      swapChainFormat, DXGI_ENUM_MODES_INTERLACED, &numDisplayModes, NULL));

  DXGI_MODE_DESC *displayModes =
      MMALLOC_ARRAY_UNINITIALIZED(DXGI_MODE_DESC, castU32I32(numDisplayModes));

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

static HWND getWin32WindowHandle(SDL_Window *window) {
  SDL_SysWMinfo wmInfo;
  SDL_VERSION(&wmInfo.version)
  SDL_GetWindowWMInfo(window, &wmInfo);
  HWND hwnd = wmInfo.info.win.window;
  return hwnd;
}

static int getBytesPerPixelFromFormat(DXGI_FORMAT format) {
  int bytesPerPixel = 0;
  switch (format) {
  case DXGI_FORMAT_R8G8B8A8_UNORM:
  case DXGI_FORMAT_R32_TYPELESS:
  case DXGI_FORMAT_D32_FLOAT:
    bytesPerPixel = 4;
    break;
  case DXGI_FORMAT_R32G32B32A32_FLOAT:
    bytesPerPixel = 16;
    break;
  default:
    ASSERT(false);
  }
  return bytesPerPixel;
}

static DXGI_FORMAT
getDefaultCompatibleTextureViewFormat(DXGI_FORMAT textureFormat) {
  DXGI_FORMAT viewFormat = DXGI_FORMAT_UNKNOWN;

  switch (textureFormat) {
  case DXGI_FORMAT_R8G8B8A8_UNORM:
  case DXGI_FORMAT_D32_FLOAT:
  case DXGI_FORMAT_R32G32B32A32_FLOAT:
    viewFormat = textureFormat;
    break;
  case DXGI_FORMAT_R32_TYPELESS:
    viewFormat = DXGI_FORMAT_R32_FLOAT;
    break;
  default:
    ASSERT(false);
  }

  return viewFormat;
}

static smkTexture createTexture2D(ID3D11Device *device,
                                  ID3D11DeviceContext *context, int width,
                                  int height, DXGI_FORMAT format,
                                  D3D11_USAGE usage, uint32_t bindFlags,
                                  bool generateMipMaps, void *initialData) {
  ASSERT(device && context);
  ASSERT(generateMipMaps ? (usage == D3D11_USAGE_DEFAULT) : true);

  smkTexture texture = {};

  D3D11_TEXTURE2D_DESC textureDesc = {
      .Width = castI32U32(width),
      .Height = castI32U32(height),
      .MipLevels = 1,
      .ArraySize = 1,
      .Format = format,
      .SampleDesc =
          {
              .Count = 1,
              .Quality = 0,
          },
      .Usage = usage,
      .BindFlags = bindFlags,
      .CPUAccessFlags = 0,
  };

  if (generateMipMaps) {
    textureDesc.MipLevels =
        castFloatToU32(floorf(log2f((float)MAX(width, height))));
    textureDesc.BindFlags |=
        (D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
    textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    HR_ASSERT(device->CreateTexture2D(&textureDesc, NULL, &texture.handle));
    if (initialData) {
      int bytesPerPixel = getBytesPerPixelFromFormat(format);
      UINT pitch = castI32U32(width * bytesPerPixel);
      context->UpdateSubresource(texture.handle, 0, NULL, initialData, pitch,
                                 0);
    }
  } else {
    if (initialData) {
      int bytesPerPixel = getBytesPerPixelFromFormat(format);
      D3D11_SUBRESOURCE_DATA initialDataDesc = {
          .pSysMem = initialData,
          .SysMemPitch = castI32U32(width * bytesPerPixel),
      };

      HR_ASSERT(device->CreateTexture2D(&textureDesc, &initialDataDesc,
                                        &texture.handle));
    } else {
      HR_ASSERT(device->CreateTexture2D(&textureDesc, NULL, &texture.handle));
    }
  }

  D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {
      .Format = getDefaultCompatibleTextureViewFormat(format),
      .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
      .Texture2D =
          {
              .MostDetailedMip = 0,
              .MipLevels = textureDesc.MipLevels,
          },
  };

  HR_ASSERT(device->CreateShaderResourceView(texture.handle, &viewDesc,
                                             &texture.view));

  if (generateMipMaps) {
    context->GenerateMips(texture.view);
  }

  return texture;
}

static ID3D11SamplerState *
createSampler(ID3D11Device *device, D3D11_FILTER filter,
              D3D11_TEXTURE_ADDRESS_MODE addressModeU,
              D3D11_TEXTURE_ADDRESS_MODE addressModeV) {
  ASSERT(device);

  ID3D11SamplerState *sampler = {};

  D3D11_SAMPLER_DESC samplerDesc = {
      .Filter = filter,
      .AddressU = addressModeU,
      .AddressV = addressModeV,
      .AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
      .MaxLOD = D3D11_FLOAT32_MAX,
  };

  HR_ASSERT(device->CreateSamplerState(&samplerDesc, &sampler));

  return sampler;
}

static ID3D11Buffer *createBuffer(ID3D11Device *device, int size,
                                  void *initialData, D3D11_USAGE usage,
                                  uint32_t bindFlags) {
  ASSERT(device);

  ID3D11Buffer *buffer = NULL;

  D3D11_BUFFER_DESC bufferDesc = {
      .ByteWidth = castI32U32(size),
      .Usage = usage,
      .BindFlags = bindFlags,
  };

  if (initialData) {
    D3D11_SUBRESOURCE_DATA initialDataDesc = {
        .pSysMem = initialData,
    };
    HR_ASSERT(device->CreateBuffer(&bufferDesc, &initialDataDesc, &buffer));
  } else {
    HR_ASSERT(device->CreateBuffer(&bufferDesc, NULL, &buffer));
  }

  return buffer;
}

typedef struct _smkFileData {
  void *data;
  int size;
} smkFileData;

static smkFileData readFile(const String *filePath) {
  smkFileData fileData = {};

  HANDLE f = CreateFileA(filePath->buf, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL, 0);
  ASSERT(f != INVALID_HANDLE_VALUE);
  DWORD fileSize = GetFileSize(f, NULL);
  fileData.data = MMALLOC_ARRAY_UNINITIALIZED(uint8_t, castU32I32(fileSize));
  DWORD bytesRead;
  ReadFile(f, fileData.data, fileSize, &bytesRead, NULL);
  ASSERT(fileSize == bytesRead);
  fileData.size = castU32I32(bytesRead);
  CloseHandle(f);

  return fileData;
}

static void destroyFileData(smkFileData *fileData) {
  MFREE(fileData->data);
  *fileData = {};
}

smkShaderProgram loadProgramFromShaderAsset(ID3D11Device *device,
                                            const char *assetPath) {
  ASSERT(device);

  smkShaderProgram program = {};

  String basePath = {};
  copyShaderRootPath(&basePath);

  String vertexShaderPath = {};
  copyString(&vertexShaderPath, &basePath);
  appendPathCStr(&vertexShaderPath, assetPath);
  appendCStr(&vertexShaderPath, ".vs.cso");

  LOG("Opening vertex shader at %s", vertexShaderPath.buf);
  smkFileData vertexShaderSource = readFile(&vertexShaderPath);

  String fragmentShaderPath = {};
  copyString(&fragmentShaderPath, &basePath);
  appendPathCStr(&fragmentShaderPath, assetPath);
  appendCStr(&fragmentShaderPath, ".fs.cso");

  LOG("Opening fragment shader at %s", fragmentShaderPath.buf);
  smkFileData fragmentShaderSource = readFile(&fragmentShaderPath);

  {
    HR_ASSERT(device->CreateVertexShader(vertexShaderSource.data,
                                         castI32Usize(vertexShaderSource.size),
                                         NULL, &program.vertexShader));

    ID3D11ShaderReflection *refl;
    HR_ASSERT(D3DReflect(vertexShaderSource.data,
                         castI32Usize(vertexShaderSource.size),
                         IID_PPV_ARGS(&refl)));

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

    HR_ASSERT(device->CreateInputLayout(
        inputAttribs, shaderDesc.InputParameters, vertexShaderSource.data,
        castI32Usize(vertexShaderSource.size), &program.vertexLayout));

    COM_RELEASE(refl);
    MFREE(inputAttribs);

    HR_ASSERT(device->CreatePixelShader(fragmentShaderSource.data,
                                        castI32Usize(fragmentShaderSource.size),
                                        NULL, &program.fragmentShader));
  }

  destroyFileData(&fragmentShaderSource);
  destroyString(&fragmentShaderPath);
  destroyFileData(&vertexShaderSource);
  destroyString(&vertexShaderPath);
  destroyString(&basePath);

  return program;
}

smkRenderer smkCreateRenderer(void) {
  ASSERT(!gIsRendererInitialized);

  smkRenderer renderer = {};

#ifdef DEBUG
  if (isDeveloperModeEnabled()) {
    HR_ASSERT(
        D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_12_0,
                          IID_PPV_ARGS(&renderer.dummyDeviceForFixedGPUClock)));
    HR_ASSERT(renderer.dummyDeviceForFixedGPUClock->SetStablePowerState(TRUE));
  } else {
    LOG("Warning: Developer mode is not enabled. GPU profiler result will be "
        "inconsistent");
  }
#endif

  int windowWidth, windowHeight;
  SDL_GetWindowSize(gApp.window, &windowWidth, &windowHeight);

  DXGI_SWAP_CHAIN_DESC swapChainDesc = {
      .BufferDesc =
          {
              .Width = castI32U32(windowWidth),
              .Height = castI32U32(windowHeight),
              .RefreshRate = queryRefreshRate(windowWidth, windowHeight,
                                              DXGI_FORMAT_R8G8B8A8_UNORM),
              .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
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
      D3D11_SDK_VERSION, &swapChainDesc, &renderer.swapChain, &renderer.device,
      NULL, &renderer.context));

  ID3D11Texture2D *backBuffer;
  HR_ASSERT(renderer.swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));
  HR_ASSERT(renderer.device->CreateRenderTargetView(backBuffer, NULL,
                                                    &renderer.swapChainRTV));
  COM_RELEASE(backBuffer);

  renderer.swapChainDepthTexture = createTexture2D(
      renderer.device, renderer.context, windowWidth, windowHeight,
      DXGI_FORMAT_R32_TYPELESS, D3D11_USAGE_DEFAULT,
      GPUResourceBindBits_DEPTH_STENCIL | GPUResourceBindBits_SHADER_RESOURCE,
      false, NULL);

  D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {
      .Format = DXGI_FORMAT_D32_FLOAT,
      .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
      .Flags = 0,
      .Texture2D = {.MipSlice = 0},
  };
  HR_ASSERT(renderer.device->CreateDepthStencilView(
      renderer.swapChainDepthTexture.handle, &depthStencilViewDesc,
      &renderer.swapChainDSV));

  uint32_t white = 0xffffffff;
  renderer.defaultTexture = createTexture2D(
      renderer.device, renderer.context, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM,
      D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE, false, &white);

  renderer.defaultSampler =
      createSampler(renderer.device, D3D11_FILTER_MIN_MAG_MIP_LINEAR,
                    D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);

  D3D11_DEPTH_STENCIL_DESC depthStencilStateDesc = {
      .DepthEnable = TRUE,
      .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
      .DepthFunc = D3D11_COMPARISON_GREATER_EQUAL,
      .StencilEnable = FALSE,
  };
  HR_ASSERT(renderer.device->CreateDepthStencilState(
      &depthStencilStateDesc, &renderer.defaultDepthStencilState));

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
  HR_ASSERT(renderer.device->CreateRasterizerState(
      &rasterizerStateDesc, &renderer.defaultRasterizerState));

  renderer.viewUniformBuffer =
      createBuffer(renderer.device, sizeof(smkViewUniforms), NULL,
                   D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER);

  renderer.materialUniformBuffer =
      createBuffer(renderer.device, sizeof(smkMaterialUniforms), NULL,
                   D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER);

  renderer.drawUniformBuffer =
      createBuffer(renderer.device, sizeof(smkDrawUniforms), NULL,
                   D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER);

  renderer.cameraVolumeVertexBuffer =
      createBuffer(renderer.device, sizeof(smkCameraVolumeVertexData), NULL,
                   D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER);

  renderer.forwardPBRProgram =
      loadProgramFromShaderAsset(renderer.device, "forward_pbr");
  renderer.debugProgram = loadProgramFromShaderAsset(renderer.device, "debug");

#ifdef DEBUG
  D3D11_QUERY_DESC queryDesc = {};
  queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
  HR_ASSERT(renderer.device->CreateQuery(&queryDesc,
                                         &renderer.profilerDisjointQuery));

  queryDesc.Query = D3D11_QUERY_TIMESTAMP;
  HR_ASSERT(renderer.device->CreateQuery(
      &queryDesc, &renderer.profilerTimestampBeginQuery));
  HR_ASSERT(renderer.device->CreateQuery(&queryDesc,
                                         &renderer.profilerTimestampEndQuery));
#endif

  gIsRendererInitialized = true;

  return renderer;
}

void smkDestroyRenderer(smkRenderer *renderer) {
#ifdef DEBUG
  COM_RELEASE(renderer->profilerTimestampEndQuery);
  COM_RELEASE(renderer->profilerTimestampBeginQuery);
  COM_RELEASE(renderer->profilerDisjointQuery);
#endif

  smkDestroyProgram(&renderer->debugProgram);
  smkDestroyProgram(&renderer->forwardPBRProgram);

  COM_RELEASE(renderer->cameraVolumeVertexBuffer);
  COM_RELEASE(renderer->drawUniformBuffer);
  COM_RELEASE(renderer->materialUniformBuffer);
  COM_RELEASE(renderer->viewUniformBuffer);

  COM_RELEASE(renderer->defaultRasterizerState);
  COM_RELEASE(renderer->defaultDepthStencilState);

  COM_RELEASE(renderer->defaultSampler);
  smkDestroyTexture(&renderer->defaultTexture);

  COM_RELEASE(renderer->swapChainDSV);
  smkDestroyTexture(&renderer->swapChainDepthTexture);
  COM_RELEASE(renderer->swapChainRTV);
  COM_RELEASE(renderer->swapChain);

  COM_RELEASE(renderer->context);
  COM_RELEASE(renderer->device);

#ifdef DEBUG
  HR_ASSERT(renderer->dummyDeviceForFixedGPUClock->SetStablePowerState(FALSE));
  COM_RELEASE(renderer->dummyDeviceForFixedGPUClock);
#endif

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
    HR_ASSERT(debug->ReportLiveObjects(
        DXGI_DEBUG_ALL,
        (DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_ALL |
                               DXGI_DEBUG_RLO_IGNORE_INTERNAL)));

    FreeLibrary(dxgidebugLibrary);
    COM_RELEASE(debug);
  }
#endif

  *renderer = {};

  gIsRendererInitialized = false;
}

smkShaderProgram smkLoadProgramFromShaderAsset(smkRenderer *renderer,
                                               const char *assetPath) {
  ASSERT(renderer);
  smkShaderProgram program =
      loadProgramFromShaderAsset(renderer->device, assetPath);
  return program;
}

void smkDestroyProgram(smkShaderProgram *program) {
  COM_RELEASE(program->fragmentShader);
  COM_RELEASE(program->vertexLayout);
  COM_RELEASE(program->vertexShader);
  *program = {};
}

void smkUseProgram(smkRenderer *renderer, smkShaderProgram *program) {
  ASSERT(renderer && program);
  renderer->context->IASetInputLayout(program->vertexLayout);
  renderer->context->VSSetShader(program->vertexShader, NULL, 0);
  renderer->context->PSSetShader(program->fragmentShader, NULL, 0);
}

smkTexture smkCreateTexture2D(smkRenderer *renderer, int width, int height,
                              DXGI_FORMAT format, D3D11_USAGE usage,
                              uint32_t bindFlags, bool generateMipMaps,
                              void *initialData) {
  ASSERT(renderer);

  smkTexture texture =
      createTexture2D(renderer->device, renderer->context, width, height,
                      format, usage, bindFlags, generateMipMaps, initialData);
  return texture;
}

void smkDestroyTexture(smkTexture *texture) {
  COM_RELEASE(texture->view);
  COM_RELEASE(texture->handle);

  *texture = {};
}

smkScene smkLoadSceneFromGLTFAsset(smkRenderer *renderer,
                                   const char *assetPath) {
  ASSERT(renderer);

  smkScene scene = {};

  String filePath = {};
  String basePath = {};
  {
    String pathString = {};
    copyStringFromCStr(&pathString, assetPath);

    copyAssetRootPath(&filePath);

    if (endsWithCString(&pathString, ".glb")) {
      appendPathCStr(&filePath, pathString.buf);
    } else {
      const char *baseName = pathBaseName(&pathString);
      appendPathCStr(&filePath, assetPath);
      copyString(&basePath, &filePath);
      appendPathCStr(&filePath, baseName);
      appendCStr(&filePath, ".gltf");
    }
    LOG("Loading GLTF: %s", filePath.buf);
    destroyString(&pathString);
  }

  cgltf_options options = {};
  cgltf_data *gltf;
  cgltf_result gltfLoadResult = cgltf_parse_file(&options, filePath.buf, &gltf);
  ASSERT(gltfLoadResult == cgltf_result_success);
  cgltf_load_buffers(&options, gltf, filePath.buf);

  scene.numTextures = castUsizeI32(gltf->textures_count);
  if (scene.numTextures > 0) {
    scene.textures = MMALLOC_ARRAY(smkTexture, scene.numTextures);

    String imageFilePath = {};
    for (cgltf_size textureIndex = 0; textureIndex < gltf->images_count;
         ++textureIndex) {
      cgltf_image *gltfImage = &gltf->images[textureIndex];
      int w, h, numComponents;
      stbi_uc *data;
      if (gltfImage->buffer_view) {
        data = stbi_load_from_memory(
            (uint8_t *)gltfImage->buffer_view->buffer->data +
                gltfImage->buffer_view->offset,
            castUsizeI32(gltfImage->buffer_view->size), &w, &h, &numComponents,
            STBI_rgb_alpha);
      } else {
        copyString(&imageFilePath, &basePath);
        appendPathCStr(&imageFilePath, gltfImage->uri);
        data = stbi_load(imageFilePath.buf, &w, &h, &numComponents,
                         STBI_rgb_alpha);
      }
      ASSERT(data);

      scene.textures[textureIndex] = smkCreateTexture2D(
          renderer, w, h, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_USAGE_DEFAULT,
          D3D11_BIND_SHADER_RESOURCE, true, data);

      stbi_image_free(data);
    }

    destroyString(&imageFilePath);
  }

#define GLTF_NEAREST 9728
#define GLTF_LINEAR 9729
#define GLTF_NEAREST_MIPMAP_NEAREST 9984
#define GLTF_LINEAR_MIPMAP_NEAREST 9985
// #define GLTF_NEAREST_MIPMAP_LINEAR 9986
#define GLTF_LINEAR_MIPMAP_LINEAR 9987
#define GLTF_CLAMP_TO_EDGE 33071
#define GLTF_MIRRORED_REPEAT 33648
#define GLTF_REPEAT 10497
  scene.numSamplers = castUsizeI32(gltf->samplers_count);
  if (scene.numSamplers > 0) {
    scene.samplers = MMALLOC_ARRAY(ID3D11SamplerState *, scene.numSamplers);

    for (cgltf_size samplerIndex = 0; samplerIndex < gltf->samplers_count;
         ++samplerIndex) {
      cgltf_sampler *gltfSampler = &gltf->samplers[samplerIndex];

      D3D11_FILTER filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
      D3D11_TEXTURE_ADDRESS_MODE addressModeU = D3D11_TEXTURE_ADDRESS_WRAP;
      D3D11_TEXTURE_ADDRESS_MODE addressModeV = D3D11_TEXTURE_ADDRESS_WRAP;

      if (gltfSampler->mag_filter == GLTF_NEAREST) {
        if (gltfSampler->min_filter == GLTF_NEAREST) {
          filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        } else if (gltfSampler->min_filter == GLTF_LINEAR) {
          filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        } else if (gltfSampler->min_filter == GLTF_NEAREST_MIPMAP_NEAREST) {
          filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        } else if (gltfSampler->min_filter == GLTF_LINEAR_MIPMAP_NEAREST) {
          filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        } else if (gltfSampler->min_filter == GLTF_LINEAR_MIPMAP_LINEAR) {
          filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        } else { // NEAREST_MIPMAP_LINEAR
          filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
        }
      } else { // LINEAR
        if (gltfSampler->min_filter == GLTF_NEAREST) {
          filter = D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
        } else if (gltfSampler->min_filter == GLTF_LINEAR) {
          filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        } else if (gltfSampler->min_filter == GLTF_NEAREST_MIPMAP_NEAREST) {
          filter = D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
        } else if (gltfSampler->min_filter == GLTF_LINEAR_MIPMAP_NEAREST) {
          filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        } else if (gltfSampler->min_filter == GLTF_LINEAR_MIPMAP_LINEAR) {
          filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        } else { // NEAREST_MIPMAP_LINEAR
          filter = D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
        }
      }

      switch (gltfSampler->wrap_s) {
      case GLTF_CLAMP_TO_EDGE:
        addressModeU = D3D11_TEXTURE_ADDRESS_CLAMP;
        break;
      case GLTF_MIRRORED_REPEAT:
        addressModeU = D3D11_TEXTURE_ADDRESS_MIRROR;
        break;
      case GLTF_REPEAT:
      default:
        addressModeU = D3D11_TEXTURE_ADDRESS_WRAP;
        break;
      }
      switch (gltfSampler->wrap_t) {
      case GLTF_CLAMP_TO_EDGE:
        addressModeV = D3D11_TEXTURE_ADDRESS_CLAMP;
        break;
      case GLTF_MIRRORED_REPEAT:
        addressModeV = D3D11_TEXTURE_ADDRESS_MIRROR;
        break;
      case GLTF_REPEAT:
      default:
        addressModeV = D3D11_TEXTURE_ADDRESS_WRAP;
        break;
      }

      scene.samplers[samplerIndex] =
          createSampler(renderer->device, filter, addressModeU, addressModeV);
    }
  }

#undef GLTF_NEAREST
#undef GLTF_LINEAR
#undef GLTF_NEAREST_MIPMAP_NEAREST
#undef GLTF_LINEAR_MIPMAP_NEAREST
// #undef GLTF_NEAREST_MIPMAP_LINEAR
#undef GLTF_LINEAR_MIPMAP_LINEAR
#undef GLTF_CLAMP_TO_EDGE
#undef GLTF_MIRRORED_REPEAT
#undef GLTF_REPEAT

  scene.numMaterials = castUsizeI32(gltf->materials_count);
  if (scene.numMaterials > 0) {
    scene.materials = MMALLOC_ARRAY(smkMaterial, scene.numMaterials);
    for (cgltf_size materialIndex = 0; materialIndex < gltf->materials_count;
         ++materialIndex) {
      cgltf_material *gltfMaterial = &gltf->materials[materialIndex];
      smkMaterial *material = &scene.materials[materialIndex];
      material->baseColorTexture = -1;
      material->baseColorSampler = -1;
      material->metallicRoughnessTexture = -1;
      material->metallicRoughnessSampler = -1;
      material->normalTexture = -1;
      material->normalSampler = -1;
      material->occlusionTexture = -1;
      material->occlusionSampler = -1;
      material->emissiveTexture = -1;
      material->emissiveSampler = -1;

      ASSERT(gltfMaterial->has_pbr_metallic_roughness);

      cgltf_pbr_metallic_roughness *pbrMR =
          &gltfMaterial->pbr_metallic_roughness;

      memcpy(&material->baseColorFactor, pbrMR->base_color_factor,
             sizeof(Float4));

      if (pbrMR->base_color_texture.texture) {
        material->baseColorTexture =
            castSsizeI32(gltfMaterial->pbr_metallic_roughness.base_color_texture
                             .texture->image -
                         gltf->images);

        if (pbrMR->base_color_texture.texture->sampler) {
          material->baseColorSampler =
              castSsizeI32(gltfMaterial->pbr_metallic_roughness
                               .base_color_texture.texture->sampler -
                           gltf->samplers);
        }
      }

      if (pbrMR->metallic_roughness_texture.texture) {
        material->metallicRoughnessTexture = castSsizeI32(
            pbrMR->metallic_roughness_texture.texture->image - gltf->images);
        if (pbrMR->metallic_roughness_texture.texture->sampler) {
          material->metallicRoughnessSampler =
              castSsizeI32(pbrMR->metallic_roughness_texture.texture->sampler -
                           gltf->samplers);
        }
      }

      material->metallicFactor = pbrMR->metallic_factor;
      material->roughnessFactor = pbrMR->roughness_factor;
    }
  }

  scene.numMeshes = castUsizeI32(gltf->meshes_count);
  if (scene.numMeshes > 0) {
    scene.meshes = MMALLOC_ARRAY(smkMesh, scene.numMeshes);
    for (cgltf_size meshIndex = 0; meshIndex < gltf->meshes_count;
         ++meshIndex) {
      cgltf_mesh *gltfMesh = &gltf->meshes[meshIndex];
      smkMesh *mesh = &scene.meshes[meshIndex];

      mesh->numSubMeshes = castUsizeI32(gltfMesh->primitives_count);
      mesh->subMeshes = MMALLOC_ARRAY(smkSubMesh, mesh->numSubMeshes);

      for (cgltf_size primIndex = 0; primIndex < gltfMesh->primitives_count;
           ++primIndex) {
        cgltf_primitive *prim = &gltfMesh->primitives[primIndex];

        smkSubMesh *subMesh = &mesh->subMeshes[primIndex];
        subMesh->indexBase = mesh->numIndices;
        subMesh->numIndices = castUsizeI32(prim->indices->count);
        mesh->numIndices += subMesh->numIndices;

        uint32_t maxIndex = 0;
        for (cgltf_size i = 0; i < prim->indices->count; ++i) {
          VertexIndex index =
              castUsizeU32(cgltf_accessor_read_index(prim->indices, i));
          if (maxIndex < index) {
            maxIndex = index;
          }
        }

        subMesh->vertexBase = mesh->numVertices;
        subMesh->numVertices = castU32I32(maxIndex + 1);
        mesh->numVertices += subMesh->numVertices;

        subMesh->material = castSsizeI32(prim->material - gltf->materials);
      }

      int bufferSize = mesh->numVertices * SSIZEOF32(smkVertex) +
                       mesh->numIndices * SSIZEOF32(uint32_t);
      mesh->bufferMemory = MMALLOC_ARRAY(uint8_t, bufferSize);

      mesh->vertices = (smkVertex *)mesh->bufferMemory;
      for (int vertexIndex = 0; vertexIndex < mesh->numVertices;
           ++vertexIndex) {
        mesh->vertices[vertexIndex].color = float4(1, 1, 1, 1);
      }

      mesh->indices = (uint32_t *)(mesh->bufferMemory +
                                   mesh->numVertices * sizeof(smkVertex));

      uint32_t indexBase = 0;
      uint32_t vertexBase = 0;

      for (cgltf_size primIndex = 0; primIndex < gltfMesh->primitives_count;
           ++primIndex) {
        cgltf_primitive *prim = &gltfMesh->primitives[primIndex];
        smkSubMesh *subMesh = &mesh->subMeshes[primIndex];

        for (cgltf_size indexIndex = 0; indexIndex < prim->indices->count;
             ++indexIndex) {
          mesh->indices[indexBase + indexIndex] = castUsizeU32(
              cgltf_accessor_read_index(prim->indices, indexIndex));
        }

        indexBase += castI32U32(subMesh->numIndices);

        for (cgltf_size attributeIndex = 0;
             attributeIndex < prim->attributes_count; ++attributeIndex) {
          cgltf_attribute *attribute = &prim->attributes[attributeIndex];
          ASSERT(!attribute->data->is_sparse); // Sparse is not supported yet
          switch (attribute->type) {
          case cgltf_attribute_type_position:
            for (cgltf_size vertexIndex = 0;
                 vertexIndex < attribute->data->count; ++vertexIndex) {
              cgltf_size numComponents =
                  cgltf_num_components(attribute->data->type);
              cgltf_bool readResult = cgltf_accessor_read_float(
                  attribute->data, vertexIndex,
                  (float *)&mesh->vertices[vertexBase + vertexIndex].position,
                  numComponents);
              ASSERT(readResult);
            }
            break;
          case cgltf_attribute_type_color:
            for (cgltf_size vertexIndex = 0;
                 vertexIndex < attribute->data->count; ++vertexIndex) {
              cgltf_size numComponents =
                  cgltf_num_components(attribute->data->type);
              cgltf_bool readResult = cgltf_accessor_read_float(
                  attribute->data, vertexIndex,
                  (float *)&mesh->vertices[vertexBase + vertexIndex].color,
                  numComponents);
              ASSERT(readResult);
            }
            break;
          case cgltf_attribute_type_texcoord:
            for (cgltf_size vertexIndex = 0;
                 vertexIndex < attribute->data->count; ++vertexIndex) {
              cgltf_size numComponents =
                  cgltf_num_components(attribute->data->type);
              cgltf_bool readResult = cgltf_accessor_read_float(
                  attribute->data, vertexIndex,
                  (float *)&mesh->vertices[vertexBase + vertexIndex].texCoord,
                  numComponents);
              ASSERT(readResult);
            }
            break;
          case cgltf_attribute_type_normal:
            for (cgltf_size vertexIndex = 0;
                 vertexIndex < attribute->data->count; ++vertexIndex) {
              cgltf_size numComponents =
                  cgltf_num_components(attribute->data->type);
              cgltf_bool readResult = cgltf_accessor_read_float(
                  attribute->data, vertexIndex,
                  (float *)&mesh->vertices[vertexBase + vertexIndex].normal,
                  numComponents);
              ASSERT(readResult);
            }
            break;
          default:
            break;
          }
        }

        vertexBase += castI32U32(subMesh->numVertices);
      }

      mesh->gpuBuffer =
          createBuffer(renderer->device, bufferSize, mesh->bufferMemory,
                       D3D11_USAGE_IMMUTABLE,
                       D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_INDEX_BUFFER);
    }
  }

  scene.numEntities = castUsizeI32(gltf->nodes_count + gltf->scenes_count);
  if (scene.numEntities > 0) {
    scene.entities = MMALLOC_ARRAY(smkEntity, scene.numEntities);
    for (cgltf_size nodeIndex = 0; nodeIndex < gltf->nodes_count; ++nodeIndex) {
      cgltf_node *gltfNode = &gltf->nodes[nodeIndex];
      smkEntity *entity = &scene.entities[nodeIndex];
      if (gltfNode->name) {
        copyStringFromCStr(&entity->name, gltfNode->name);
      } else {
        FORMAT_STRING(&entity->name, "Node %zu", nodeIndex);
      }

      cgltf_node_transform_local(gltfNode,
                                 (float *)entity->localTransform.matrix.cols);
      cgltf_node_transform_world(gltfNode,
                                 (float *)entity->worldTransform.matrix.cols);

      if (gltfNode->parent) {
        entity->parent = castSsizeI32(gltfNode->parent - gltf->nodes);
      } else {
        entity->parent = -1;
      }

      if (gltfNode->mesh) {
        entity->mesh = castSsizeI32(gltfNode->mesh - gltf->meshes);
      } else {
        entity->mesh = -1;
      }

      if (gltfNode->children_count > 0) {
        entity->numChildren = castUsizeI32(gltfNode->children_count);
        entity->children = MMALLOC_ARRAY(int, entity->numChildren);
        for (cgltf_size childIndex = 0; childIndex < gltfNode->children_count;
             ++childIndex) {
          entity->children[childIndex] =
              castSsizeI32(gltfNode->children[childIndex] - gltf->nodes);
        }
      }
    }
  }

  for (cgltf_size sceneIndex = 0; sceneIndex < gltf->scenes_count;
       ++sceneIndex) {
    cgltf_scene *gltfScene = &gltf->scenes[sceneIndex];
    smkEntity *entity = &scene.entities[gltf->nodes_count + sceneIndex];
    if (gltfScene->name) {
      copyStringFromCStr(&entity->name, gltfScene->name);
    } else {
      FORMAT_STRING(&entity->name, "Scene %zu", sceneIndex);
    }
    entity->parent = -1;
    entity->localTransform.matrix = mat4Identity();
    entity->worldTransform.matrix = mat4Identity();
    entity->mesh = -1;

    if (gltfScene->nodes_count > 0) {
      entity->numChildren = castUsizeI32(gltfScene->nodes_count);
      entity->children = MMALLOC_ARRAY(int, entity->numChildren);

      for (cgltf_size nodeIndex = 0; nodeIndex < gltfScene->nodes_count;
           ++nodeIndex) {
        cgltf_node *gltfNode = gltfScene->nodes[nodeIndex];
        entity->children[nodeIndex] = castSsizeI32(gltfNode - gltf->nodes);
      }
    }
  }

  cgltf_free(gltf);
  destroyString(&basePath);
  destroyString(&filePath);

  return scene;
}

void smkDestroyScene(smkScene *scene) {
  for (int i = 0; i < scene->numEntities; ++i) {
    smkEntity *entity = &scene->entities[i];
    MFREE(entity->children);
    destroyString(&entity->name);
  }
  MFREE(scene->entities);

  for (int i = 0; i < scene->numMeshes; ++i) {
    smkMesh *mesh = &scene->meshes[i];
    MFREE(mesh->subMeshes);
    COM_RELEASE(mesh->gpuBuffer);
    MFREE(mesh->bufferMemory);
  }
  MFREE(scene->meshes);

  MFREE(scene->materials);

  for (int i = 0; i < scene->numSamplers; ++i) {
    ID3D11SamplerState *sampler = scene->samplers[i];
    COM_RELEASE(sampler);
  }
  MFREE(scene->samplers);

  for (int i = 0; i < scene->numTextures; ++i) {
    smkTexture *texture = &scene->textures[i];
    smkDestroyTexture(texture);
  }
  MFREE(scene->textures);

  *scene = {};
}

smkScene smkMergeScene(const smkScene *a, const smkScene *b) {}

static void smkRenderMesh(smkRenderer *renderer, const smkScene *scene,
                          const smkMesh *mesh) {
  ASSERT(renderer);

  UINT vertexStride = sizeof(smkVertex);
  UINT vertexOffset = 0;
  renderer->context->IASetVertexBuffers(0, 1, &mesh->gpuBuffer, &vertexStride,
                                        &vertexOffset);
  UINT indexOffset =
      castUsizeU32(castI32Usize(mesh->numVertices) * sizeof(smkVertex));
  renderer->context->IASetIndexBuffer(mesh->gpuBuffer, DXGI_FORMAT_R32_UINT,
                                      indexOffset);

  for (int i = 0; i < mesh->numSubMeshes; ++i) {
    const smkSubMesh *subMesh = &mesh->subMeshes[i];
    const smkMaterial *material = &scene->materials[subMesh->material];

    smkMaterialUniforms materialUniforms = {
        .baseColorFactor = material->baseColorFactor,
        .metallicRoughnessFactor = {material->metallicFactor,
                                    material->roughnessFactor},
    };

    smkTexture *baseColorTexture = &renderer->defaultTexture;
    if (material->baseColorTexture >= 0) {
      baseColorTexture = &scene->textures[material->baseColorTexture];
    }

    ID3D11SamplerState *baseColorSampler = renderer->defaultSampler;
    if (material->baseColorSampler >= 0) {
      baseColorSampler = scene->samplers[material->baseColorSampler];
    }

    smkTexture *metallicRoughnessTexture = &renderer->defaultTexture;
    if (material->metallicRoughnessTexture >= 0) {
      metallicRoughnessTexture =
          &scene->textures[material->metallicRoughnessTexture];
    }

    ID3D11SamplerState *metallicRoughnessSampler = renderer->defaultSampler;
    if (material->metallicRoughnessSampler >= 0) {
      metallicRoughnessSampler =
          scene->samplers[material->metallicRoughnessSampler];
    }

    renderer->context->UpdateSubresource(renderer->materialUniformBuffer, 0,
                                         NULL, &materialUniforms, 0, 0);

    ID3D11ShaderResourceView *textureViews[] = {baseColorTexture->view,
                                                metallicRoughnessTexture->view};
    ID3D11SamplerState *samplers[] = {baseColorSampler,
                                      metallicRoughnessSampler};

    renderer->context->PSSetShaderResources(2, ARRAY_SIZE(textureViews),
                                            textureViews);
    renderer->context->PSSetSamplers(2, ARRAY_SIZE(samplers), samplers);
    renderer->context->DrawIndexed(castI32U32(subMesh->numIndices),
                                   castI32U32(subMesh->indexBase),
                                   subMesh->vertexBase);
  }
}

void smkRenderScene(smkRenderer *renderer, const smkScene *scene) {
  ASSERT(renderer);

  renderer->context->IASetPrimitiveTopology(
      D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  for (int i = 0; i < scene->numEntities; ++i) {
    const smkEntity *entity = &scene->entities[i];
    if (entity->mesh >= 0) {
      smkDrawUniforms drawUniforms = {};
      drawUniforms.modelMat = entity->worldTransform.matrix;
      drawUniforms.invModelMat = mat4Inverse(entity->worldTransform.matrix);
      renderer->context->UpdateSubresource(renderer->drawUniformBuffer, 0, NULL,
                                           &drawUniforms, 0, 0);

      smkMesh *mesh = &scene->meshes[entity->mesh];
      smkRenderMesh(renderer, scene, mesh);
    }
  }
}

void smkSubmitRenderCommands(smkRenderer *renderer, int numRenderCommands,
                             smkRenderCommand *renderCommands) {
  ASSERT(renderer);

  // renderer->context->OMSetRenderTargets(1, &renderer->swapChainRTV,
  //                                       renderer->swapChainDSV);
  // renderer->context->ClearRenderTargetView(renderer->swapChainRTV,
  //                                          const FLOAT *ColorRGBA)

  //     smkRenderCommand *lastCommand = NULL;
  // for (int i = 0; i < numRenderCommands; ++i) {
  //   smkRenderCommand *command = &renderCommands[i];
  //   if (!lastCommand || (lastCommand->viewCamera != command->viewCamera)) {
  //     smkViewUniforms viewUniforms = {};
  //     viewUniforms.viewMat = getViewMatrix(command->viewCamera);
  //     viewUniforms.projMat = mat4Perspective(
  //         command->viewCamera->verticalFovDeg,
  //         command->viewCamera->aspectRatio, command->viewCamera->nearZ,
  //         command->viewCamera->farZ);
  //   }

  //   smkRenderScene(renderer, command->scene);

  //   lastCommand = command;
  // }
}

void smkInitGUI(smkRenderer *renderer) {
  ImGui::CreateContext();
  ImGui::StyleColorsLight();
  ImGui_ImplSDL2_InitForD3D(gApp.window);
  ImGui_ImplDX11_Init(renderer->device, renderer->context);
  ImGui::GetStyle().Alpha = 0.6f;

  ImVec4 *colors = ImGui::GetStyle().Colors;
  colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.00f, 1.00f, 0.00f, 0.57f);
  colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
  // colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
  colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
  colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.60f);
  // colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.39f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 0.67f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 1.00f, 0.00f, 0.78f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(1.00f, 1.00f, 1.00f, 0.57f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.00f, 1.00f, 0.00f, 0.57f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
  colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
  colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.57f);
  colors[ImGuiCol_Button] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.57f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
  colors[ImGuiCol_Header] = ImVec4(0.00f, 1.00f, 0.00f, 0.78f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.39f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 1.00f, 0.00f, 0.57f);
  colors[ImGuiCol_Separator] = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(0.72f, 0.72f, 0.72f, 0.78f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
  colors[ImGuiCol_ResizeGrip] = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);
  colors[ImGuiCol_Tab] = ImVec4(0.24f, 0.79f, 0.69f, 1.00f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
}

void smkDestroyGUI(void) {
  ImGui_ImplDX11_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
}

C_INTERFACE_END