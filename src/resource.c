#include "resource.h"
#include "render.h"

// #define PUSH_STRUCT(p, type)                                                   \
//   (p = (__typeof__(p))((uint8_t *)p + sizeof(type)), (type *)p)
// #define PUSH_STRUCT_ARRAY(p, type, count)                                      \
//   (p = (__typeof__(p))((uint8_t *)p + sizeof(type) * count), (type *)p)

#if 0
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

typedef struct _TextureView {
  ID3D11Texture2D *texture;
  ID3D11ShaderResourceView *view;
} TextureView;

typedef struct _TextureHandle {
  int index;
} TextureHandle;
#endif

// TODO:
// - Test multiple meshes/models
// - Test multiple vertices/indices buffers

typedef struct _MeshBlock {
  int size;
  struct _MeshBlock *next;
  Mesh *mesh;
} MeshBlock;

typedef struct _MeshStorage {
  int size;
  int used;
  void *mem;
  MeshBlock *head;
  MeshBlock *tail;
} MeshStorage;

static void initMeshStorage(MeshStorage *storage, int reservedSizeInBytes) {
  *storage = (MeshStorage){};
  storage->size = reservedSizeInBytes;
  storage->mem = MMALLOC_ARRAY(uint8_t, storage->size);
  storage->head = storage->tail = (MeshBlock *)storage->mem;
}

static void destroyMeshStorage(MeshStorage *storage) { MFREE(storage->mem); }

static Mesh *allocateStaticMeshFromStorage(MeshStorage *storage,
                                           int numSubMeshes) {
  ASSERT(storage->tail);
  int blockSize = SSIZEOF32(MeshBlock) + SSIZEOF32(Mesh) +
                  numSubMeshes * SSIZEOF32(SubMesh);
  ASSERT((storage->used + blockSize) <= storage->size);
  MeshBlock *curr = storage->tail;
  curr->size = blockSize;
  curr->next = (MeshBlock *)((uint8_t *)curr + blockSize);
  Mesh *mesh = curr->mesh = (Mesh *)((uint8_t *)curr + sizeof(MeshBlock));
  mesh->numSubMeshes = numSubMeshes;
  mesh->subMeshes = (SubMesh *)((uint8_t *)mesh + sizeof(Mesh));
  storage->tail = curr->next;

  return mesh;
}

typedef struct _VertexStorage {
  int numVertices;
  int usedVertices;
  Vertex *vertices;
  GPUBuffer *vertexBufferCache;

  int numIndices;
  int usedIndices;
  VertexIndex *indices;
  GPUBuffer *indexBufferCache;
} VertexStorage;

static void initVertexStorage(VertexStorage *storage, int reservedNumVertices,
                              int reservedNumIndices) {
  *storage = (VertexStorage){};
  storage->numVertices = reservedNumVertices;
  storage->vertices = MMALLOC_ARRAY(Vertex, storage->numVertices);
  storage->numIndices = reservedNumIndices;
  storage->indices = MMALLOC_ARRAY(VertexIndex, storage->numIndices);
}

static void destroyVertexStorage(VertexStorage *storage) {
  destroyBuffer(storage->indexBufferCache);
  MFREE(storage->indices);
  destroyBuffer(storage->vertexBufferCache);
  MFREE(storage->vertices);
}

static Vertex *allocateVerticesFromStorage(VertexStorage *storage,
                                           int numVertices) {
  ASSERT((storage->usedVertices + numVertices) <= storage->numVertices);
  Vertex *result = storage->vertices + storage->usedVertices;
  storage->usedVertices += numVertices;
  return result;
}

static VertexIndex *allocateIndicesFromStorage(VertexStorage *storage,
                                               int numIndices) {
  ASSERT((storage->usedIndices + numIndices) <= storage->numIndices);
  VertexIndex *result = storage->indices + storage->usedIndices;
  storage->usedIndices += numIndices;
  return result;
}

typedef struct _ResourceTable {
  MeshStorage meshStorage;
  VertexStorage vertexStorage;

#if 0
  Material *materials;

  GPUTexture2D **textures;
  GPUTextureView ***textureViews; // tied with resource(buffer or texture) use
                                  // index as texture type?
  GPUSampler **samplers;          // hash samplers (they're not resources?)
#endif
} ResourceManager;

static ResourceManager gResourceManager;

#if 0
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
#endif

void initResourceManager(void) {
  initMeshStorage(&gResourceManager.meshStorage, 10000);
  initVertexStorage(&gResourceManager.vertexStorage, 600000, 2400000);
}

void destroyResourceManager(void) {
  destroyVertexStorage(&gResourceManager.vertexStorage);
  destroyMeshStorage(&gResourceManager.meshStorage);
}

Mesh *allocateStaticMesh(int numSubMeshes) {
  Mesh *result = allocateStaticMeshFromStorage(&gResourceManager.meshStorage,
                                               numSubMeshes);
  return result;
}

Vertex *allocateVertices(int numVertices, int *offset) {
  if (offset) {
    *offset = gResourceManager.vertexStorage.usedVertices;
  }
  Vertex *result =
      allocateVerticesFromStorage(&gResourceManager.vertexStorage, numVertices);
  return result;
}

VertexIndex *allocateIndices(int numIndices, int *offset) {
  if (offset) {
    *offset = gResourceManager.vertexStorage.usedIndices;
  }
  VertexIndex *result =
      allocateIndicesFromStorage(&gResourceManager.vertexStorage, numIndices);
  return result;
}

void refreshStaticBufferCache(void) {
  VertexStorage *storage = &gResourceManager.vertexStorage;

  destroyBuffer(storage->vertexBufferCache);
  createImmutableVertexBuffer(storage->usedVertices, storage->vertices,
                              &storage->vertexBufferCache);

  destroyBuffer(storage->indexBufferCache);
  createImmutableIndexBuffer(storage->usedIndices, storage->indices,
                             &storage->indexBufferCache);
}

GPUStaticBufferCache getStaticBufferCache(void) {
  return (GPUStaticBufferCache){
      .vertexBuffer = gResourceManager.vertexStorage.vertexBufferCache,
      .indexBuffer = gResourceManager.vertexStorage.indexBufferCache,
  };
}

// SubMesh *allocateSubMesh(void) {
//   ASSERT(gResourceManager.numStaticSubMeshes < RESERVED_NUM_SUBMESHES);
//   return &gResourceManager
//               .staticSubMeshes[gResourceManager.numStaticSubMeshes++];
// }

// Mesh *allocateStaticMesh(void) {
//   ASSERT(gResourceManager.numStaticMeshes < RESERVED_NUM_MESHES);
//   return &gResourceManager.staticMeshes[gResourceManager.numStaticMeshes++];
// }

// hash...ed..?

// PushMesh()
// PushSubMesh() { ..append submesh to last mesh }