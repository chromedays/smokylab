#pragma once
#include "str.h"
#include "util.h"
#include "render.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#pragma clang diagnostic pop

C_INTERFACE_BEGIN

void initAssetLoaderFromConfigFile(void);
void initAssetLoader(const char *assetRootPath, const char *shaderRootPath);
void destroyAssetLoader(void);

void copyAssetRootPath(String *path);

ShaderProgram loadProgram(const char *baseName);

void loadGLTFModel(const char *path, Model *model);

// void loadIBLTexture(const char *baseName, int *skyWidth, int *skyHeight,
//                     GPUTexture2D **skyTex, GPUTexture2D **irrTex,
//                     ID3D11ShaderResourceView **skyTexView,
//                     ID3D11ShaderResourceView **irrTexView);

C_INTERFACE_END