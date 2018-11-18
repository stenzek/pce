set(SRCS
  include/imconfig.h
  include/imgui.h
  src/imgui.cpp
  src/imgui_demo.cpp
  src/imgui_draw.cpp
  src/imgui_internal.h
  src/stb_rect_pack.h
  src/stb_textedit.h
  src/stb_truetype.h
)

add_library(imgui ${SRCS})
target_include_directories(imgui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(imgui INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(WIN32)
  target_sources(imgui PRIVATE include/imgui_impl_dx11.h src/imgui_impl_dx11.cpp)
endif()

if(ENABLE_SDL_FRONTEND)
  target_sources(imgui PRIVATE include/imgui_impl_sdl_gl3.h src/imgui_impl_sdl_gl3.cpp)
  target_include_directories(imgui PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(imgui PRIVATE glad ${SDL2_LIBRARIES})
endif()

