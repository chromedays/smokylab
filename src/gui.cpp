#include "gui.h"
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
}

void destroyGUI(void) {
  ImGui_ImplDX11_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
}

void handleGUIEvent(SDL_Event *event) { ImGui_ImplSDL2_ProcessEvent(event); }

void updateGUI(SDL_Window *window, GUI *gui) {
  ImGui_ImplDX11_NewFrame();
  ImGui_ImplSDL2_NewFrame(window);
  ImGui::NewFrame();

  int ww, wh;
  SDL_GetWindowSize(window, &ww, &wh);
  ImGui::SetNextWindowSize({(float)ww, (float)wh}, ImGuiCond_Once);
  ImGui::SetNextWindowPos({0, 0});
  ImGui::Begin("Main", NULL, ImGuiWindowFlags_NoDecoration);
  ImGui::SliderFloat("Exposure", &gui->exposure, 0.1f, 100.f);
  ImGui::Image((ImTextureID)gui->renderedView, {640, 360});
  gui->isHoveringScene = ImGui::IsItemHovered();
  ImGui::End();

  ImGui::Render();
}

void renderGUI(void) { ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); }

bool guiWantsCaptureMouse(void) { return ImGui::GetIO().WantCaptureMouse; }

C_INTERFACE_END
