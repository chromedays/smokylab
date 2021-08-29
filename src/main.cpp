#include "render.h"
#include "util.h"
#include "editor_gui.h"
#include "asset.h"
#include "camera.h"
#include "app.h"
#include "editor.h"
#include "resource.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <Windows.h>
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#pragma clang diagnostic pop

int main(UNUSED int argc, UNUSED char **argv) {
  // Initialize asset paths
  initAssetLoaderFromConfigFile();
  initApp("Smokylab", 1280, 720);
  initRenderer();
  initResourceManager();

#if 0
  // clang-format off
  Float4 skyboxVertices[8] = {
      {-1, -1, -1},
      {-1, -1,  1},
      { 1, -1,  1},
      { 1, -1, -1},

      {-1,  1, -1},
      {-1,  1,  1},
      { 1,  1,  1},
      { 1,  1, -1},
  };

  uint32_t skyboxIndices[36] = {
      // Top
      4, 7, 6, 6, 5, 4,
      // Bottom
      1, 2, 3, 3, 0, 1,
      // Front
      7, 4, 0, 0, 3, 7,
      // Back
      5, 6, 2, 2, 1, 5,
      // Left
      4, 5, 1, 1, 0, 4,
      // Right
      6, 7, 3, 3, 2, 6,
  };
  // clang-format on
#endif

  Camera cam;
  initCameraLookingAtTarget(&cam, float3(-5, 1, 0), float3(-5, 1.105f, -1));

  initGUI();
  GUI gui = {
      .exposure = 1,
      .depthVisualizedRangeFar = 15,
      .lightAngle = 30,
      .lightIntensity = 10,
      .globalOpacity = 0.4f,
      .ssaoNumSamples = 10,
      .ssaoRadius = 0.5f,
      .ssaoScaleFactor = 1,
      .ssaoContrastFactor = 1,
      .cam = &cam,
      .models = NULL,
      .numModels = 0,
  };

  Editor editor;
  initEditor(&editor);

  LOG("Entering main loop.");

  while (gApp.running) {
    pollAppEvent();
    updateAppInput();

    updateEditor(&editor);
    updateGUI(&gui);

    renderEditor(&editor);

    renderGUI();

    swapBuffers();
  }

  destroyEditor(&editor);

  destroyGUI();

  destroyResourceManager();

  destroyRenderer();

  destroyApp();

  destroyAssetLoader();

  return 0;
}
