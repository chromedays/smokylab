#include "util.h"
#include "asset.h"
#include "camera.h"
#include "app.h"
#include "smk.h"
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
  initApp("Smokylab", 1920, 1080);

  smkRenderer renderer = smkCreateRenderer();

  // initRenderer();
  // initResourceManager();
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

#if 0
  Camera cam;
  initCameraLookingAtTarget(&cam, float3(-5, 1, 0), float3(-5, 1.105f, -1));

  // initGUI();
  GUI gui = {
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

    beginRender();
    {
      renderEditor(&editor);
      renderGUI();
    }
    endRender();
  }
#endif

  smkInitGUI(&renderer);

  Camera camera = {};
  initCameraLookingAtTarget(&camera, float3(-5, 1, 0), float3(-5, 1.105f, -1));

  smkScene scene = smkLoadSceneFromGLTFAsset(&renderer, "models/Sponza");
  smkScene scene2 = smkLoadSceneFromGLTFAsset(&renderer, "models/FlightHelmet");
  smkScene scene3 = smkMergeScene(&renderer, &scene, &scene2);

  while (gApp.running) {
    pollAppEvent();
    updateAppInput();

    moveCameraByInputs(&camera);

    smkRenderScene(&renderer, &scene3, &camera);

    smkSwapBuffers(&renderer);
  }

  smkDestroyScene(&scene3);
  smkDestroyScene(&scene2);
  smkDestroyScene(&scene);

  smkDestroyGUI();
  smkDestroyRenderer(&renderer);
  // destroyEditor(&editor);
  // destroyGUI();
  // destroyResourceManager();
  // destroyRenderer();

  destroyApp();
  destroyAssetLoader();

  return 0;
}
