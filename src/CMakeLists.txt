cmake_minimum_required(VERSION 3.11)

include(FetchContent)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(smokylab LANGUAGES C CXX)

add_executable(
  smokylab
  gui.cpp
  main.cpp
  smokylab.cpp
  str.c
  vmath.c
  "../dep/imgui/imgui_demo.cpp"
  "../dep/imgui/imgui_draw.cpp"
  "../dep/imgui/imgui_impl_dx11.cpp"
  "../dep/imgui/imgui_impl_sdl.cpp"
  "../dep/imgui/imgui_tables.cpp"
  "../dep/imgui/imgui_widgets.cpp"
  "../dep/imgui/imgui.cpp")
target_include_directories(smokylab PUBLIC "../dep")
target_link_directories(smokylab PUBLIC "../dep/SDL2")
target_link_libraries(
  smokylab
  PUBLIC "SDL2.lib"
  PUBLIC "SDL2main.lib"
  PUBLIC gdi32.lib
  PUBLIC Shell32.lib
  PUBLIC d3d11.lib
  PUBLIC dxgi.lib
  PUBLIC dxguid.lib
  PUBLIC d3dcompiler.lib
  PUBLIC winmm.lib
  PUBLIC Shcore.lib)

# set_target_properties(smokylab PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG
# ${CMAKE_CURRENT_BINARY_DIR})

# add_custom_command( TARGET smokylab POST_BUILD COMMAND ${CMAKE_COMMAND} -E
# copy "../dep/SDL2/SDL2.dll" ${PROJECT_BINARY_DIR})
