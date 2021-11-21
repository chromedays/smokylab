#include "asset.h"
#include "util.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <cgltf.h>
#include <stb_image.h>
#pragma clang diagnostic pop

static String gAssetRootPath;
static String gShaderRootPath;

void initAssetLoader(const char *assetRootPath, const char *shaderRootPath);

void initAssetLoaderFromConfigFile(void) {
  String assetConfigPath = {};
  copyBasePath(&assetConfigPath);
  appendPathCStr(&assetConfigPath, "asset_config.txt");
  FILE *assetConfigFile = fopen(assetConfigPath.buf, "r");
  destroyString(&assetConfigPath);
  ASSERT(assetConfigFile);
  char assetBasePath[100] = {};
  char shaderBasePath[100] = {};
  fgets(assetBasePath, 100, assetConfigFile);
  for (int i = 0; i < 100; ++i) {
    if (assetBasePath[i] == '\n') {
      assetBasePath[i] = 0;
      break;
    }
  }
  fgets(shaderBasePath, 100, assetConfigFile);
  for (int i = 0; i < 100; ++i) {
    if (shaderBasePath[i] == '\n') {
      shaderBasePath[i] = 0;
      break;
    }
  }
  LOG("Asset base path: %s", assetBasePath);
  LOG("Shader base path: %s", shaderBasePath);

  fclose(assetConfigFile);
  initAssetLoader(assetBasePath, shaderBasePath);
}

void initAssetLoader(const char *assetRootPath, const char *shaderRootPath) {
  copyStringFromCStr(&gAssetRootPath, assetRootPath);
  copyStringFromCStr(&gShaderRootPath, shaderRootPath);
}

void destroyAssetLoader(void) {
  destroyString(&gShaderRootPath);
  destroyString(&gAssetRootPath);
}

void copyAssetRootPath(String *path) { copyString(path, &gAssetRootPath); }
void copyShaderRootPath(String *path) { copyString(path, &gShaderRootPath); }
