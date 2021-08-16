#include "render.h"
#include "util.h"
#include "gui.h"
#include "asset.h"
#include "camera.h"
#include "app.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define CUTE_TIME_IMPLEMENTATION
#include <cute_time.h>
#pragma clang diagnostic pop

int main(UNUSED int argc, UNUSED char **argv) {
  // Initialize asset paths
  initAssetLoaderFromConfigFile();
  initApp("Smokylab", 1280, 720);
  initRenderer();

  ShaderProgram forwardPBRProgram = {};
  loadProgram("forward_pbr", &forwardPBRProgram);

  ID3D11Buffer *viewUniformBuffer;
  BufferDesc viewUniformBufferDesc = {
      .size = sizeof(ViewUniforms),
      .usage = D3D11_USAGE_DEFAULT,
      .bindFlags = D3D11_BIND_CONSTANT_BUFFER,
  };
  createBuffer(&viewUniformBufferDesc, &viewUniformBuffer);

  ID3D11Buffer *drawUniformBuffer;
  BufferDesc drawUniformBufferDesc = {
      .size = sizeof(DrawUniforms),
      .usage = D3D11_USAGE_DEFAULT,
      .bindFlags = D3D11_BIND_CONSTANT_BUFFER,
  };
  createBuffer(&drawUniformBufferDesc, &drawUniformBuffer);

  ID3D11Buffer *materialUniformBuffer;
  BufferDesc materialUniformBufferDesc = {
      .size = sizeof(MaterialUniforms),
      .usage = D3D11_USAGE_DEFAULT,
      .bindFlags = D3D11_BIND_CONSTANT_BUFFER,
  };
  createBuffer(&materialUniformBufferDesc, &materialUniformBuffer);

  int numModels = 0;
  Model *models[10] = {};
  {
    Model **model;
    model = &models[numModels++];
    *model = MMALLOC(Model);
    loadGLTFModel("models/Sponza", *model);
    model = &models[numModels++];
    *model = MMALLOC(Model);
    loadGLTFModel("models/FlightHelmet", *model);
    model = &models[numModels++];
    *model = MMALLOC(Model);
    loadGLTFModel("models/Planes", *model);
  }

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
      .models = models,
      .numModels = numModels,
  };

  float nearZ = 0.1f;
  float farZ = 500.f;

  ViewUniforms viewUniforms = {
      // .skySize = {skyWidth, skyHeight},
      .exposureNearFar = {1, nearZ, gui.depthVisualizedRangeFar},
  };
  // generateHammersleySequence(NUM_SAMPLES, viewUniforms.randomPoints);

  LOG("Entering main loop.");

  int lastCursorX = 0;
  int lastCursorY = 0;
  bool leftDown = false;
  bool rightDown = false;
  bool forwardDown = false;
  bool backDown = false;
  bool mouseDown = false;
  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      handleGUIEvent(&event);
      switch (event.type) {
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
        mouseDown = (event.button.state == SDL_PRESSED);
        break;
      case SDL_KEYDOWN:
      case SDL_KEYUP:
        forwardDown = processKeyboardEvent(&event, SDLK_w, forwardDown);
        backDown = processKeyboardEvent(&event, SDLK_s, backDown);
        leftDown = processKeyboardEvent(&event, SDLK_a, leftDown);
        rightDown = processKeyboardEvent(&event, SDLK_d, rightDown);
        break;
      case SDL_QUIT:
        running = false;
        break;
      }
    }

    updateGUI(&gui);
    viewUniforms.exposureNearFar.xz =
        float2(gui.exposure, gui.depthVisualizedRangeFar);
    viewUniforms.dirLightDirIntensity.xyz = float3Normalize(float3(
        cosf(degToRad(gui.lightAngle)), -1, sinf(degToRad(gui.lightAngle))));
    viewUniforms.dirLightDirIntensity.w = gui.lightIntensity;
    viewUniforms.overrideOpacity.x = gui.overrideOpacity ? 1 : 0;
    viewUniforms.overrideOpacity.y = gui.globalOpacity;
    viewUniforms.ssaoFactors = {(float)gui.ssaoNumSamples, gui.ssaoRadius,
                                gui.ssaoScaleFactor, gui.ssaoContrastFactor};

    float dt = ct_time();

    int ww, wh;
    SDL_GetWindowSize(gApp.window, &ww, &wh);

    int cursorX;
    int cursorY;
    SDL_GetMouseState(&cursorX, &cursorY);

    int cursorDeltaX = 0;
    int cursorDeltaY = 0;
    if (lastCursorX || lastCursorY) {
      cursorDeltaX = cursorX - lastCursorX;
      cursorDeltaY = cursorY - lastCursorY;
    }
    lastCursorX = cursorX;
    lastCursorY = cursorY;

    if (mouseDown && !guiWantsCaptureMouse()) {
      cam.yaw += (float)cursorDeltaX * 0.6f;
      cam.pitch -= (float)cursorDeltaY * 0.6f;
      cam.pitch = CLAMP(cam.pitch, -88.f, 88.f);
    }

    int dx = 0, dy = 0;
    if (leftDown) {
      dx -= 1;
    }
    if (rightDown) {
      dx += 1;
    }
    if (forwardDown) {
      dy += 1;
    }
    if (backDown) {
      dy -= 1;
    }

    float camMovementSpeed = 4.f;
    Float3 camMovement = (getRight(&cam) * (float)dx * camMovementSpeed +
                          getLook(&cam) * (float)dy * camMovementSpeed) *
                         dt;
    cam.pos += camMovement;

    viewUniforms.viewPos.xyz = cam.pos;
    viewUniforms.viewMat = getViewMatrix(&cam);
    viewUniforms.projMat =
        mat4Perspective(60, (float)ww / (float)wh, nearZ, farZ);

    updateBufferData(viewUniformBuffer, &viewUniforms);

    DrawUniforms drawUniforms = {
        .modelMat = mat4Identity(),
        .invModelMat = mat4Identity(),
    };
    updateBufferData(drawUniformBuffer, &drawUniforms);

    ID3D11Buffer *uniformBuffers[] = {viewUniformBuffer, drawUniformBuffer,
                                      materialUniformBuffer};
    bindBuffers(ARRAY_SIZE(uniformBuffers), uniformBuffers);

    setViewport(0, 0, (float)ww, (float)wh);

    {
      setDefaultRenderStates(float4(0, 0, 0, 0));
      useProgram(&forwardPBRProgram);
      for (int i = 0; i < numModels; ++i) {
        renderModel(models[i], drawUniformBuffer, materialUniformBuffer);
      }
    }

    renderGUI();

    swapBuffers();
  }

  destroyGUI();

  // destroySSAOResources();

  for (int i = 0; i < numModels; ++i) {
    destroyModel(models[i]);
    MFREE(models[i]);
  }

  COM_RELEASE(materialUniformBuffer);
  COM_RELEASE(drawUniformBuffer);
  COM_RELEASE(viewUniformBuffer);

  destroyProgram(&forwardPBRProgram);

  destroyRenderer();

  destroyApp();

  destroyAssetLoader();

  return 0;
}
