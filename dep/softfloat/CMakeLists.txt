set(SRCS
    include/softfloat-specialize.h
    include/softfloat.h
    include/softfloatx80.h
    src/fpu_constant.h
    src/softfloat-compare.h
    src/softfloat-macros.h
    src/softfloat-round-pack.h
    src/f2xm1.cc
    src/fpatan.cc
    src/fprem.cc
    src/fsincos.cc
    src/fyl2x.cc
    src/poly.cc
    src/softfloat-muladd.cc
    src/softfloat-round-pack.cc
    src/softfloat-specialize.cc
    src/softfloat.cc
    src/softfloat16.cc
    src/softfloatx80.cc
)

add_library(softfloat ${SRCS})
target_include_directories(softfloat PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(softfloat INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

