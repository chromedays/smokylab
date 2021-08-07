#pragma once
#include "str.h"
#include "util.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <d3d11_1.h>
#pragma clang diagnostic pop

FORWARD_DECL(ShaderProgram);
FORWARD_DECL(Model);

C_INTERFACE_BEGIN

void initAssetLoader(const char *assetRootPath, const char *shaderRootPath);
void destroyAssetLoader(void);

void copyAssetRootPath(String *path);

void loadProgram(const char *baseName, ShaderProgram *program);

void loadGLTFModel(const char *path, Model *model);

void loadIBLTexture(const char *baseName, int *skyWidth, int *skyHeight,
                    ID3D11Texture2D **skyTex, ID3D11Texture2D **irrTex,
                    ID3D11ShaderResourceView **skyTexView,
                    ID3D11ShaderResourceView **irrTexView);

C_INTERFACE_END