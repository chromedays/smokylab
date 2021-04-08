#include "smokylab.h"
#include "util.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <cgltf.h>
#include <stb_image.h>
#include <SDL2/SDL_syswm.h>
#include <d3d11shader.h>
#pragma clang diagnostic pop

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
    gContext->UpdateSubresource(*texture, 0, 0, desc->initialData, pitch, 0);
  } else {
    D3D11_SUBRESOURCE_DATA initialData = {
        .pSysMem = desc->initialData,
        .SysMemPitch = pitch,
    };
    HR_ASSERT(gDevice->CreateTexture2D(&textureDesc, &initialData, texture));
  }

  if (textureView) {
    HR_ASSERT(gDevice->CreateShaderResourceView(*texture, NULL, textureView));

    if (desc->generateMipMaps) {
      gContext->GenerateMips(*textureView);
    }
  }
}

void createIBLTexture(const char *baseName, int *skyWidth, int *skyHeight,
                      ID3D11Texture2D **skyTex, ID3D11Texture2D **irrTex,
                      ID3D11ShaderResourceView **skyTexView,
                      ID3D11ShaderResourceView **irrTexView) {
  String basePath = {0};
  copyBasePath(&basePath);
  appendPathCStr(&basePath, "../assets/ibl");
  appendPathCStr(&basePath, baseName);

  String skyPath = {0};
  copyString(&skyPath, &basePath);
  appendCStr(&skyPath, ".hdr");
  int w, h, nc;
  void *pixels = stbi_loadf(skyPath.buf, &w, &h, &nc, STBI_rgb_alpha);
  *skyWidth = w;
  *skyHeight = h;
  ASSERT(pixels);
  TextureDesc skyTexDesc = {
      .width = w,
      .height = h,
      .bytesPerPixel = 16,
      .format = DXGI_FORMAT_R32G32B32A32_FLOAT,
      .usage = D3D11_USAGE_IMMUTABLE,
      .bindFlags = D3D11_BIND_SHADER_RESOURCE,
      .generateMipMaps = true,
      .initialData = pixels,
  };
  createTexture2D(&skyTexDesc, skyTex, skyTexView);

  String irrPath = {0};
  copyString(&irrPath, &basePath);
  appendCStr(&irrPath, ".irr.hdr");

  pixels = stbi_loadf(irrPath.buf, &w, &h, &nc, STBI_rgb_alpha);
  ASSERT(pixels);
  TextureDesc irrTexDesc = {
      .width = w,
      .height = h,
      .bytesPerPixel = 16,
      .format = DXGI_FORMAT_R32G32B32A32_FLOAT,
      .usage = D3D11_USAGE_IMMUTABLE,
      .bindFlags = D3D11_BIND_SHADER_RESOURCE,
      .initialData = pixels,
  };
  createTexture2D(&irrTexDesc, irrTex, irrTexView);

  destroyString(&irrPath);
  destroyString(&skyPath);
  destroyString(&basePath);
}

void loadGLTFModel(const char *path, Model *model) {
  String pathString = {0};
  copyStringFromCStr(&pathString, path);

  String basePath = {0};

  String filePath = {0};
  copyBasePath(&filePath);

  if (endsWithCString(&pathString, ".glb")) {
    appendPathCStr(&filePath, pathString.buf);
  } else {
    const char *baseName = pathBaseName(&pathString);
    appendPathCStr(&filePath, path);
    copyString(&basePath, &filePath);
    appendPathCStr(&filePath, baseName);
    appendCStr(&filePath, ".gltf");
  }
  LOG("Loading GLTF: %s", filePath.buf);
  destroyString(&pathString);

  cgltf_options options = {};
  cgltf_data *gltf;
  cgltf_result gltfLoadResult = cgltf_parse_file(&options, filePath.buf, &gltf);
  ASSERT(gltfLoadResult == cgltf_result_success);
  cgltf_load_buffers(&options, gltf, filePath.buf);

  model->numTextures = gltf->textures_count;
  if (model->numTextures > 0) {
    model->textures =
        MMALLOC_ARRAY(ID3D11Texture2D *, castI32U32(model->numTextures));
    model->textureViews = MMALLOC_ARRAY(ID3D11ShaderResourceView *,
                                        castI32U32(model->numTextures));

    String imageFilePath = {0};
    for (cgltf_size textureIndex = 0; textureIndex < gltf->images_count;
         ++textureIndex) {
      cgltf_image *gltfImage = &gltf->images[textureIndex];
      int w, h, numComponents;
      stbi_uc *data;
      if (gltfImage->buffer_view) {
        data = stbi_load_from_memory(
            (uint8_t *)gltfImage->buffer_view->buffer->data +
                gltfImage->buffer_view->offset,
            gltfImage->buffer_view->size, &w, &h, &numComponents,
            STBI_rgb_alpha);
      } else {
        copyString(&imageFilePath, &basePath);
        appendPathCStr(&imageFilePath, gltfImage->uri);

        data = stbi_load(imageFilePath.buf, &w, &h, &numComponents,
                         STBI_rgb_alpha);
      }
      ASSERT(data);

      TextureDesc desc = {
          .width = w,
          .height = h,
          .bytesPerPixel = 4,
          .format = DXGI_FORMAT_R8G8B8A8_UNORM,
          .usage = D3D11_USAGE_IMMUTABLE,
          .bindFlags = D3D11_BIND_SHADER_RESOURCE,
          .generateMipMaps = true,
          .initialData = data,
      };
      createTexture2D(&desc, &model->textures[textureIndex],
                      &model->textureViews[textureIndex]);

      stbi_image_free(data);
    }
    destroyString(&imageFilePath);
  }

#define GLTF_NEAREST 9728
#define GLTF_LINEAR 9729
#define GLTF_NEAREST_MIPMAP_NEAREST 9984
#define GLTF_LINEAR_MIPMAP_NEAREST 9985
#define GLTF_NEAREST_MIPMAP_LINEAR 9986
#define GLTF_LINEAR_MIPMAP_LINEAR 9987
#define GLTF_CLAMP_TO_EDGE 33071
#define GLTF_MIRRORED_REPEAT 33648
#define GLTF_REPEAT 10497
  model->numSamplers = (int)gltf->samplers_count;
  if (model->numSamplers > 0) {
    model->samplers =
        MMALLOC_ARRAY(ID3D11SamplerState *, castI32U32(model->numSamplers));

    for (cgltf_size samplerIndex = 0; samplerIndex < gltf->samplers_count;
         ++samplerIndex) {
      cgltf_sampler *gltfSampler = &gltf->samplers[samplerIndex];
      ID3D11SamplerState **sampler = &model->samplers[samplerIndex];

      D3D11_SAMPLER_DESC desc = {
          .AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
          .MaxLOD = D3D11_FLOAT32_MAX,
      };

      if (gltfSampler->mag_filter == GLTF_NEAREST) {
        if (gltfSampler->min_filter == GLTF_NEAREST) {
          desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        } else if (gltfSampler->min_filter == GLTF_LINEAR) {
          desc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        } else if (gltfSampler->min_filter == GLTF_NEAREST_MIPMAP_NEAREST) {
          desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        } else if (gltfSampler->min_filter == GLTF_LINEAR_MIPMAP_NEAREST) {
          desc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        } else if (gltfSampler->min_filter == GLTF_LINEAR_MIPMAP_LINEAR) {
          desc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        } else { // NEAREST_MIPMAP_LINEAR
          desc.Filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
        }
      } else { // LINEAR
        if (gltfSampler->min_filter == GLTF_NEAREST) {
          desc.Filter = D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
        } else if (gltfSampler->min_filter == GLTF_LINEAR) {
          desc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        } else if (gltfSampler->min_filter == GLTF_NEAREST_MIPMAP_NEAREST) {
          desc.Filter = D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
        } else if (gltfSampler->min_filter == GLTF_LINEAR_MIPMAP_NEAREST) {
          desc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        } else if (gltfSampler->min_filter == GLTF_LINEAR_MIPMAP_LINEAR) {
          desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        } else { // NEAREST_MIPMAP_LINEAR
          desc.Filter = D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
        }
      }

      switch (gltfSampler->wrap_s) {
      case GLTF_CLAMP_TO_EDGE:
        desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        break;
      case GLTF_MIRRORED_REPEAT:
        desc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
        break;
      case GLTF_REPEAT:
      default:
        desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        break;
      }
      switch (gltfSampler->wrap_t) {
      case GLTF_CLAMP_TO_EDGE:
        desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        break;
      case GLTF_MIRRORED_REPEAT:
        desc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
        break;
      case GLTF_REPEAT:
      default:
        desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        break;
      }

      HR_ASSERT(gDevice->CreateSamplerState(&desc, sampler));
    }
  }

#undef GLTF_NEAREST
#undef GLTF_LINEAR
#undef GLTF_NEAREST_MIPMAP_NEAREST
#undef GLTF_LINEAR_MIPMAP_NEAREST
#undef GLTF_NEAREST_MIPMAP_LINEAR
#undef GLTF_LINEAR_MIPMAP_LINEAR
#undef GLTF_CLAMP_TO_EDGE
#undef GLTF_MIRRORED_REPEAT
#undef GLTF_REPEAT

  model->numMaterials = gltf->materials_count;
  if (model->numMaterials > 0) {
    model->materials = MMALLOC_ARRAY(Material, model->numMaterials);
    for (cgltf_size materialIndex = 0; materialIndex < gltf->materials_count;
         ++materialIndex) {
      cgltf_material *gltfMaterial = &gltf->materials[materialIndex];
      Material *material = &model->materials[materialIndex];
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
        material->baseColorTexture = gltfMaterial->pbr_metallic_roughness
                                         .base_color_texture.texture->image -
                                     gltf->images;

        if (pbrMR->base_color_texture.texture->sampler) {
          material->baseColorSampler =
              gltfMaterial->pbr_metallic_roughness.base_color_texture.texture
                  ->sampler -
              gltf->samplers;
        }
      }

      if (pbrMR->metallic_roughness_texture.texture) {
        material->metallicRoughnessTexture =
            pbrMR->metallic_roughness_texture.texture->image - gltf->images;
        if (pbrMR->metallic_roughness_texture.texture->sampler) {
          material->metallicRoughnessSampler =
              pbrMR->metallic_roughness_texture.texture->sampler -
              gltf->samplers;
        }
      }

      material->metallicFactor = pbrMR->metallic_factor;
      material->roughnessFactor = pbrMR->roughness_factor;
    }
  }

  model->numMeshes = gltf->meshes_count;
  model->meshes = MMALLOC_ARRAY(Mesh, model->numMeshes);

  int numVertices = 0;
  int numIndices = 0;
  for (cgltf_size meshIndex = 0; meshIndex < gltf->meshes_count; ++meshIndex) {
    cgltf_mesh *gltfMesh = &gltf->meshes[meshIndex];

    Mesh *mesh = &model->meshes[meshIndex];
    mesh->numSubMeshes = gltfMesh->primitives_count;
    mesh->subMeshes = MMALLOC_ARRAY(SubMesh, mesh->numSubMeshes);

    for (cgltf_size primIndex = 0; primIndex < gltfMesh->primitives_count;
         ++primIndex) {
      cgltf_primitive *prim = &gltfMesh->primitives[primIndex];

      SubMesh *subMesh = &mesh->subMeshes[primIndex];
      subMesh->numIndices = prim->indices->count;

      VertexIndex maxIndex = 0;
      for (cgltf_size i = 0; i < prim->indices->count; ++i) {
        VertexIndex index = cgltf_accessor_read_index(prim->indices, i);
        if (maxIndex < index) {
          maxIndex = index;
        }
      }

      subMesh->numVertices = maxIndex + 1;

      subMesh->material = prim->material - gltf->materials;

      numVertices += subMesh->numVertices;
      numIndices += subMesh->numIndices;
    }
  }

  model->bufferSize =
      (numVertices * sizeof(Vertex) + numIndices * sizeof(VertexIndex));
  model->bufferBase = MMALLOC_ARRAY(uint8_t, model->bufferSize);
  model->numVertices = numVertices;
  model->vertexBase = (Vertex *)model->bufferBase;
  model->numIndices = numIndices;
  model->indexBase = (VertexIndex *)((uint8_t *)model->bufferBase +
                                     numVertices * sizeof(Vertex));
  Vertex *vertexBuffer = model->vertexBase;
  VertexIndex *indexBuffer = model->indexBase;

  for (cgltf_size meshIndex = 0; meshIndex < gltf->meshes_count; ++meshIndex) {
    cgltf_mesh *gltfMesh = &gltf->meshes[meshIndex];
    Mesh *mesh = &model->meshes[meshIndex];

    for (cgltf_size primIndex = 0; primIndex < gltfMesh->primitives_count;
         ++primIndex) {

      cgltf_primitive *prim = &gltfMesh->primitives[primIndex];
      SubMesh *subMesh = &mesh->subMeshes[primIndex];

      subMesh->indices = indexBuffer;
      indexBuffer += subMesh->numIndices;
      subMesh->vertices = vertexBuffer;
      vertexBuffer += subMesh->numVertices;

      for (cgltf_size i = 0; i < prim->indices->count; ++i) {
        subMesh->indices[i] = cgltf_accessor_read_index(prim->indices, i);
      }

      for (int i = 0; i < subMesh->numVertices; ++i) {
        subMesh->vertices[i].color = (Float4){1, 1, 1, 1};
      }
      for (cgltf_size attribIndex = 0; attribIndex < prim->attributes_count;
           ++attribIndex) {
        cgltf_attribute *attrib = &prim->attributes[attribIndex];
        ASSERT(!attrib->data->is_sparse); // Sparse is not supported yet;
        switch (attrib->type) {
        case cgltf_attribute_type_position:
          for (cgltf_size vertexIndex = 0; vertexIndex < attrib->data->count;
               ++vertexIndex) {
            cgltf_size numComponents = cgltf_num_components(attrib->data->type);
            cgltf_bool readResult = cgltf_accessor_read_float(
                attrib->data, vertexIndex,
                (float *)&subMesh->vertices[vertexIndex].position,
                numComponents);
            ASSERT(readResult);
          }
          break;
        case cgltf_attribute_type_texcoord:
          for (cgltf_size vertexIndex = 0; vertexIndex < attrib->data->count;
               ++vertexIndex) {
            cgltf_size numComponents = cgltf_num_components(attrib->data->type);
            cgltf_bool readResult = cgltf_accessor_read_float(
                attrib->data, vertexIndex,
                (float *)&subMesh->vertices[vertexIndex].texcoord,
                numComponents);
            ASSERT(readResult);
          }
          break;
        case cgltf_attribute_type_color:
          for (cgltf_size vertexIndex = 0; vertexIndex < attrib->data->count;
               ++vertexIndex) {
            cgltf_size numComponents = cgltf_num_components(attrib->data->type);
            cgltf_bool readResult = cgltf_accessor_read_float(
                attrib->data, vertexIndex,
                (float *)&subMesh->vertices[vertexIndex].color, numComponents);
            ASSERT(readResult);
          }
          break;
        case cgltf_attribute_type_normal:
          for (cgltf_size vertexIndex = 0; vertexIndex < attrib->data->count;
               ++vertexIndex) {
            cgltf_size numComponents = cgltf_num_components(attrib->data->type);
            cgltf_bool readResult = cgltf_accessor_read_float(
                attrib->data, vertexIndex,
                (float *)&subMesh->vertices[vertexIndex].normal, numComponents);
            ASSERT(readResult);
          }
          break;
        default:
          break;
        }
      }

      subMesh->material = prim->material - gltf->materials;
    }
  }

  BufferDesc bufferDesc = {
      .size = (int)(numVertices * sizeof(Vertex)),
      .initialData = model->vertexBase,
      .usage = D3D11_USAGE_IMMUTABLE,
      .bindFlags = D3D11_BIND_VERTEX_BUFFER,
  };
  createBuffer(&bufferDesc, &model->gpuVertexBuffer);
  bufferDesc = {
      .size = (int)(numIndices * sizeof(VertexIndex)),
      .initialData = model->indexBase,
      .usage = D3D11_USAGE_IMMUTABLE,
      .bindFlags = D3D11_BIND_INDEX_BUFFER,
  };
  createBuffer(&bufferDesc, &model->gpuIndexBuffer);

  model->numNodes = gltf->nodes_count;
  model->nodes = MMALLOC_ARRAY(SceneNode, model->numNodes);
  for (cgltf_size nodeIndex = 0; nodeIndex < gltf->nodes_count; ++nodeIndex) {
    cgltf_node *gltfNode = &gltf->nodes[nodeIndex];
    SceneNode *node = &model->nodes[nodeIndex];

    cgltf_node_transform_local(gltfNode,
                               (float *)node->localTransform.matrix.cols);
    cgltf_node_transform_world(gltfNode,
                               (float *)node->worldTransform.matrix.cols);

    if (gltfNode->parent) {
      node->parent = gltfNode->parent - gltf->nodes;
    } else {
      node->parent = -1;
    }

    if (gltfNode->mesh) {
      node->mesh = gltfNode->mesh - gltf->meshes;
    } else {
      node->mesh = -1;
    }

    if (gltfNode->children_count > 0) {
      node->numChildNodes = gltfNode->children_count;
      node->childNodes = MMALLOC_ARRAY(int, node->numChildNodes);
      for (cgltf_size childIndex = 0; childIndex < gltfNode->children_count;
           ++childIndex) {
        node->childNodes[childIndex] =
            gltfNode->children[childIndex] - gltf->nodes;
      }
    }
  }

  model->numScenes = gltf->scenes_count;
  model->scenes = MMALLOC_ARRAY(Scene, model->numScenes);

  for (cgltf_size sceneIndex = 0; sceneIndex < gltf->scenes_count;
       ++sceneIndex) {
    cgltf_scene *gltfScene = &gltf->scenes[sceneIndex];
    Scene *scene = &model->scenes[sceneIndex];

    if (gltfScene->nodes_count > 0) {
      scene->numNodes = gltfScene->nodes_count;
      scene->nodes = MMALLOC_ARRAY(int, scene->numNodes);

      for (cgltf_size nodeIndex = 0; nodeIndex < gltfScene->nodes_count;
           ++nodeIndex) {
        cgltf_node *gltfNode = gltfScene->nodes[nodeIndex];
        scene->nodes[nodeIndex] = gltfNode - gltf->nodes;
      }
    }
  }

  cgltf_free(gltf);

  destroyString(&filePath);
}

void destroyModel(Model *model) {
  for (int i = 0; i < model->numScenes; ++i) {
    MFREE(model->scenes[i].nodes);
  }
  MFREE(model->scenes);

  for (int i = 0; i < model->numNodes; ++i) {
    MFREE(model->nodes[i].childNodes);
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
}

static void renderMesh(const Model *model, const Mesh *mesh,
                       ID3D11Buffer *drawUniformBuffer,
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
                          subMesh->indices - model->indexBase,
                          subMesh->vertices - model->vertexBase);
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
  return (Float3){-sinf(yawRadian) * cosPitch, sinf(pitchRadian),
                  cosf(yawRadian) * cosPitch};
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
