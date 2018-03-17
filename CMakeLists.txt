cmake_minimum_required(VERSION 3.1)
project(pce C CXX)

# Apparently GCC6+ appends ABI-specific function names and gdb doesn't like it.
# Not being able to set breakpoints is annoying, so let's just turn it off.
if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.0)
    add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
endif()

# Set _DEBUG macro for Debug builds.
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")

# Detect C++ version support.
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
    CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
    if(COMPILER_SUPPORTS_CXX14)
        message("Enabling C++14 support.")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
    elseif(COMPILER_SUPPORTS_CXX14)
        message(WARNING "No C++14 support, enabling C++11 support.")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    else()
        message(WARNING "No C++11 support. This may cause issues.")
    endif()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-switch")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-switch")
endif()

# Enable threads everywhere.
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_THREAD_LIBS_INIT}")
#if(CMAKE_USE_PTHREADS_INIT)
    #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
    #    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")
    #endif()

# Find SDL2
find_package(SDL2)

enable_testing()
add_subdirectory(dep)
add_subdirectory(src)
