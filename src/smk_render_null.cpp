#include "smk.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <imgui/imgui.h>
#pragma clang diagnostic pop

C_INTERFACE_BEGIN

smkShaderProgram smkLoadProgramFromShaderAsset(smkRenderer *renderer,
                                               const char *assetPath) {
  return {};
}
void smkDestroyProgram(smkShaderProgram *program) {}
void smkUseProgram(smkRenderer *renderer, smkShaderProgram *program) {}

void smkDestroyTexture(smkTexture *texture) {}

smkRenderer smkCreateRenderer(void) { return {}; }
void smkDestroyRenderer(smkRenderer *renderer) {}

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

void smkSwapBuffers(smkRenderer *renderer) {}

void smkInitGUI(smkRenderer *renderer) { ImGui::CreateContext(); }
void smkDestroyGUI(void) { ImGui::DestroyContext(); }
C_INTERFACE_END