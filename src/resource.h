#pragma once
#include "util.h"

FORWARD_DECL(Mesh);
FORWARD_DECL(Vertex);
typedef uint32_t VertexIndex;

C_INTERFACE_BEGIN

void initResourceManager(void);
void destroyResourceManager(void);

Mesh *allocateStaticMesh(int numSubMeshes);
Vertex *allocateVertices(int numVertices);
VertexIndex *allocateIndices(int numIndices);

C_INTERFACE_END