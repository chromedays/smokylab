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

    smkViewUniforms viewUniforms = {};
    viewUniforms.viewMat = getViewMatrix(&camera);
    viewUniforms.projMat = getProjMatrix(&camera);
    renderer.context->UpdateSubresource(renderer.viewUniformBuffer.handle, 0,
                                        NULL, &viewUniforms, 0, 0);

    int windowWidth, windowHeight;
    SDL_GetWindowSize(gApp.window, &windowWidth, &windowHeight);

    D3D11_VIEWPORT viewport = {
        .TopLeftX = 0,
        .TopLeftY = 0,
        .Width = (FLOAT)windowWidth,
        .Height = (FLOAT)windowHeight,
        .MinDepth = 0,
        .MaxDepth = 1,
    };
    renderer.context->RSSetViewports(1, &viewport);

    Float4 clearColor = {0.5f, 0.5f, 1, 1};
    renderer.context->OMSetRenderTargets(1, &renderer.swapChainRTV,
                                         renderer.swapChainDSV);
    renderer.context->ClearRenderTargetView(renderer.swapChainRTV,
                                            (FLOAT *)&clearColor);
    renderer.context->ClearDepthStencilView(renderer.swapChainDSV,
                                            D3D11_CLEAR_DEPTH, 0, 0);

    renderer.context->RSSetState(renderer.defaultRasterizerState);
    renderer.context->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    renderer.context->OMSetDepthStencilState(renderer.defaultDepthStencilState,
                                             0);
    renderer.context->OMSetBlendState(NULL, NULL, 0xFFFFFFFF);

#if 1
    ID3D11Buffer *uniformBuffers[] = {renderer.viewUniformBuffer.handle,
                                      renderer.drawUniformBuffer.handle,
                                      renderer.materialUniformBuffer.handle};

    renderer.context->VSSetConstantBuffers(0, ARRAY_SIZE(uniformBuffers),
                                           uniformBuffers);
    renderer.context->PSSetConstantBuffers(0, ARRAY_SIZE(uniformBuffers),
                                           uniformBuffers);

    renderer.context->IASetInputLayout(renderer.forwardPBRProgram.vertexLayout);
    renderer.context->VSSetShader(renderer.forwardPBRProgram.vertexShader, NULL,
                                  0);
    renderer.context->PSSetShader(renderer.forwardPBRProgram.fragmentShader,
                                  NULL, 0);

    smkRenderScene(&renderer, &scene3);
#endif

    renderer.swapChain->Present(0, 0);
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
