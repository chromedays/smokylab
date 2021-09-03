#include "asset.h"
#include "util.h"
#include "render.h"
#include "resource.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <cgltf.h>
#include <stb_image.h>
#include <Windows.h>
#pragma clang diagnostic pop

static String gAssetRootPath;
static String gShaderRootPath;

void initAssetLoaderFromConfigFile(void) {
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

void initAssetLoader(const char *assetRootPath, const char *shaderRootPath) {
  copyStringFromCStr(&gAssetRootPath, assetRootPath);
  copyStringFromCStr(&gShaderRootPath, shaderRootPath);
}
void destroyAssetLoader(void) {
  destroyString(&gShaderRootPath);
  destroyString(&gAssetRootPath);
}

void copyAssetRootPath(String *path) { copyString(path, &gAssetRootPath); }
void copyShaderRootPath(String *path) { copyString(path, &gShaderRootPath); }

static void readFile(const String *filePath, void **data, int *size) {
  HANDLE f = CreateFileA(filePath->buf, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL, 0);
  ASSERT(f != INVALID_HANDLE_VALUE);
  DWORD fileSize = GetFileSize(f, NULL);
  *data = MMALLOC_ARRAY_UNINITIALIZED(uint8_t, castU32I32(fileSize));
  DWORD bytesRead;
  ReadFile(f, *data, fileSize, &bytesRead, NULL);
  ASSERT(fileSize == bytesRead);
  *size = (int)bytesRead;
  CloseHandle(f);
}

void loadProgram(const char *baseName, ShaderProgram *program) {
  String basePath = {};
  copyShaderRootPath(&basePath);

  String vertPath = {};
  copyString(&vertPath, &basePath);
  appendPathCStr(&vertPath, baseName);
  appendCStr(&vertPath, ".vs.cso");

  void *vertSrc;
  int vertSrcSize;
  LOG("Opening vertex shader at %s", vertPath.buf);
  readFile(&vertPath, &vertSrc, &vertSrcSize);

  String fragPath = {};
  copyString(&fragPath, &basePath);
  appendPathCStr(&fragPath, baseName);
  appendCStr(&fragPath, ".fs.cso");

  void *fragSrc;
  int fragSrcSize;
  LOG("Opening fragment shader at %s", fragPath.buf);
  readFile(&fragPath, &fragSrc, &fragSrcSize);

  createProgram(program, vertSrcSize, vertSrc, fragSrcSize, fragSrc);

  MFREE(fragSrc);
  destroyString(&fragPath);

  MFREE(vertSrc);
  destroyString(&vertPath);

  destroyString(&basePath);
}

void loadGLTFModel(const char *path, Model *model) {
  String pathString = {};
  copyStringFromCStr(&pathString, path);

  String basePath = {};

  String filePath = {};
  // copyBasePath(&filePath);
  copyAssetRootPath(&filePath);

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

  copyStringFromCStr(&model->name, path);

  model->numTextures = (int)gltf->textures_count;
  if (model->numTextures > 0) {
    model->textures = MMALLOC_ARRAY(GPUTexture2D *, model->numTextures);
    model->textureViews = MMALLOC_ARRAY(GPUTextureView *, model->numTextures);

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

      createTexture2D(
          &(TextureDesc){
              .width = w,
              .height = h,
              .bytesPerPixel = 4,
              .format = GPUResourceFormat_R8G8B8A8_UNORM,
              .usage = GPUResourceUsage_IMMUTABLE,
              .bindFlags =
                  (GPUResourceBindFlag){GPUResourceBindBits_SHADER_RESOURCE},
              .generateMipMaps = true,
              .initialData = data,
          },
          &model->textures[textureIndex], &model->textureViews[textureIndex]);

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
  model->numSamplers = (int)gltf->samplers_count;
  if (model->numSamplers > 0) {
    model->samplers = MMALLOC_ARRAY(GPUSampler *, model->numSamplers);

    for (cgltf_size samplerIndex = 0; samplerIndex < gltf->samplers_count;
         ++samplerIndex) {
      cgltf_sampler *gltfSampler = &gltf->samplers[samplerIndex];
      GPUSampler **sampler = &model->samplers[samplerIndex];

      SamplerDesc desc = {};

      if (gltfSampler->mag_filter == GLTF_NEAREST) {
        if (gltfSampler->min_filter == GLTF_NEAREST) {
          desc.filter = GPUFilter_MIN_MAG_MIP_POINT;
        } else if (gltfSampler->min_filter == GLTF_LINEAR) {
          desc.filter = GPUFilter_MIN_LINEAR_MAG_MIP_POINT;
        } else if (gltfSampler->min_filter == GLTF_NEAREST_MIPMAP_NEAREST) {
          desc.filter = GPUFilter_MIN_MAG_MIP_POINT;
        } else if (gltfSampler->min_filter == GLTF_LINEAR_MIPMAP_NEAREST) {
          desc.filter = GPUFilter_MIN_LINEAR_MAG_MIP_POINT;
        } else if (gltfSampler->min_filter == GLTF_LINEAR_MIPMAP_LINEAR) {
          desc.filter = GPUFilter_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        } else { // NEAREST_MIPMAP_LINEAR
          desc.filter = GPUFilter_MIN_MAG_POINT_MIP_LINEAR;
        }
      } else { // LINEAR
        if (gltfSampler->min_filter == GLTF_NEAREST) {
          desc.filter = GPUFilter_MIN_POINT_MAG_LINEAR_MIP_POINT;
        } else if (gltfSampler->min_filter == GLTF_LINEAR) {
          desc.filter = GPUFilter_MIN_MAG_LINEAR_MIP_POINT;
        } else if (gltfSampler->min_filter == GLTF_NEAREST_MIPMAP_NEAREST) {
          desc.filter = GPUFilter_MIN_POINT_MAG_LINEAR_MIP_POINT;
        } else if (gltfSampler->min_filter == GLTF_LINEAR_MIPMAP_NEAREST) {
          desc.filter = GPUFilter_MIN_MAG_LINEAR_MIP_POINT;
        } else if (gltfSampler->min_filter == GLTF_LINEAR_MIPMAP_LINEAR) {
          desc.filter = GPUFilter_MIN_MAG_MIP_LINEAR;
        } else { // NEAREST_MIPMAP_LINEAR
          desc.filter = GPUFilter_MIN_POINT_MAG_MIP_LINEAR;
        }
      }

      switch (gltfSampler->wrap_s) {
      case GLTF_CLAMP_TO_EDGE:
        desc.addressMode.u = GPUTextureAddressMode_CLAMP;
        break;
      case GLTF_MIRRORED_REPEAT:
        desc.addressMode.u = GPUTextureAddressMode_MIRROR;
        break;
      case GLTF_REPEAT:
      default:
        desc.addressMode.u = GPUTextureAddressMode_WRAP;
        break;
      }
      switch (gltfSampler->wrap_t) {
      case GLTF_CLAMP_TO_EDGE:
        desc.addressMode.v = GPUTextureAddressMode_CLAMP;
        break;
      case GLTF_MIRRORED_REPEAT:
        desc.addressMode.v = GPUTextureAddressMode_MIRROR;
        break;
      case GLTF_REPEAT:
      default:
        desc.addressMode.v = GPUTextureAddressMode_WRAP;
        break;
      }

      createSampler(&desc, sampler);
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

  model->numMaterials = castUsizeI32(gltf->materials_count);
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

  model->numMeshes = castUsizeI32(gltf->meshes_count);
  model->meshes = MMALLOC_ARRAY(Mesh *, model->numMeshes);
  for (int i = 0; i < model->numMeshes; ++i) {
    model->meshes[i] = allocateStaticMesh();
  }

  int numVertices = 0;
  int numIndices = 0;
  for (cgltf_size meshIndex = 0; meshIndex < gltf->meshes_count; ++meshIndex) {
    cgltf_mesh *gltfMesh = &gltf->meshes[meshIndex];

    Mesh *mesh = model->meshes[meshIndex];
    mesh->numSubMeshes = castUsizeI32(gltfMesh->primitives_count);
    mesh->subMeshes = MMALLOC_ARRAY(SubMesh, mesh->numSubMeshes);

    for (cgltf_size primIndex = 0; primIndex < gltfMesh->primitives_count;
         ++primIndex) {
      cgltf_primitive *prim = &gltfMesh->primitives[primIndex];

      SubMesh *subMesh = &mesh->subMeshes[primIndex];
      subMesh->numIndices = castUsizeI32(prim->indices->count);

      VertexIndex maxIndex = 0;
      for (cgltf_size i = 0; i < prim->indices->count; ++i) {
        VertexIndex index =
            castUsizeU32(cgltf_accessor_read_index(prim->indices, i));
        if (maxIndex < index) {
          maxIndex = index;
        }
      }

      subMesh->numVertices = castU32I32(maxIndex + 1);

      subMesh->material = castSsizeI32(prim->material - gltf->materials);

      numVertices += subMesh->numVertices;
      numIndices += subMesh->numIndices;
    }
  }

  model->bufferSize =
      castUsizeI32(castI32Usize(numVertices) * sizeof(Vertex) +
                   castI32Usize(numIndices) * sizeof(VertexIndex));
  model->bufferBase = MMALLOC_ARRAY(uint8_t, model->bufferSize);
  model->numVertices = numVertices;
  model->vertexBase = (Vertex *)model->bufferBase;
  model->numIndices = numIndices;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-align"
  model->indexBase =
      (VertexIndex *)((uint8_t *)model->bufferBase +
                      castI32Usize(numVertices) * sizeof(Vertex));
#pragma clang diagnostic pop
  Vertex *vertexBuffer = model->vertexBase;
  VertexIndex *indexBuffer = model->indexBase;

  for (cgltf_size meshIndex = 0; meshIndex < gltf->meshes_count; ++meshIndex) {
    cgltf_mesh *gltfMesh = &gltf->meshes[meshIndex];
    Mesh *mesh = model->meshes[meshIndex];

    for (cgltf_size primIndex = 0; primIndex < gltfMesh->primitives_count;
         ++primIndex) {

      cgltf_primitive *prim = &gltfMesh->primitives[primIndex];
      SubMesh *subMesh = &mesh->subMeshes[primIndex];

      subMesh->indices = indexBuffer;
      indexBuffer += subMesh->numIndices;
      subMesh->vertices = vertexBuffer;
      vertexBuffer += subMesh->numVertices;

      for (cgltf_size i = 0; i < prim->indices->count; ++i) {
        subMesh->indices[i] =
            castUsizeU32(cgltf_accessor_read_index(prim->indices, i));
      }

      for (int i = 0; i < subMesh->numVertices; ++i) {
        subMesh->vertices[i].color = float4(1, 1, 1, 1);
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

      subMesh->material = castSsizeI32(prim->material - gltf->materials);
    }
  }

  createBuffer(
      &(BufferDesc){
          .size = numVertices * castUsizeI32(sizeof(Vertex)),
          .initialData = model->vertexBase,
          .usage = GPUResourceUsage_IMMUTABLE,
          .bindFlags = GPUResourceBindBits_VERTEX_BUFFER,
      },
      &model->gpuVertexBuffer);
  createBuffer(
      &(BufferDesc){
          .size = numIndices * castUsizeI32(sizeof(VertexIndex)),
          .initialData = model->indexBase,
          .usage = GPUResourceUsage_IMMUTABLE,
          .bindFlags = GPUResourceBindBits_INDEX_BUFFER,
      },
      &model->gpuIndexBuffer);

  model->numNodes = castUsizeI32(gltf->nodes_count);
  model->nodes = MMALLOC_ARRAY(SceneNode, model->numNodes);
  for (cgltf_size nodeIndex = 0; nodeIndex < gltf->nodes_count; ++nodeIndex) {
    cgltf_node *gltfNode = &gltf->nodes[nodeIndex];
    SceneNode *node = &model->nodes[nodeIndex];
    if (gltfNode->name) {
      copyStringFromCStr(&node->name, gltfNode->name);
    } else {
      FORMAT_STRING(&node->name, "Node %zu", nodeIndex);
    }

    cgltf_node_transform_local(gltfNode,
                               (float *)node->localTransform.matrix.cols);
    cgltf_node_transform_world(gltfNode,
                               (float *)node->worldTransform.matrix.cols);

    if (gltfNode->parent) {
      node->parent = castSsizeI32(gltfNode->parent - gltf->nodes);
    } else {
      node->parent = -1;
    }

    if (gltfNode->mesh) {
      node->mesh = castSsizeI32(gltfNode->mesh - gltf->meshes);
    } else {
      node->mesh = -1;
    }

    if (gltfNode->children_count > 0) {
      node->numChildNodes = castUsizeI32(gltfNode->children_count);
      node->childNodes = MMALLOC_ARRAY(int, node->numChildNodes);
      for (cgltf_size childIndex = 0; childIndex < gltfNode->children_count;
           ++childIndex) {
        node->childNodes[childIndex] =
            castSsizeI32(gltfNode->children[childIndex] - gltf->nodes);
      }
    }
  }

  model->numScenes = castUsizeI32(gltf->scenes_count);
  model->scenes = MMALLOC_ARRAY(Scene, model->numScenes);

  for (cgltf_size sceneIndex = 0; sceneIndex < gltf->scenes_count;
       ++sceneIndex) {
    cgltf_scene *gltfScene = &gltf->scenes[sceneIndex];
    Scene *scene = &model->scenes[sceneIndex];
    if (gltfScene->name) {
      copyStringFromCStr(&scene->name, gltfScene->name);
    } else {
      FORMAT_STRING(&scene->name, "Scene %zu", sceneIndex);
    }

    if (gltfScene->nodes_count > 0) {
      scene->numNodes = castUsizeI32(gltfScene->nodes_count);
      scene->nodes = MMALLOC_ARRAY(int, scene->numNodes);

      for (cgltf_size nodeIndex = 0; nodeIndex < gltfScene->nodes_count;
           ++nodeIndex) {
        cgltf_node *gltfNode = gltfScene->nodes[nodeIndex];
        scene->nodes[nodeIndex] = castSsizeI32(gltfNode - gltf->nodes);
      }
    }
  }

  cgltf_free(gltf);

  destroyString(&filePath);
}

void loadIBLTexture(const char *baseName, int *skyWidth, int *skyHeight,
                    GPUTexture2D **skyTex, GPUTexture2D **irrTex,
                    GPUTextureView **skyTexView, GPUTextureView **irrTexView) {
  String basePath = {};
  copyAssetRootPath(&basePath);
  appendPathCStr(&basePath, baseName);

  String skyPath = {};
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
      .format = GPUResourceFormat_R32G32B32A32_FLOAT,
      .usage = GPUResourceUsage_IMMUTABLE,
      .bindFlags = GPUResourceBindBits_SHADER_RESOURCE,
      .generateMipMaps = true,
      .initialData = pixels,
  };
  createTexture2D(&skyTexDesc, skyTex, skyTexView);

  String irrPath = {};
  copyString(&irrPath, &basePath);
  appendCStr(&irrPath, ".irr.hdr");

  pixels = stbi_loadf(irrPath.buf, &w, &h, &nc, STBI_rgb_alpha);
  ASSERT(pixels);
  TextureDesc irrTexDesc = {
      .width = w,
      .height = h,
      .bytesPerPixel = 16,
      .format = GPUResourceFormat_R32G32B32A32_FLOAT,
      .usage = GPUResourceUsage_IMMUTABLE,
      .bindFlags = GPUResourceBindBits_SHADER_RESOURCE,
      .initialData = pixels,
  };
  createTexture2D(&irrTexDesc, irrTex, irrTexView);

  destroyString(&irrPath);
  destroyString(&skyPath);
  destroyString(&basePath);
}