#pragma once
#include "util.h"

FORWARD_DECL(Mesh);
FORWARD_DECL(Vertex);
FORWARD_DECL(GPUBuffer);
typedef uint32_t VertexIndex;

C_INTERFACE_BEGIN

void initResourceManager(void);
void destroyResourceManager(void);

Mesh *allocateStaticMesh(int numSubMeshes);
Vertex *allocateVertices(int numVertices);
VertexIndex *allocateIndices(int numIndices);
// void refreshStaticBufferCache(void);
// typedef struct _GPUStaticBufferCache {
//   GPUBuffer *vertexBuffer;
//   GPUBuffer *indexBuffer;
// } GPUStaticBufferCache;
// GPUStaticBufferCache getStaticBufferCache(void);

C_INTERFACE_END