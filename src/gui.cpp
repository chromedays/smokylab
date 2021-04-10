#include "gui.h"
#include "smokylab.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_impl_sdl.h>
#pragma clang diagnostic pop

C_INTERFACE_BEGIN

void initGUI(SDL_Window *window, ID3D11Device *device,
             ID3D11DeviceContext *deviceContext) {
  ImGui::CreateContext();
  ImGui::StyleColorsLight();
  ImGui_ImplSDL2_InitForD3D(window);
  ImGui_ImplDX11_Init(device, deviceContext);
  ImGui::GetStyle().Alpha = 0.6f;

  ImVec4 *colors = ImGui::GetStyle().Colors;
  colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.00f, 1.00f, 0.00f, 0.57f);
  colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
  // colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
  colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
  colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.60f);
  // colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.39f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 0.67f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 1.00f, 0.00f, 0.78f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(1.00f, 1.00f, 1.00f, 0.57f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.00f, 1.00f, 0.00f, 0.57f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
  colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
  colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.57f);
  colors[ImGuiCol_Button] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.57f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
  colors[ImGuiCol_Header] = ImVec4(0.00f, 1.00f, 0.00f, 0.78f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.39f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 1.00f, 0.00f, 0.57f);
  colors[ImGuiCol_Separator] = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(0.72f, 0.72f, 0.72f, 0.78f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
  colors[ImGuiCol_ResizeGrip] = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);
  colors[ImGuiCol_Tab] = ImVec4(0.24f, 0.79f, 0.69f, 1.00f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
}

void destroyGUI(void) {
  ImGui_ImplDX11_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
}

void handleGUIEvent(SDL_Event *event) { ImGui_ImplSDL2_ProcessEvent(event); }

static void guiPostProcessingMenu(GUI *gui, UNUSED void *userdata) {
  ImGui::SliderFloat("Exposure", &gui->exposure, 0.1f, 10.f);
}

static void guiSceneNodeTree(const Model *model, const SceneNode *node) {
  if (node->numChildNodes > 0) {
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode(node->name.buf)) {
      for (int i = 0; i < node->numChildNodes; ++i) {
        const SceneNode *childNode = &model->nodes[node->childNodes[i]];
        guiSceneNodeTree(model, childNode);
      }
      ImGui::TreePop();
    }
  } else {
    ImGui::Indent();
    ImGui::TextUnformatted(node->name.buf);
    ImGui::Unindent();
  }
}

static void guiSceneMenu(GUI *gui, UNUSED void *userdata) {
  ImGui::SetNextItemOpen(true, ImGuiCond_Once);

  const Model *model = gui->model;
  for (int sceneIndex = 0; sceneIndex < model->numScenes; ++sceneIndex) {
    const Scene *scene = &model->scenes[sceneIndex];
    if (ImGui::TreeNode(scene->name.buf)) {
      for (int nodeIndex = 0; nodeIndex < scene->numNodes; ++nodeIndex) {
        const SceneNode *node = &model->nodes[scene->nodes[nodeIndex]];
        guiSceneNodeTree(model, node);
      }
      ImGui::TreePop();
    }
  }
}

typedef struct _MainMenu {
  const char *title;
  void (*guiFunc)(GUI *, void *);
} MainMenu;
static int gFocusedMainMenuID = -1;
typedef struct _MainMenuContext {
  int numMenues;
  MainMenu menues[40];
} MainMenuContext;
static void pushMainMenu(MainMenuContext *context, const char *title,
                         void (*guiFunc)(GUI *, void *)) {
  context->menues[context->numMenues++] = {
      .title = title,
      .guiFunc = guiFunc,
  };
}
static void renderMainMenues(MainMenuContext *mmctx, GUI *gui) {
  int newFocusedMenuID = gFocusedMainMenuID;

  const float focusButtonWidth = 50;
  const float backButtonWidth = 50;

  ImGui::AlignTextToFramePadding();
  if (gFocusedMainMenuID >= 0) {
    ImGui::TextUnformatted(mmctx->menues[gFocusedMainMenuID].title);
    ImGui::SameLine(ImGui::GetWindowWidth() - backButtonWidth - 4);
    if (ImGui::Button("Back", {backButtonWidth, 0})) {
      newFocusedMenuID = -1;
    }
    ImGui::Separator();
    mmctx->menues[gFocusedMainMenuID].guiFunc(gui, NULL);
  } else {
    for (int i = 0; i < mmctx->numMenues; ++i) {
      ImGui::PushID(i);
      bool headerOpen = ImGui::CollapsingHeader(
          mmctx->menues[i].title, ImGuiTreeNodeFlags_AllowItemOverlap);
      ImGui::PopID();
      ImGui::SameLine(ImGui::GetWindowWidth() - focusButtonWidth - 4);
      ImGui::PushID(i);
      if (ImGui::Button("Focus", {focusButtonWidth, 0})) {
        newFocusedMenuID = i;
      }
      ImGui::PopID();

      if (headerOpen) {
        mmctx->menues[i].guiFunc(gui, NULL);
      }
    }
  }
  gFocusedMainMenuID = newFocusedMenuID;
}

void updateGUI(SDL_Window *window, GUI *gui) {
  ImGui_ImplDX11_NewFrame();
  ImGui_ImplSDL2_NewFrame(window);

  ImGui::NewFrame();

  // ImGui::ShowDemoWindow();

  int ww, wh;
  SDL_GetWindowSize(window, &ww, &wh);
  ImGui::SetNextWindowSize({300, 0});
  ImGui::SetNextWindowPos({0, 0});
  ImGui::Begin("#Main", NULL, ImGuiWindowFlags_NoDecoration);

  MainMenuContext mmctx = {};
  pushMainMenu(&mmctx, "Post Processing", guiPostProcessingMenu);
  pushMainMenu(&mmctx, "Scene", guiSceneMenu);
  renderMainMenues(&mmctx, gui);

  ImGui::End();
  ImGui::Render();
}

void renderGUI(void) { ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); }

bool guiWantsCaptureMouse(void) { return ImGui::GetIO().WantCaptureMouse; }

C_INTERFACE_END
