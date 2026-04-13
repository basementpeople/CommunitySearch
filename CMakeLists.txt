cmake_minimum_required(VERSION 3.16)
project(CSP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 默认构建类型
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# 源文件
set(CSP_SOURCES
  src/main.cpp
  src/graph/Graph.cpp
  src/graph/TreeIndex.cpp
  src/graph/SharingIndex.cpp
  src/graph/VertexScore.cpp
)

add_executable(csp ${CSP_SOURCES})

# 头文件目录
target_include_directories(csp PRIVATE
  ${PROJECT_SOURCE_DIR}/src/include
)

# 编译优化（按编译器区分）
if(MSVC)
  # 仅在 Release 开优化，避免与 Debug 默认 /RTC1 冲突
  target_compile_options(csp PRIVATE
    $<$<CONFIG:Release>:/O2 /DNDEBUG>
    $<$<CONFIG:RelWithDebInfo>:/O2 /Zi /DNDEBUG>
    $<$<CONFIG:Debug>:/Od /Zi>
  )
else()
  target_compile_options(csp PRIVATE
    -O3
    -finline-functions
    -funroll-loops
  )
  # -march=native 在跨机器分发时不建议默认开
  option(ENABLE_NATIVE_OPT "Enable -march=native" OFF)
  if(ENABLE_NATIVE_OPT)
    target_compile_options(csp PRIVATE -march=native)
  endif()

  include(CheckIPOSupported)
  check_ipo_supported(RESULT ipo_supported OUTPUT ipo_output)
  if(ipo_supported)
    set_property(TARGET csp PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  endif()
endif()