set(SRCS
    main.cpp
)

add_executable(pce-disasm ${SRCS})
target_link_libraries(pce-disasm pce)
