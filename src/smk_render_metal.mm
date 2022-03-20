#include "asset.h"
#include "smk.h"
#include "app.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <SDL_metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <imgui.h>
#pragma clang diagnostic pop

C_INTERFACE_BEGIN

void queryNextDrawable();

smkShaderProgram smkLoadProgramFromShaderAsset(smkRenderer *renderer,
                                               const char *assetPath) {

  smkString basePath = {};
  copyShaderRootPath(&basePath);

  smkString shaderPath = {};
  copyString(&shaderPath, &basePath);
  appendPathCStr(&shaderPath, assetPath);
  appendCStr(&shaderPath, ".metallib");
}

void smkDestroyProgram(smkShaderProgram *program) {}
void smkUseProgram(smkRenderer *renderer, smkShaderProgram *program) {}

void smkDestroyTexture(smkTexture *texture) {}

smkRenderer smkCreateRenderer(void) {
  smkRenderer renderer = {};
  renderer.device = MTL::CreateSystemDefaultDevice();
  renderer.view = SDL_Metal_CreateView(gApp.window);
  renderer.commandQueue = renderer.device->newCommandQueue();
  renderer.drawableRenderDescriptor = MTL::RenderPassDescriptor::alloc()->init();
  renderer.drawableRenderDescriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
  renderer.drawableRenderDescriptor->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
  renderer.drawableRenderDescriptor->colorAttachments()->object(0)->setClearColor(MTL::ClearColor(0, 1, 1, 1));

  return {};
}
void smkDestroyRenderer(smkRenderer *renderer) {
  renderer->drawableRenderDescriptor->release();
  renderer->commandQueue->release();
  SDL_Metal_DestroyView(renderer->view);
  renderer->device->release();
}

smkScene smkLoadSceneFromGLTFAsset(smkRenderer *renderer,
                                   const char *assetPath) {
  return {};
}
void smkDestroyScene(smkScene *scene) {}
smkScene smkMergeScene(smkRenderer *renderer, const smkScene *sceneA,
                       const smkScene *sceneB) {
  return {};
}
void smkRenderScene(smkRenderer *renderer, const smkScene *scene,
                    const Camera *camera) {}
void smkSubmitRenderCommands(smkRenderer *renderer, int numRenderCommands,
                             smkRenderCommand *renderCommands) {}

void smkBeginRender(smkRenderer *renderer) {
  ASSERT(renderer);

  renderer->currentCommandBuffer = renderer->commandQueue->commandBuffer();

  CAMetalLayer *metalLayer =
      (__bridge CAMetalLayer *)SDL_Metal_GetLayer(renderer->view);
  renderer->currentDrawable =
      (__bridge CA::MetalDrawable *)[metalLayer nextDrawable];

  renderer->drawableRenderDescriptor->colorAttachments()->object(0)->setTexture(renderer->currentDrawable->texture());

  renderer->currentCommandEncoder = renderer->currentCommandBuffer->renderCommandEncoder(renderer->drawableRenderDescriptor);
  renderer->currentCommandEncoder->endEncoding();
}

void smkEndRender(smkRenderer *renderer) {
  ASSERT(renderer);

  renderer->currentCommandBuffer->presentDrawable(renderer->currentDrawable);
  renderer->currentCommandBuffer->commit();
  renderer->currentCommandBuffer->release();
  renderer->currentCommandEncoder->release();
}

void smkInitGUI(smkRenderer *renderer) { ImGui::CreateContext(); }
void smkDestroyGUI(void) { ImGui::DestroyContext(); }
C_INTERFACE_END