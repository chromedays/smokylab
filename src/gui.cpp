#include "gui.h"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_impl_sdl.h>

C_INTERFACE_BEGIN

void initGUI(SDL_Window *window, SDL_GLContext glContext) {
  ImGui::CreateContext();
  ImGui::StyleColorsLight();
  ImGui_ImplSDL2_InitForOpenGL(window, glContext);
  // ImGui_ImplDX11_Init( , ID3D11DeviceContext *device_context)("#version
  // 130");
}

void destroyGUI(void) {
  // ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
}

void handleGUIEvent(SDL_Event *event) { ImGui_ImplSDL2_ProcessEvent(event); }

void updateGUI(SDL_Window *window, GUI *gui) {
  // ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame(window);
  ImGui::NewFrame();

  ImGui::Begin("Test");
  ImGui::SliderFloat("Exposure", &gui->exposure, 0.1f, 100.f);
  ImGui::End();

  ImGui::Render();
}

void renderGUI(void) {
  // ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool guiWantsCaptureMouse(void) { return ImGui::GetIO().WantCaptureMouse; }
C_INTERFACE_END