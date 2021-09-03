#pragma once
#include "util.h"

FORWARD_DECL(SubMesh);
FORWARD_DECL(Mesh);

C_INTERFACE_BEGIN

void initResourceManager(void);
void destroyResourceManager(void);

SubMesh *allocateSubMesh(void);
Mesh *allocateStaticMesh(void);

C_INTERFACE_END