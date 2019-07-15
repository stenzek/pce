if(WIN32)
  set(SDL2_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/dep/msvc/include/SDL")
  set(SDL2_LIBRARIES "SDL2")
else()
  find_package(SDL2 REQUIRED)
endif()

set(SRCS
  audio_sdl.cpp
  audio_sdl.h
  host_interface.cpp
  host_interface.h
  main.cpp
  scancodes_sdl.cpp
  scancodes_sdl.h
)

add_executable(pce-sdl ${SRCS})
target_include_directories(pce-sdl PRIVATE "${SDL2_INCLUDE_DIRS}")
target_link_libraries(pce-sdl pce imgui nativefiledialog "${SDL2_LIBRARIES}")
