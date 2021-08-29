#include "resource.h"
#include "render.h"

typedef struct _GPUBufferSlice {
  GPUBuffer *buffer;
  int offset;
  int size;
} GPUBufferSlice;

struct SubMesh {
  GPUBufferSlice vertexBuffer;
  GPUBufferSlice indexBuffer;
};

typedef struct _SampledTexture {
  GPUTexture2D *texture;
  GPUTextureView *textureView;
  GPUSampler *sampler;
} SampledTexture;

// typedef struct _TextureView {
//   ID3D11Texture2D *texture;
//   ID3D11ShaderResourceView *view;
// } TextureView;

typedef struct _TextureHandle {
  int index;
} TextureHandle;

#define RESERVED_NUM_SUBMESHES 100
#define RESERVED_NUM_MESHES 100
#define RESERVED_NUM_VERTICES 10000
#define RESERVED_NUM_INDICES 10000
#define RESERVED_NUM_MATERIALS 100
#define RESERVED_NUM_TEXTURES 100
#define RESERVED_NUM_TEXTUREVIEWS 100
#define RESERVED_NUM_SAMPLERS 100

// pushStruct()..

typedef struct _ResourceTable {
  int memSize;
  void *memBase;
  SubMesh *staticSubMeshes;
  int numStaticSubMeshes;
  Mesh *staticMeshes;
  int numStaticMeshes;
  void *cpuStaticBufferBase;
  int cpuStaticBufferSize;
  Vertex *cpuStaticVertexBuffer;
  int numVertices;
  VertexIndex *cpuStaticIndexBuffer;
  int numIndices;
  bool isGPUStaticBufferCacheDirty;
  GPUBuffer *gpuStaticBufferCache;

#if 0
  Material *materials;

  GPUTexture2D **textures;
  GPUTextureView ***textureViews; // tied with resource(buffer or texture) use
                                  // index as texture type?
  GPUSampler **samplers;          // hash samplers (they're not resources?)
#endif
} ResourceManager;

ResourceManager gResourceManager;

static void refreshStaticBufferCache(void) {
  if (gResourceManager.isGPUStaticBufferCacheDirty) {
    if (gResourceManager.gpuStaticBufferCache) {
      destroyBuffer(gResourceManager.gpuStaticBufferCache);
      gResourceManager.gpuStaticBufferCache = NULL;
    }

    createBuffer(
        &(BufferDesc){
            .size = gResourceManager.cpuStaticBufferSize,
            .initialData = gResourceManager.cpuStaticBufferBase,
            .usage = GPUResourceUsage_IMMUTABLE,
            .bindFlags = GPUResourceBindBits_VERTEX_BUFFER |
                         GPUResourceBindBits_INDEX_BUFFER,
        },
        &gResourceManager.gpuStaticBufferCache);
    gResourceManager.isGPUStaticBufferCacheDirty = false;
  }
}

void initResourceManager(void) {
  int cpuStaticBufferSize = sizeof(Vertex) * RESERVED_NUM_VERTICES +
                            sizeof(VertexIndex) * RESERVED_NUM_INDICES;

  gResourceManager.memSize = sizeof(SubMesh) * RESERVED_NUM_SUBMESHES +
                             sizeof(Mesh) * RESERVED_NUM_MESHES +
                             cpuStaticBufferSize;
  gResourceManager.memBase = MMALLOC_ARRAY(uint8_t, gResourceManager.memSize);
  uint8_t *p = (uint8_t *)gResourceManager.memBase;
  gResourceManager.staticSubMeshes = (SubMesh *)p;
  p += sizeof(SubMesh) * RESERVED_NUM_SUBMESHES;
  gResourceManager.staticMeshes = (Mesh *)p;
  p += sizeof(Mesh) * RESERVED_NUM_MESHES;

  gResourceManager.cpuStaticBufferBase = p;
  gResourceManager.cpuStaticBufferSize = cpuStaticBufferSize;
  gResourceManager.cpuStaticVertexBuffer =
      (Vertex *)gResourceManager.cpuStaticBufferBase;
  gResourceManager.cpuStaticIndexBuffer =
      (VertexIndex *)((Vertex *)gResourceManager.cpuStaticBufferBase +
                      RESERVED_NUM_VERTICES);

  p += cpuStaticBufferSize;
}

void destroyResourceManager(void) {
  if (gResourceManager.gpuStaticBufferCache) {
    destroyBuffer(gResourceManager.gpuStaticBufferCache);
  }
  MFREE(gResourceManager.memBase);
}

Vertex *allocateCPUVertexBuffer(void);
VertexIndex *allocateCPUIndexBuffer(void);
SubMesh *allocateSubMesh(void);

Mesh *allocateStaticMesh(void) {
  ASSERT(gResourceManager.numStaticMeshes < RESERVED_NUM_MESHES);
  return &gResourceManager.staticMeshes[gResourceManager.numStaticMeshes++];
}

// hash...ed..?