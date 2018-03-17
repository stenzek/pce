set(SRCS
    helpers.cpp
    helpers.h
    main.cpp
    testpcsystem.cpp
    testpcsystem.h
    tests/test186.cpp
    tests/test386.cpp
)

add_executable(pce-tests ${SRCS})
target_link_libraries(pce-tests pce gtest)
