#pragma once
#include "vmath.h"
#include "str.h"
#include "camera.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#ifdef SMK_DIRECTX11
#include <d3d11_1.h>
#include <d3d12.h>
#endif
#pragma clang diagnostic pop

C_INTERFACE_BEGIN

FORWARD_DECL(smkRenderer);

typedef struct _smkShaderProgram {
#ifdef SMK_DIRECTX11
  ID3D11VertexShader *vertexShader;
  ID3D11InputLayout *vertexLayout;
  ID3D11PixelShader *fragmentShader;
#endif
} smkShaderProgram;

smkShaderProgram smkLoadProgramFromShaderAsset(smkRenderer *renderer,
                                               const char *assetPath);
void smkDestroyProgram(smkShaderProgram *program);
void smkUseProgram(smkRenderer *renderer, smkShaderProgram *program);

typedef struct _smkTexture {
#ifdef SMK_DIRECTX11
  D3D11_TEXTURE2D_DESC desc;
  ID3D11Texture2D *handle;
  ID3D11ShaderResourceView *view;
#endif
} smkTexture;

// smkTexture smkCreateTexture2D(smkRenderer *renderer, int width, int height,
//                               DXGI_FORMAT format, D3D11_USAGE usage,
//                               uint32_t bindFlags, bool generateMipMaps,
//                               void *initialData);
void smkDestroyTexture(smkTexture *texture);

typedef struct _smkSampler {
#ifdef SMK_DIRECTX11
  D3D11_SAMPLER_DESC desc;
  ID3D11SamplerState *handle;
#endif
} smkSampler;

typedef struct _smkBuffer {
#ifdef SMK_DIRECTX11
  D3D11_BUFFER_DESC desc;
  ID3D11Buffer *handle;
#endif
} smkBuffer;

typedef struct _smkViewUniforms {
  Mat4 viewMat;
  Mat4 projMat;
  Float4 viewPos;
} smkViewUniforms;

typedef struct _smkMaterialUniforms {
  Float4 baseColorFactor;
  Float4 metallicRoughnessFactor;
} smkMaterialUniforms;

typedef struct _smkDrawUniforms {
  Mat4 modelMat;
  Mat4 invModelMat;
} smkDrawUniforms;

typedef struct _smkCameraVolumeVertexData {
  Float4 vertices[24];
} smkCameraVolumeVertexData;

typedef struct _smkRenderer {
#ifdef SMK_DIRECTX11
#ifdef DEBUG
  ID3D12Device *dummyDeviceForFixedGPUClock;
#endif

  ID3D11Device *device;
  ID3D11DeviceContext *context;
  IDXGISwapChain *swapChain;
  ID3D11RenderTargetView *swapChainRTV;
  smkTexture swapChainDepthTexture;
  ID3D11DepthStencilView *swapChainDSV;

  smkTexture defaultTexture;
  smkSampler defaultSampler;

  ID3D11DepthStencilState *defaultDepthStencilState;
  ID3D11RasterizerState *defaultRasterizerState;

  smkBuffer viewUniformBuffer;
  smkBuffer materialUniformBuffer;
  smkBuffer drawUniformBuffer;

  smkBuffer cameraVolumeVertexBuffer;

  smkShaderProgram forwardPBRProgram;
  smkShaderProgram debugProgram;

#ifdef DEBUG
  ID3D11Query *profilerDisjointQuery;
  ID3D11Query *profilerTimestampBeginQuery;
  ID3D11Query *profilerTimestampEndQuery;
#endif
#endif
} smkRenderer;

smkRenderer smkCreateRenderer(void);
void smkDestroyRenderer(smkRenderer *renderer);

typedef struct _smkMaterial {
  int baseColorTexture;
  int baseColorSampler;
  Float4 baseColorFactor;
  int metallicRoughnessTexture; // r: ao (optional), g: roughness, b: metallic
  int metallicRoughnessSampler;
  float metallicFactor;
  float roughnessFactor;
  int normalTexture;
  int normalSampler;
  float normalScale;
  int occlusionTexture;
  int occlusionSampler;
  float occlusionStrength;
  int emissiveTexture;
  int emissiveSampler;
  Float3 emissiveFactor;
} smkMaterial;

typedef struct _smkSubMesh {
  int vertexBase;
  int numVertices;
  int indexBase;
  int numIndices;
  int material;
} smkSubMesh;

typedef struct _smkVertex {
  Float4 position;
  Float4 color;
  Float4 texCoord;
  Float4 normal;
} smkVertex;

typedef struct _smkMesh {
  int numVertices;
  int numIndices;
  int bufferSize;
  uint8_t *bufferMemory;
  smkVertex *vertices;
  uint32_t *indices;

  smkBuffer gpuBuffer;

  int numSubMeshes;
  smkSubMesh *subMeshes;
} smkMesh;

// TODO:
// https://math.stackexchange.com/questions/237369/given-this-transformation-matrix-how-do-i-decompose-it-into-translation-rotati/417813
typedef struct _smkTransform {
  Mat4 matrix;
} smkTransform;

typedef struct _smkEntity {
  String name;

  int parent;

  smkTransform localTransform;
  smkTransform worldTransform;

  int mesh;

  int numChildren;
  int *children;
} smkEntity;

typedef struct _smkScene {
  int numTextures;
  smkTexture *textures;

  int numSamplers;
  smkSampler *samplers;

  int numMaterials;
  smkMaterial *materials;

  int numMeshes;
  smkMesh *meshes;

  int numEntities;
  smkEntity *entities;
} smkScene;

smkScene smkLoadSceneFromGLTFAsset(smkRenderer *renderer,
                                   const char *assetPath);
void smkDestroyScene(smkScene *scene);
smkScene smkMergeScene(smkRenderer *renderer, const smkScene *sceneA,
                       const smkScene *sceneB);
void smkRenderScene(smkRenderer *renderer, const smkScene *scene);

typedef struct _smkRenderCommand {
  Camera *viewCamera;
  smkScene *scene;
} smkRenderCommand;

void smkSubmitRenderCommands(smkRenderer *renderer, int numRenderCommands,
                             smkRenderCommand *renderCommands);

//
// GUI
//

void smkInitGUI(smkRenderer *renderer);
void smkDestroyGUI(void);

C_INTERFACE_END