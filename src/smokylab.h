#pragma once
#include "util.h"
#include "str.h"
#include "vmath.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <SDL2/SDL.h>
#include <d3d11_1.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>
#pragma clang diagnostic pop

C_INTERFACE_BEGIN

extern ID3D11Device *gDevice;
extern ID3D11DeviceContext *gContext;
extern IDXGISwapChain *gSwapChain;
extern ID3D11RenderTargetView *gSwapChainRTV;
extern ID3D11Texture2D *gSwapChainDepthStencilBuffer;
extern ID3D11DepthStencilView *gSwapChainDSV;

DXGI_RATIONAL queryRefreshRate(int ww, int wh, DXGI_FORMAT swapChainFormat);
HWND getWin32WindowHandle(SDL_Window *window);

typedef struct _ShaderProgram {
  ID3D11VertexShader *vert;
  ID3D11InputLayout *inputLayout;
  ID3D11PixelShader *frag;
} ShaderProgram;

void createProgram(const char *baseName, ShaderProgram *program);
void destroyProgram(ShaderProgram *program);
void useProgram(const ShaderProgram *program);

typedef struct _Vertex {
  Float4 position;
  Float4 color;
  Float4 texcoord;
  Float4 normal;
} Vertex;

typedef uint32_t VertexIndex;

typedef struct _BufferDesc {
  int size;
  const void *initialData;
  D3D11_USAGE usage;
  UINT bindFlags;
} BufferDesc;

void createBuffer(const BufferDesc *desc, ID3D11Buffer **buffer);

typedef struct _ViewUniforms {
  Mat4 viewMat;
  Mat4 projMat;
} ViewUniforms;

typedef struct _DrawUniforms {
  Mat4 modelMat;
  Mat4 invModelMat;
} DrawUniforms;

typedef struct _Material {
  int baseColorTexture;
  int baseColorSampler;
  Float4 baseColorFactor;
  int metallicRoughnessTexture;
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
} Material;

typedef struct _SubMesh {
  int numVertices;
  Vertex *vertices;
  int numIndices;
  VertexIndex *indices;

  int material;
} SubMesh;

typedef struct _Mesh {
  int numSubMeshes;
  SubMesh *subMeshes;
} Mesh;

typedef struct _Transform {
  Mat4 matrix;
} Transform;

typedef struct _SceneNode {
  int parent;

  Transform localTransform;
  Transform worldTransform;

  int mesh;

  int *childNodes;
  int numChildNodes;
} SceneNode;

typedef struct _Scene {
  int numNodes;
  int *nodes;
} Scene;

typedef struct _Model {
  int numTextures;
  uint32_t *textures;

  int numSamplers;
  uint32_t *samplers;

  int numMaterials;
  Material *materials;

  int numMeshes;
  Mesh *meshes;

  int numNodes;
  SceneNode *nodes;

  int numScenes;
  Scene *scenes;

  // Combined CPU buffer (Vertex buffer + Index buffer)
  int bufferSize;
  void *bufferBase;

  int numVertices;
  Vertex *vertexBase;
  int numIndices;
  VertexIndex *indexBase;

  ID3D11Buffer *gpuVertexBuffer;
  ID3D11Buffer *gpuIndexBuffer;
} Model;

void loadGLTFModel(const char *path, Model *model);
void destroyModel(Model *model);
void renderModel(const Model *model, ID3D11Buffer *drawUniformBuffer);

typedef struct _FreeLookCamera {
  Float3 pos;
  float yaw;
  float pitch;
} FreeLookCamera;

Float3 getLook(const FreeLookCamera *cam);
Float3 getRight(const FreeLookCamera *cam);
Mat4 getViewMatrix(const FreeLookCamera *cam);

bool processKeyboardEvent(const SDL_Event *event, SDL_Keycode keycode,
                          bool keyDown);

C_INTERFACE_END
