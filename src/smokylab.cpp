#include "smokylab.h"
#include "util.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <SDL2/SDL_syswm.h>
#include <d3d11shader.h>
#pragma clang diagnostic pop

ID3D11Device *gDevice;
ID3D11DeviceContext *gContext;
IDXGISwapChain *gSwapChain;
ID3D11RenderTargetView *gSwapChainRTV;
ID3D11Texture2D *gSwapChainDepthStencilBuffer;
ID3D11DepthStencilView *gSwapChainDSV;

DXGI_RATIONAL queryRefreshRate(int ww, int wh, DXGI_FORMAT swapChainFormat) {
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

static void readFile(const String *filePath, void **data, int *size) {
  HANDLE f = CreateFileA(filePath->buf, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL, 0);
  ASSERT(f != INVALID_HANDLE_VALUE);
  DWORD fileSize = GetFileSize(f, NULL);
  *data = MMALLOC_ARRAY_UNINITIALIZED(uint8_t, fileSize);
  DWORD bytesRead;
  ReadFile(f, *data, fileSize, &bytesRead, NULL);
  ASSERT(fileSize == bytesRead);
  *size = (int)bytesRead;
  CloseHandle(f);
}

void createProgram(const char *baseName, ShaderProgram *program) {
  String basePath = {};
  copyBasePath(&basePath);

  {
    String vertPath = {};
    copyString(&vertPath, &basePath);
    appendPathCStr(&vertPath, baseName);
    appendCStr(&vertPath, ".vs.cso");

    void *vertSrc;
    int vertSize;
    readFile(&vertPath, &vertSrc, &vertSize);
    HR_ASSERT(gDevice->CreateVertexShader(vertSrc, castI32U32(vertSize), NULL,
                                          &program->vert));

    ID3D11ShaderReflection *refl;
    HR_ASSERT(D3DReflect(vertSrc, castI32U32(vertSize), IID_PPV_ARGS(&refl)));

    D3D11_SHADER_DESC shaderDesc;
    HR_ASSERT(refl->GetDesc(&shaderDesc));

    D3D11_INPUT_ELEMENT_DESC *inputAttribs =
        MMALLOC_ARRAY(D3D11_INPUT_ELEMENT_DESC, shaderDesc.InputParameters);

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
        inputAttribs, shaderDesc.InputParameters, vertSrc, castI32U32(vertSize),
        &program->inputLayout));

    MFREE(inputAttribs);

    MFREE(vertSrc);

    destroyString(&vertPath);
  }

  {
    String fragPath = {};
    copyString(&fragPath, &basePath);
    appendPathCStr(&fragPath, baseName);
    appendCStr(&fragPath, ".fs.cso");

    void *fragSrc;
    int fragSize;
    readFile(&fragPath, &fragSrc, &fragSize);
    HR_ASSERT(gDevice->CreatePixelShader(fragSrc, castI32U32(fragSize), NULL,
                                         &program->frag));
    MFREE(fragSrc);

    destroyString(&fragPath);
  }

  destroyString(&basePath);
}

void destroyProgram(ShaderProgram *program) {
  COM_RELEASE(program->frag);
  COM_RELEASE(program->vert);
  COM_RELEASE(program->inputLayout);
  *program = {};
}

void useProgram(const ShaderProgram *program) {
  gContext->IASetInputLayout(program->inputLayout);
  gContext->VSSetShader(program->vert, NULL, 0);
  gContext->PSSetShader(program->frag, NULL, 0);
}
