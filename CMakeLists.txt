cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

include(cmake/CPM.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
enable_language(OBJC)
enable_language(OBJCXX)

# Downloads and configures external packages
cpmaddpackage(NAME SDL2 VERSION 2.0.16 URL
              https://libsdl.org/release/SDL2-2.0.16.zip)
cpmaddpackage(
  NAME
  imgui
  VERSION
  1.85
  GITHUB_REPOSITORY
  ocornut/imgui
  DOWNLOAD_ONLY
  TRUE)
cpmaddpackage(
  NAME
  cgltf
  VERSION
  1.11
  GITHUB_REPOSITORY
  jkuhlmann/cgltf
  DOWNLOAD_ONLY
  TRUE)
cpmaddpackage(
  NAME
  cute_headers
  GIT_TAG
  master
  GITHUB_REPOSITORY
  RandyGaul/cute_headers
  DOWNLOAD_ONLY
  TRUE)
cpmaddpackage(
  NAME
  stb
  GIT_TAG
  master
  GITHUB_REPOSITORY
  nothings/stb
  DOWNLOAD_ONLY
  TRUE)
if(APPLE)
  cpmaddpackage(
    NAME
    metalcpp
    GIT_TAG
    metal-cpp_macOS12_iOS15
    GITHUB_REPOSITORY
    bkaradzic/metal-cpp
    DOWNLOAD_ONLY
    TRUE)
endif()

if(imgui_ADDED)
  add_library(imgui STATIC)
  target_include_directories(
    imgui
    PUBLIC "${imgui_SOURCE_DIR}"
    PRIVATE "${SDL2_SOURCE_DIR}/include")
  target_sources(
    imgui
    PRIVATE "${imgui_SOURCE_DIR}/imgui.cpp"
            "${imgui_SOURCE_DIR}/imgui_demo.cpp"
            "${imgui_SOURCE_DIR}/imgui_draw.cpp"
            "${imgui_SOURCE_DIR}/imgui_tables.cpp"
            "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
            "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl.cpp")

  if(WIN32)
    target_sources(imgui
                   PRIVATE "${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp")
  elseif(APPLE)
    target_sources(imgui
                   PRIVATE "${imgui_SOURCE_DIR}/backends/imgui_impl_metal.mm")
    target_link_libraries(imgui PRIVATE SDL2 "-framework Metal"
                                        "-framework Foundation")
  endif()

  target_compile_options(imgui PRIVATE -fobjc-arc)
endif()
if(cgltf_ADDED)
  add_library(cgltf INTERFACE)
  target_include_directories(cgltf INTERFACE "${cgltf_SOURCE_DIR}")
endif()
if(cute_headers_ADDED)
  add_library(cute_headers INTERFACE)
  target_include_directories(cute_headers
                             INTERFACE "${cute_headers_SOURCE_DIR}")
endif()
if(stb_ADDED)
  add_library(stb INTERFACE)
  target_include_directories(stb INTERFACE "${stb_SOURCE_DIR}")
endif()
if(metalcpp_ADDED)
  add_library(metalcpp INTERFACE)
  target_include_directories(stb INTERFACE "${metalcpp_SOURCE_DIR}")
endif()

project(smokylab LANGUAGES C CXX OBJC OBJCXX)
add_executable(
  smokylab
  "${CMAKE_CURRENT_SOURCE_DIR}/src/app.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/str.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/vmath.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/asset.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/camera.cpp")

if(APPLE)
  # target_sources( smokylab PRIVATE
  # "${CMAKE_CURRENT_SOURCE_DIR}/src/metal_bindings.mm"
  # "${CMAKE_CURRENT_SOURCE_DIR}/src/smk_render_metal.mm")
  target_sources(
    smokylab PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/smk_render_metal.mm"
                     "${CMAKE_CURRENT_SOURCE_DIR}/src/smk_render_metal_aux.mm")
endif()

target_link_libraries(smokylab PRIVATE SDL2 imgui cgltf cute_headers stb)
if(WIN32)
  target_link_libraries(
    smokylab
    PRIVATE gdi32.lib
            Shell32.lib
            d3d12.lib
            d3d11.lib
            dxgi.lib
            dxguid.lib
            d3dcompiler.lib
            winmm.lib
            Shcore.lib)
elseif(APPLE)
  target_link_libraries(smokylab
                        PRIVATE "-framework Metal -framework Foundation")
endif()

if(WIN32)
  target_compile_definitions(smokylab PRIVATE SMK_WINDOWS=1 SMK_DIRECTX11=1)
elseif(APPLE)
  target_compile_definitions(smokylab PRIVATE SMK_MACOS=1 SMK_METAL=1)
endif()

set_property(TARGET smokylab PROPERTY CXX_STANDARD 20)
if(WIN32)
  set_property(TARGET smokylab PROPERTY C_STANDARD 17)
else()
  set_property(TARGET smokylab PROPERTY C_STANDARD 11)
endif()

target_compile_options(
  smokylab
  PRIVATE # -fsanitize=address
          -Wall
          -Wextra
          -Wpedantic
          -Wno-newline-eof
          -Wno-c++98-compat-pedantic
          -Wno-gnu-empty-initializer
          -Wno-old-style-cast
          -Wno-zero-as-null-pointer-constant
          -Wno-language-extension-token
          -Wno-double-promotion)
if(WIN32)
  target_compile_options(smokylab PRIVATE -Zi -Od -MT)
endif()

target_compile_definitions(smokylab PRIVATE _CRT_SECURE_NO_WARNINGS)
target_compile_definitions(smokylab PRIVATE "$<$<CONFIG:DEBUG>:DEBUG>")

add_custom_command(
  TARGET smokylab
  POST_BUILD
  COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/scripts/post_build.py"
          "$<TARGET_FILE_DIR:smokylab>/post_build_config.txt"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

file(
  GENERATE
  OUTPUT "$<TARGET_FILE_DIR:smokylab>/post_build_config.txt"
  CONTENT "${CMAKE_CURRENT_SOURCE_DIR}/shaders
$<TARGET_FILE_DIR:smokylab>")

file(
  GENERATE
  OUTPUT "$<TARGET_FILE_DIR:smokylab>/asset_config.txt"
  CONTENT "${CMAKE_CURRENT_SOURCE_DIR}/assets
$<TARGET_FILE_DIR:smokylab>")
