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

typedef enum _GPUResourceUsage {
  GPUResourceUsage_DEFAULT = 0,
  GPUResourceUsage_IMMUTABLE = 1,
  GPUResourceUsage_DYNAMIC = 2,
  GPUResourceUsage_STAGING = 3
} GPUResourceUsage;

typedef enum _GPUResourceFormat {
  GPUResourceFormat_UNKNOWN = 0,
  GPUResourceFormat_R32G32B32A32_TYPELESS = 1,
  GPUResourceFormat_R32G32B32A32_FLOAT = 2,
  GPUResourceFormat_R32G32B32A32_UINT = 3,
  GPUResourceFormat_R32G32B32A32_SINT = 4,
  GPUResourceFormat_R32G32B32_TYPELESS = 5,
  GPUResourceFormat_R32G32B32_FLOAT = 6,
  GPUResourceFormat_R32G32B32_UINT = 7,
  GPUResourceFormat_R32G32B32_SINT = 8,
  GPUResourceFormat_R16G16B16A16_TYPELESS = 9,
  GPUResourceFormat_R16G16B16A16_FLOAT = 10,
  GPUResourceFormat_R16G16B16A16_UNORM = 11,
  GPUResourceFormat_R16G16B16A16_UINT = 12,
  GPUResourceFormat_R16G16B16A16_SNORM = 13,
  GPUResourceFormat_R16G16B16A16_SINT = 14,
  GPUResourceFormat_R32G32_TYPELESS = 15,
  GPUResourceFormat_R32G32_FLOAT = 16,
  GPUResourceFormat_R32G32_UINT = 17,
  GPUResourceFormat_R32G32_SINT = 18,
  GPUResourceFormat_R32G8X24_TYPELESS = 19,
  GPUResourceFormat_D32_FLOAT_S8X24_UINT = 20,
  GPUResourceFormat_R32_FLOAT_X8X24_TYPELESS = 21,
  GPUResourceFormat_X32_TYPELESS_G8X24_UINT = 22,
  GPUResourceFormat_R10G10B10A2_TYPELESS = 23,
  GPUResourceFormat_R10G10B10A2_UNORM = 24,
  GPUResourceFormat_R10G10B10A2_UINT = 25,
  GPUResourceFormat_R11G11B10_FLOAT = 26,
  GPUResourceFormat_R8G8B8A8_TYPELESS = 27,
  GPUResourceFormat_R8G8B8A8_UNORM = 28,
  GPUResourceFormat_R8G8B8A8_UNORM_SRGB = 29,
  GPUResourceFormat_R8G8B8A8_UINT = 30,
  GPUResourceFormat_R8G8B8A8_SNORM = 31,
  GPUResourceFormat_R8G8B8A8_SINT = 32,
  GPUResourceFormat_R16G16_TYPELESS = 33,
  GPUResourceFormat_R16G16_FLOAT = 34,
  GPUResourceFormat_R16G16_UNORM = 35,
  GPUResourceFormat_R16G16_UINT = 36,
  GPUResourceFormat_R16G16_SNORM = 37,
  GPUResourceFormat_R16G16_SINT = 38,
  GPUResourceFormat_R32_TYPELESS = 39,
  GPUResourceFormat_D32_FLOAT = 40,
  GPUResourceFormat_R32_FLOAT = 41,
  GPUResourceFormat_R32_UINT = 42,
  GPUResourceFormat_R32_SINT = 43,
  GPUResourceFormat_R24G8_TYPELESS = 44,
  GPUResourceFormat_D24_UNORM_S8_UINT = 45,
  GPUResourceFormat_R24_UNORM_X8_TYPELESS = 46,
  GPUResourceFormat_X24_TYPELESS_G8_UINT = 47,
  GPUResourceFormat_R8G8_TYPELESS = 48,
  GPUResourceFormat_R8G8_UNORM = 49,
  GPUResourceFormat_R8G8_UINT = 50,
  GPUResourceFormat_R8G8_SNORM = 51,
  GPUResourceFormat_R8G8_SINT = 52,
  GPUResourceFormat_R16_TYPELESS = 53,
  GPUResourceFormat_R16_FLOAT = 54,
  GPUResourceFormat_D16_UNORM = 55,
  GPUResourceFormat_R16_UNORM = 56,
  GPUResourceFormat_R16_UINT = 57,
  GPUResourceFormat_R16_SNORM = 58,
  GPUResourceFormat_R16_SINT = 59,
  GPUResourceFormat_R8_TYPELESS = 60,
  GPUResourceFormat_R8_UNORM = 61,
  GPUResourceFormat_R8_UINT = 62,
  GPUResourceFormat_R8_SNORM = 63,
  GPUResourceFormat_R8_SINT = 64,
  GPUResourceFormat_A8_UNORM = 65,
  GPUResourceFormat_R1_UNORM = 66,
  GPUResourceFormat_R9G9B9E5_SHAREDEXP = 67,
  GPUResourceFormat_R8G8_B8G8_UNORM = 68,
  GPUResourceFormat_G8R8_G8B8_UNORM = 69,
  GPUResourceFormat_BC1_TYPELESS = 70,
  GPUResourceFormat_BC1_UNORM = 71,
  GPUResourceFormat_BC1_UNORM_SRGB = 72,
  GPUResourceFormat_BC2_TYPELESS = 73,
  GPUResourceFormat_BC2_UNORM = 74,
  GPUResourceFormat_BC2_UNORM_SRGB = 75,
  GPUResourceFormat_BC3_TYPELESS = 76,
  GPUResourceFormat_BC3_UNORM = 77,
  GPUResourceFormat_BC3_UNORM_SRGB = 78,
  GPUResourceFormat_BC4_TYPELESS = 79,
  GPUResourceFormat_BC4_UNORM = 80,
  GPUResourceFormat_BC4_SNORM = 81,
  GPUResourceFormat_BC5_TYPELESS = 82,
  GPUResourceFormat_BC5_UNORM = 83,
  GPUResourceFormat_BC5_SNORM = 84,
  GPUResourceFormat_B5G6R5_UNORM = 85,
  GPUResourceFormat_B5G5R5A1_UNORM = 86,
  GPUResourceFormat_B8G8R8A8_UNORM = 87,
  GPUResourceFormat_B8G8R8X8_UNORM = 88,
  GPUResourceFormat_R10G10B10_XR_BIAS_A2_UNORM = 89,
  GPUResourceFormat_B8G8R8A8_TYPELESS = 90,
  GPUResourceFormat_B8G8R8A8_UNORM_SRGB = 91,
  GPUResourceFormat_B8G8R8X8_TYPELESS = 92,
  GPUResourceFormat_B8G8R8X8_UNORM_SRGB = 93,
  GPUResourceFormat_BC6H_TYPELESS = 94,
  GPUResourceFormat_BC6H_UF16 = 95,
  GPUResourceFormat_BC6H_SF16 = 96,
  GPUResourceFormat_BC7_TYPELESS = 97,
  GPUResourceFormat_BC7_UNORM = 98,
  GPUResourceFormat_BC7_UNORM_SRGB = 99,
  GPUResourceFormat_AYUV = 100,
  GPUResourceFormat_Y410 = 101,
  GPUResourceFormat_Y416 = 102,
  GPUResourceFormat_NV12 = 103,
  GPUResourceFormat_P010 = 104,
  GPUResourceFormat_P016 = 105,
  GPUResourceFormat_420_OPAQUE = 106,
  GPUResourceFormat_YUY2 = 107,
  GPUResourceFormat_Y210 = 108,
  GPUResourceFormat_Y216 = 109,
  GPUResourceFormat_NV11 = 110,
  GPUResourceFormat_AI44 = 111,
  GPUResourceFormat_IA44 = 112,
  GPUResourceFormat_P8 = 113,
  GPUResourceFormat_A8P8 = 114,
  GPUResourceFormat_B4G4R4A4_UNORM = 115,
  GPUResourceFormat_P208 = 130,
  GPUResourceFormat_V208 = 131,
  GPUResourceFormat_V408 = 132,
  GPUResourceFormat_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE = 189,
  GPUResourceFormat_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE = 190,
  GPUResourceFormat_FORCE_UINT = 0xffffffff
} GPUResourceFormat;

typedef enum _GPUResourceBindBits {
  GPUResourceBindBits_VERTEX_BUFFER = 0x1L,
  GPUResourceBindBits_INDEX_BUFFER = 0x2L,
  GPUResourceBindBits_CONSTANT_BUFFER = 0x4L,
  GPUResourceBindBits_SHADER_RESOURCE = 0x8L,
  GPUResourceBindBits_STREAM_OUTPUT = 0x10L,
  GPUResourceBindBits_RENDER_TARGET = 0x20L,
  GPUResourceBindBits_DEPTH_STENCIL = 0x40L,
  GPUResourceBindBits_UNORDERED_ACCESS = 0x80L,
  GPUResourceBindBits_DECODER = 0x200L,
  GPUResourceBindBits_VIDEO_ENCODER = 0x400L
} GPUResourceBindBits;
typedef uint32_t GPUResourceBindFlag;

FORWARD_DECL(GPUDevice);
FORWARD_DECL(GPUDeviceContext);
FORWARD_DECL(GPUTexture2D);
FORWARD_DECL(GPUVertexShader);
FORWARD_DECL(GPUFragmentShader);
FORWARD_DECL(GPUVertexLayout);
FORWARD_DECL(GPUTextureView);
FORWARD_DECL(GPUBuffer);
FORWARD_DECL(GPUSampler);

typedef struct _Renderer {
  GPUDevice *device;
  GPUDeviceContext *deviceContext;
} Renderer;

extern Renderer gRenderer;

C_INTERFACE_BEGIN

void initRenderer(void);
void destroyRenderer(void);

// DXGI_RATIONAL queryRefreshRate(int ww, int wh, DXGI_FORMAT swapChainFormat);
HWND getWin32WindowHandle(SDL_Window *window);

void setViewport(float x, float y, float w, float h);
void setDefaultRenderStates(Float4 clearColor);
void swapBuffers(void);

typedef struct _ShaderProgram {
  GPUVertexShader *vert;
  GPUVertexLayout *inputLayout;
  GPUFragmentShader *frag;
} ShaderProgram;

void createProgram(ShaderProgram *program, int vertSrcSize, void *vertSrc,
                   int fragSrcSize, void *fragSrc);
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
  GPUResourceUsage usage;
  GPUResourceBindFlag bindFlags;
} BufferDesc;

void createBuffer(const BufferDesc *desc, ID3D11Buffer **buffer);
void updateBufferData(ID3D11Buffer *buffer, void *data);
void bindBuffers(int numBuffers, ID3D11Buffer **buffers);

typedef struct _TextureDesc {
  int width;
  int height;
  int bytesPerPixel; // Source bytes per pixel
  GPUResourceFormat format;
  GPUResourceFormat viewFormat;
  GPUResourceUsage usage;
  GPUResourceBindFlag bindFlags;
  bool generateMipMaps;
  void *initialData;
} TextureDesc;
void createTexture2D(const TextureDesc *desc, GPUTexture2D **texture,
                     ID3D11ShaderResourceView **textureView);

// TODO: Create custom sampler desc struct
void createSampler(const D3D11_SAMPLER_DESC *desc,
                   ID3D11SamplerState **sampler);

#define NUM_SAMPLES 40
typedef struct _ViewUniforms {
  Mat4 viewMat;
  Mat4 projMat;
  Float4 viewPos;
  Int4 skySize;
  Float4 randomPoints[NUM_SAMPLES];
  Float4 exposureNearFar;
  Float4 dirLightDirIntensity;
  Float4 overrideOpacity;
  Float4 ssaoFactors;
} ViewUniforms;

typedef struct _DrawUniforms {
  Mat4 modelMat;
  Mat4 invModelMat;
} DrawUniforms;

typedef struct _MaterialUniforms {
  Float4 baseColorFactor;
  Float4 metallicRoughnessFactor;
} MaterialUniforms;

typedef struct _Material {
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
  String name;

  int parent;

  Transform localTransform;
  Transform worldTransform;

  int mesh;

  int *childNodes;
  int numChildNodes;
} SceneNode;

typedef struct _Scene {
  String name;
  int numNodes;
  int *nodes;
} Scene;

typedef struct _Model {
  String name;

  int numTextures;
  GPUTexture2D **textures;
  ID3D11ShaderResourceView **textureViews;

  int numSamplers;
  ID3D11SamplerState **samplers;

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

void destroyModel(Model *model);
void renderModel(const Model *model, ID3D11Buffer *drawUniformBuffer,
                 ID3D11Buffer *materialUniformBuffer);

bool processKeyboardEvent(const SDL_Event *event, SDL_Keycode keycode,
                          bool keyDown);

#if 0
void generateHammersleySequence(int n, Float4 *values);

extern ID3D11Texture2D *gPositionTexture;
extern ID3D11ShaderResourceView *gPositionView;
extern ID3D11RenderTargetView *gPositionRTV;

extern ID3D11Texture2D *gNormalTexture;
extern ID3D11ShaderResourceView *gNormalView;
extern ID3D11RenderTargetView *gNormalRTV;

extern ID3D11Texture2D *gAlbedoTexture;
extern ID3D11ShaderResourceView *gAlbedoView;
extern ID3D11RenderTargetView *gAlbedoRTV;

extern ID3D11Buffer *gSSAOKernelBuffer;

extern ShaderProgram gSSAOProgram;

void createSSAOResources(int ww, int wh);
void destroySSAOResources();
#endif

C_INTERFACE_END
