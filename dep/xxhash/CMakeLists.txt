set(SRCS
    xxhash.c
)

add_library(xxhash ${SRCS})
target_include_directories(xxhash PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(xxhash INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(xxhash Threads::Threads)
