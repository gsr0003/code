# 根目录 CMakeLists.txt
cmake_minimum_required(VERSION 3.18)
project(CppPythonGUI
    VERSION 1.0.0
    DESCRIPTION "Python GUI with C++ Backend"
    LANGUAGES CXX
)

# 设置默认构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING 
        "Build type (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
endif()

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译器特定设置
if(MSVC)
    add_compile_options(/W4 /WX /EHsc /MP)
    add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
    add_compile_options(-Wno-unused-parameter -Wno-missing-field-initializers)
endif()

# 添加子目录
add_subdirectory(cpp_backend)
add_subdirectory(tests)  # 可选：C++单元测试