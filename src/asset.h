#pragma once
#include "str.h"
#include "util.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#pragma clang diagnostic pop

C_INTERFACE_BEGIN

void initAssetLoaderFromConfigFile(void);
void destroyAssetLoader(void);

void copyAssetRootPath(String *path);
void copyShaderRootPath(String *path);

C_INTERFACE_END