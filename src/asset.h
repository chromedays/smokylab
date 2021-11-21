#pragma once
#include "str.h"
#include "util.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#pragma clang diagnostic pop

C_INTERFACE_BEGIN

void initAssetLoaderFromConfigFile(void);
void destroyAssetLoader(void);

void copyAssetRootPath(smkString *path);
void copyShaderRootPath(smkString *path);

C_INTERFACE_END