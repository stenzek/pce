set(SRCS
    cpu_8086/system.cpp
    cpu_8086/system.h
    cpu_8086/test186.cpp
    cpu_x86/system.cpp
    cpu_x86/system.h
    cpu_x86/test186.cpp
    cpu_x86/test386.cpp
    helpers.cpp
    helpers.h
    main.cpp
    stub_host_interface.cpp
    stub_host_interface.h
)

add_executable(pce-tests ${SRCS})
target_link_libraries(pce-tests pce gtest)
