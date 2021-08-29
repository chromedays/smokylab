#pragma once
#include "util.h"

FORWARD_DECL(Mesh);

C_INTERFACE_BEGIN

void initResourceManager(void);
void destroyResourceManager(void);
Mesh *allocateStaticMesh(void);

C_INTERFACE_END