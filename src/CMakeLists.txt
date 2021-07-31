cmake_minimum_required(VERSION 3.2)
project(smokylab LANGUAGES C CXX)
add_executable(smokylab gui.cpp
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
target_link_libraries(smokylab PUBLIC "SDL2.lib"
                               PUBLIC "SDL2main.lib"
                               PUBLIC gdi32.lib
                               PUBLIC Shell32.lib
                               PUBLIC d3d11.lib
                               PUBLIC dxgi.lib
                               PUBLIC dxguid.lib
                               PUBLIC d3dcompiler.lib
                               PUBLIC winmm.lib
                               PUBLIC Shcore.lib)