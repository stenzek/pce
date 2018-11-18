set(SRCS
    ini.c
    cpp/INIReader.cpp
)

add_library(inih ${SRCS})
target_include_directories(inih PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/cpp")
target_include_directories(inih INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/cpp")
