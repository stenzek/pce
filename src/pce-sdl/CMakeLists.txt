set(SRCS
    audio_sdl.cpp
    audio_sdl.h
    display_gl.cpp
    display_gl.h
    display_sdl.cpp
    display_sdl.h
    main.cpp
    scancodes_sdl.cpp
    scancodes_sdl.h
)

find_package(OpenGL REQUIRED)

add_executable(pce-sdl ${SRCS})
target_include_directories(pce-sdl PRIVATE "${SDL2_INCLUDE_DIRS}" "${OPENGL_INCLUDE_DIR}")
target_link_libraries(pce-sdl pce "${SDL2_LIBRARIES}" "${OPENGL_LIBRARIES}")
