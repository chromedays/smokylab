cmake_minimum_required(VERSION 3.11)

add_executable(
  smokylab
  gui.cpp
  main.cpp
  render.cpp
  str.c
  vmath.c
  asset.cpp
  camera.c
  "${SMOKYLAB_DEP_DIR}/imgui/imgui_demo.cpp"
  "${SMOKYLAB_DEP_DIR}/imgui/imgui_draw.cpp"
  "${SMOKYLAB_DEP_DIR}/imgui/imgui_impl_dx11.cpp"
  "${SMOKYLAB_DEP_DIR}/imgui/imgui_impl_sdl.cpp"
  "${SMOKYLAB_DEP_DIR}/imgui/imgui_tables.cpp"
  "${SMOKYLAB_DEP_DIR}/imgui/imgui_widgets.cpp"
  "${SMOKYLAB_DEP_DIR}/imgui/imgui.cpp")
target_include_directories(smokylab PUBLIC "${SMOKYLAB_DEP_DIR}")
target_link_directories(smokylab PUBLIC "${SMOKYLAB_DEP_DIR}/SDL2")
target_link_libraries(
  smokylab
  PUBLIC SDL2.lib
  PUBLIC SDL2main.lib
  PUBLIC gdi32.lib
  PUBLIC Shell32.lib
  PUBLIC d3d11.lib
  PUBLIC dxgi.lib
  PUBLIC dxguid.lib
  PUBLIC d3dcompiler.lib
  PUBLIC winmm.lib
  PUBLIC Shcore.lib)

target_compile_definitions(smokylab PUBLIC _CRT_SECURE_NO_WARNINGS)
target_compile_definitions(smokylab PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")

add_custom_command(
  TARGET smokylab
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy "${SMOKYLAB_DEP_DIR}/SDL2/SDL2.dll"
          $<TARGET_FILE_DIR:smokylab>)

add_custom_command(
  TARGET smokylab
  POST_BUILD
  COMMAND python3 "${SMOKYLAB_SCRIPT_DIR}/post_build.py"
          "$<TARGET_FILE_DIR:smokylab>/post_build_config.txt"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

file(
  GENERATE
  OUTPUT "$<TARGET_FILE_DIR:smokylab>/post_build_config.txt"
  CONTENT "${SMOKYLAB_ROOT}/shaders
$<TARGET_FILE_DIR:smokylab>")

file(
  GENERATE
  OUTPUT "$<TARGET_FILE_DIR:smokylab>/asset_config.txt"
  CONTENT "${SMOKYLAB_ROOT}/assets
$<TARGET_FILE_DIR:smokylab>")
