#include "smokylab.h"
#include "util.h"
#include "asset.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <cgltf.h>
#include <stb_image.h>
#include <SDL2/SDL_syswm.h>
#include <d3d11shader.h>
#pragma clang diagnostic pop
#include <random>

ID3D11Device *gDevice;
ID3D11DeviceContext *gContext;
IDXGISwapChain *gSwapChain;
ID3D11RenderTargetView *gSwapChainRTV;
ID3D11Texture2D *gSwapChainDepthStencilBuffer;
ID3D11DepthStencilView *gSwapChainDSV;

ID3D11Texture2D *gDefaultTexture;
ID3D11ShaderResourceView *gDefaultTextureView;
ID3D11SamplerState *gDefaultSampler;

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

HWND getWin32WindowHandle(SDL_Window *window) {
  SDL_SysWMinfo wmInfo;
  SDL_VERSION(&wmInfo.version)
  SDL_GetWindowWMInfo(window, &wmInfo);
  HWND hwnd = wmInfo.info.win.window;
  return hwnd;
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

void createBuffer(const BufferDesc *desc, ID3D11Buffer **buffer) {
  D3D11_BUFFER_DESC bufferDesc = {
      .ByteWidth = castI32U32(desc->size),
      .Usage = desc->usage,
      .BindFlags = desc->bindFlags,
  };

  if (desc->initialData) {
    D3D11_SUBRESOURCE_DATA initialData = {
        .pSysMem = desc->initialData,
    };
    HR_ASSERT(gDevice->CreateBuffer(&bufferDesc, &initialData, buffer));
  } else {
    HR_ASSERT(gDevice->CreateBuffer(&bufferDesc, NULL, buffer));
  }
}

void createTexture2D(const TextureDesc *desc, ID3D11Texture2D **texture,
                     ID3D11ShaderResourceView **textureView) {
  D3D11_TEXTURE2D_DESC textureDesc = {
      .Width = castI32U32(desc->width),
      .Height = castI32U32(desc->height),
      .MipLevels = 1,
      .ArraySize = 1,
      .Format = desc->format,
      .SampleDesc =
          {
              .Count = 1,
              .Quality = 0,
          },
      .Usage = desc->usage,
      .BindFlags = desc->bindFlags,
      .CPUAccessFlags = 0,
  };

  UINT pitch = castI32U32(desc->width) * castI32U32(desc->bytesPerPixel);

  if (desc->generateMipMaps) {
    textureDesc.MipLevels =
        (uint32_t)floorf(log2f((float)MAX(desc->width, desc->height)));
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags |=
        (D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
    textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    HR_ASSERT(gDevice->CreateTexture2D(&textureDesc, NULL, texture));
    if (desc->initialData) {
      gContext->UpdateSubresource(*texture, 0, 0, desc->initialData, pitch, 0);
    }
  } else {
    if (desc->initialData) {
      D3D11_SUBRESOURCE_DATA initialData = {
          .pSysMem = desc->initialData,
          .SysMemPitch = pitch,
      };
      HR_ASSERT(gDevice->CreateTexture2D(&textureDesc, &initialData, texture));
    } else {
      HR_ASSERT(gDevice->CreateTexture2D(&textureDesc, NULL, texture));
    }
  }

  if (textureView) {
    if (desc->viewFormat) {
      D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {
          .Format = desc->viewFormat,
          .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
          .Texture2D =
              {
                  .MostDetailedMip = 0,
                  .MipLevels = textureDesc.MipLevels,
              },
      };
      HR_ASSERT(
          gDevice->CreateShaderResourceView(*texture, &viewDesc, textureView));
    } else {
      HR_ASSERT(gDevice->CreateShaderResourceView(*texture, NULL, textureView));
    }

    if (desc->generateMipMaps) {
      gContext->GenerateMips(*textureView);
    }
  }
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

  COM_RELEASE(model->gpuIndexBuffer);
  COM_RELEASE(model->gpuVertexBuffer);
  MFREE(model->bufferBase);

  for (int i = 0; i < model->numMeshes; ++i) {
    MFREE(model->meshes[i].subMeshes);
  }
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
                       UNUSED ID3D11Buffer *drawUniformBuffer,
                       ID3D11Buffer *materialUniformBuffer) {

  for (int subMeshIndex = 0; subMeshIndex < mesh->numSubMeshes;
       ++subMeshIndex) {
    const SubMesh *subMesh = &mesh->subMeshes[subMeshIndex];
    const Material *material = &model->materials[subMesh->material];
    MaterialUniforms materialUniforms = {
        .baseColorFactor = material->baseColorFactor,
        .metallicRoughnessFactor = {material->metallicFactor,
                                    material->roughnessFactor}};

    ID3D11ShaderResourceView *baseColorTextureView = gDefaultTextureView;
    if (material->baseColorTexture >= 0) {
      baseColorTextureView = model->textureViews[material->baseColorTexture];
    }

    ID3D11SamplerState *baseColorSampler = gDefaultSampler;
    if (material->baseColorSampler >= 0) {
      baseColorSampler = model->samplers[material->baseColorSampler];
    }

    ID3D11ShaderResourceView *metallicRoughnessTextureView =
        gDefaultTextureView;
    if (material->metallicRoughnessTexture >= 0) {
      metallicRoughnessTextureView =
          model->textureViews[material->metallicRoughnessTexture];
    }

    ID3D11SamplerState *metallicRoughnessSampler = gDefaultSampler;
    if (material->metallicRoughnessSampler >= 0) {
      metallicRoughnessSampler =
          model->samplers[material->metallicRoughnessSampler];
    }

    gContext->UpdateSubresource(materialUniformBuffer, 0, NULL,
                                &materialUniforms, 0, 0);

    ID3D11ShaderResourceView *textureViews[] = {baseColorTextureView,
                                                metallicRoughnessTextureView};
    ID3D11SamplerState *samplers[] = {baseColorSampler,
                                      metallicRoughnessSampler};
    gContext->PSSetShaderResources(2, ARRAY_SIZE(textureViews), textureViews);
    gContext->PSSetSamplers(2, ARRAY_SIZE(samplers), samplers);
    gContext->DrawIndexed(castI32U32(subMesh->numIndices),
                          castSsizeU32(subMesh->indices - model->indexBase),
                          castSsizeI32(subMesh->vertices - model->vertexBase));
  }
}

static void renderSceneNode(const Model *model, const SceneNode *node,
                            ID3D11Buffer *drawUniformBuffer,
                            ID3D11Buffer *materialUniformBuffer) {
  if (node->mesh >= 0) {
    DrawUniforms drawUniforms = {};
    drawUniforms.modelMat = node->worldTransform.matrix;
    drawUniforms.invModelMat = mat4Inverse(drawUniforms.modelMat);
    gContext->UpdateSubresource(drawUniformBuffer, 0, NULL, &drawUniforms, 0,
                                0);
    renderMesh(model, &model->meshes[node->mesh], drawUniformBuffer,
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

void renderModel(const Model *model, ID3D11Buffer *drawUniformBuffer,
                 ID3D11Buffer *materialUniformBuffer) {
  UINT stride = sizeof(Vertex), offset = 0;
  gContext->IASetVertexBuffers(0, 1, &model->gpuVertexBuffer, &stride, &offset);
  gContext->IASetIndexBuffer(model->gpuIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
  gContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  for (int sceneIndex = 0; sceneIndex < model->numScenes; ++sceneIndex) {
    const Scene *scene = &model->scenes[sceneIndex];
    for (int nodeIndex = 0; nodeIndex < scene->numNodes; ++nodeIndex) {
      const SceneNode *node = &model->nodes[scene->nodes[nodeIndex]];
      renderSceneNode(model, node, drawUniformBuffer, materialUniformBuffer);
    }
  }
}

Float3 getLook(const FreeLookCamera *cam) {
  float yawRadian = degToRad(cam->yaw);
  float pitchRadian = degToRad(cam->pitch);
  float cosPitch = cosf(pitchRadian);
  return float3(-sinf(yawRadian) * cosPitch, sinf(pitchRadian),
                cosf(yawRadian) * cosPitch);
}

Float3 getRight(const FreeLookCamera *cam) {
  return float3Normalize(float3Cross(getLook(cam), DEFAULT_UP));
}

Mat4 getViewMatrix(const FreeLookCamera *cam) {
  return mat4LookAt(cam->pos, cam->pos + getLook(cam), DEFAULT_UP);
}

bool processKeyboardEvent(const SDL_Event *event, SDL_Keycode keycode,
                          bool keyDown) {
  if ((event->type == SDL_KEYDOWN || event->type == SDL_KEYUP) &&
      event->key.keysym.sym == keycode) {
    keyDown = (event->key.state == SDL_PRESSED);
  }
  return keyDown;
}

void generateHammersleySequence(int n, Float4 *values) {
  for (int k = 0; k < n; ++k) {
    float p = 0.5f;
    int kk = k;
    float u = 0.5f;
    while (kk) {
      if (kk & 1) {
        u += p;
      }
      p *= 0.5f;
      kk >>= 1;
    }
    float v = ((float)k + 0.5f) / (float)n;
    values[k].xy = {u, v};
  }

  Float2 avg = {0};
  for (int i = 0; i < n; ++i) {
    avg.xy += values[i].xy;
  }
  avg /= (float)n;
}

ID3D11Texture2D *gPositionTexture;
ID3D11ShaderResourceView *gPositionView;
ID3D11RenderTargetView *gPositionRTV;

ID3D11Texture2D *gNormalTexture;
ID3D11ShaderResourceView *gNormalView;
ID3D11RenderTargetView *gNormalRTV;

ID3D11Texture2D *gAlbedoTexture;
ID3D11ShaderResourceView *gAlbedoView;
ID3D11RenderTargetView *gAlbedoRTV;

ShaderProgram gSSAOProgram;

void createSSAOResources(int ww, int wh) {
  TextureDesc textureDesc = {
      .width = ww,
      .height = wh,
      .format = DXGI_FORMAT_R16G16B16A16_FLOAT,
      .usage = D3D11_USAGE_DEFAULT,
      .bindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
  };

  createTexture2D(&textureDesc, &gPositionTexture, &gPositionView);
  HR_ASSERT(
      gDevice->CreateRenderTargetView(gPositionTexture, NULL, &gPositionRTV));
  createTexture2D(&textureDesc, &gNormalTexture, &gNormalView);
  HR_ASSERT(gDevice->CreateRenderTargetView(gNormalTexture, NULL, &gNormalRTV));
  createTexture2D(&textureDesc, &gAlbedoTexture, &gAlbedoView);
  HR_ASSERT(gDevice->CreateRenderTargetView(gAlbedoTexture, NULL, &gAlbedoRTV));

  loadProgram("ssao", &gSSAOProgram);

  // std::uniform_real_distribution<float> randomFloats(0, 1);
  // std::default_random_engine generator;
  // std::vector<Float4> ssaoKernel;
  // for (int i = 0; i < 64; ++i) {
  //   Float4 sample = {
  //       randomFloats(generator) * 2.f - 1.f,
  //       randomFloats(generator) * 2.f - 1.f,
  //       randomFloats(generator) * 2.f - 1.f,
  //       0,
  //   };
  //   sample.xyz = float3Normalize(sample.xyz);
  //   float scale = (float)i / 64.f;
  //   auto lerp = [](float a, float b, float f) { return a + f * (b - a); };
  //   scale = lerp(0.1f, 1.f, scale * scale);
  //   sample *= scale;
  //   // sample *= randomFloats(generator);
  //   ssaoKernel.push_back(sample);
  // }
}

void destroySSAOResources() {
  destroyProgram(&gSSAOProgram);

  COM_RELEASE(gAlbedoRTV);
  COM_RELEASE(gAlbedoView);
  COM_RELEASE(gAlbedoTexture);
  COM_RELEASE(gNormalRTV);
  COM_RELEASE(gNormalView);
  COM_RELEASE(gNormalTexture);
  COM_RELEASE(gPositionRTV);
  COM_RELEASE(gPositionView);
  COM_RELEASE(gPositionTexture);
}
