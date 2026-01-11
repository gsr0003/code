# cpp_backend/CMakeLists.txt
# 子目录版本，支持模块化

# 项目设置
set(PROJECT_NAME cpp_backend)
project(${PROJECT_NAME})

# 查找依赖
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 REQUIRED)

# 设置源文件分组
set(COMMON_SOURCES
    src/Calculator.cpp
    src/ImageProcessor.cpp
    src/common/Logger.cpp
    src/common/MathUtils.cpp
)

set(BINDING_SOURCES
    src/bindings/main_bindings.cpp
    src/bindings/calculator_bindings.cpp
    src/bindings/image_processor_bindings.cpp
    src/bindings/utilities_bindings.cpp
)

# 创建静态库
add_library(${PROJECT_NAME}_core STATIC ${COMMON_SOURCES})

target_include_directories(${PROJECT_NAME}_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(${PROJECT_NAME}_core PUBLIC cxx_std_17)

# 创建Python模块
pybind11_add_module(${PROJECT_NAME} ${BINDING_SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_core)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Python3_INCLUDE_DIRS}
)

# 平台特定的设置
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        SUFFIX ".pyd"
        PREFIX ""
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
else()
    set_target_properties(${PROJECT_NAME} PROPERTIES
        SUFFIX ".so"
        PREFIX ""
    )
endif()

# 设置输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../python
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../python
)

# 安装配置
install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/python
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(TARGETS ${PROJECT_NAME}_core
    EXPORT ${PROJECT_NAME}Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

# 导出包配置
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)