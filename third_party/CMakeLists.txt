# Add SDL
# to use link with targets SDL2main SDL2-static
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
add_subdirectory(SDL)

# Add freetype
# to use link with target freetype
add_subdirectory(freetype)

# Add ImGui
# to use link with target Imgui
add_library(Imgui
        imgui/imgui.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        imgui/backends/imgui_impl_sdl.cpp
        imgui/backends/imgui_impl_sdlrenderer.cpp
        imgui/misc/freetype/imgui_freetype.cpp
        )
target_include_directories(Imgui PUBLIC
        imgui
        imgui/backends
        imgui/misc/freetype
        )
target_link_libraries(Imgui PUBLIC freetype SDL2main SDL2-static)
target_compile_definitions(Imgui PUBLIC IMGUI_ENABLE_FREETYPE)
if (WIN32 AND NOT (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
        target_link_libraries(Imgui PUBLIC "Imm32.lib")
endif()

# Add Asio
# to use link with target asio
add_library(Asio INTERFACE)
target_include_directories(Asio INTERFACE asio/asio/include)
