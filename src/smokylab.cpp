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
    model->textures = MMALLOC_ARRAY(uint32_t, model->numTextures);

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

      // createTexture(
      //     &(TextureDesc){
      //         .width = w,
      //         .height = h,
      //         .internalFormat = GL_RGBA8,
      //         .sourceFormat = GL_RGBA,
      //         .sourceType = GL_UNSIGNED_BYTE,
      //         .initialData = data,
      //     },
      //     &model->textures[textureIndex]);

      stbi_image_free(data);
    }
    destroyString(&imageFilePath);
  }

  // model->numSamplers = gltf->samplers_count;
  // if (model->numSamplers > 0) {
  //   model->samplers = MMALLOC_ARRAY(uint32_t, model->numSamplers);

  //   for (cgltf_size samplerIndex = 0; samplerIndex < gltf->samplers_count;
  //        ++samplerIndex) {
  //     cgltf_sampler *gltfSampler = &gltf->samplers[samplerIndex];
  //     uint32_t *sampler = &model->samplers[samplerIndex];
  //     glGenSamplers(1, sampler);
  //     int32_t magFilter = gltfSampler->mag_filter;
  //     if (magFilter == 0) {
  //       magFilter = GL_LINEAR;
  //     }
  //     int32_t minFilter = gltfSampler->min_filter;
  //     if (minFilter == 0) {
  //       minFilter = GL_NEAREST_MIPMAP_LINEAR;
  //     }
  //     glSamplerParameteri(*sampler, GL_TEXTURE_MAG_FILTER, magFilter);
  //     glSamplerParameteri(*sampler, GL_TEXTURE_MIN_FILTER, minFilter);
  //     int32_t wrapModeS = gltfSampler->wrap_s;
  //     if (wrapModeS == 0) {
  //       wrapModeS = GL_REPEAT;
  //     }
  //     int32_t wrapModeT = gltfSampler->wrap_t;
  //     if (wrapModeT == 0) {
  //       wrapModeT = GL_REPEAT;
  //     }
  //     glSamplerParameteri(*sampler, GL_TEXTURE_WRAP_S, wrapModeS);
  //     glSamplerParameteri(*sampler, GL_TEXTURE_WRAP_T, wrapModeT);
  //     glSamplerParameteri(*sampler, GL_TEXTURE_WRAP_R, GL_REPEAT);
  //   }
  // }

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
}

static void renderMesh(const Model *model, const Mesh *mesh,
                       ID3D11Buffer *drawUniformBuffer) {

  for (int subMeshIndex = 0; subMeshIndex < mesh->numSubMeshes;
       ++subMeshIndex) {
    const SubMesh *subMesh = &mesh->subMeshes[subMeshIndex];
    gContext->DrawIndexed(subMesh->numIndices,
                          subMesh->indices - model->indexBase,
                          subMesh->vertices - model->vertexBase);
  }
}

static void renderSceneNode(const Model *model, const SceneNode *node,
                            ID3D11Buffer *drawUniformBuffer) {
  if (node->mesh >= 0) {
    DrawUniforms drawUniforms = {};
    drawUniforms.modelMat = node->worldTransform.matrix;
    drawUniforms.invModelMat = mat4Inverse(drawUniforms.modelMat);
    gContext->UpdateSubresource(drawUniformBuffer, 0, NULL, &drawUniforms, 0,
                                0);
    renderMesh(model, &model->meshes[node->mesh], drawUniformBuffer);
  }

  if (node->numChildNodes > 0) {
    for (int i = 0; i < node->numChildNodes; ++i) {
      const SceneNode *childNode = &model->nodes[node->childNodes[i]];
      renderSceneNode(model, childNode, drawUniformBuffer);
    }
  }
}

void renderModel(const Model *model, ID3D11Buffer *drawUniformBuffer) {
  UINT stride = sizeof(Vertex), offset = 0;
  gContext->IASetVertexBuffers(0, 1, &model->gpuVertexBuffer, &stride, &offset);
  gContext->IASetIndexBuffer(model->gpuIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
  gContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  for (int sceneIndex = 0; sceneIndex < model->numScenes; ++sceneIndex) {
    const Scene *scene = &model->scenes[sceneIndex];
    for (int nodeIndex = 0; nodeIndex < scene->numNodes; ++nodeIndex) {
      const SceneNode *node = &model->nodes[scene->nodes[nodeIndex]];
      renderSceneNode(model, node, drawUniformBuffer);
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
