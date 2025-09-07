# cmake版本要求
cmake_minimum_required(VERSION 3.10)

# 工程名称
project(fftest)

# 设置编译器
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 设置为debug可调试
set(CMAKE_BUILD_TYPE Debug)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)

# 设置ffmpeg库路径
set(FFMPEG_ROOT_PATH ${CMAKE_SOURCE_DIR}/../install)
set(FFMPEG_HEAD_PATH ${FFMPEG_ROOT_PATH}/include)
set(FFMPEG_LIB_PATH ${FFMPEG_ROOT_PATH}/lib)

 #自定义pkgconfig目录的路径
set(ENV{PKG_CONFIG_PATH} ${FFMPEG_LIB_PATH}/pkgconfig)
find_package(PkgConfig REQUIRED)

pkg_check_modules(libavcodec    REQUIRED IMPORTED_TARGET libavcodec)
pkg_check_modules(libavdevice   REQUIRED IMPORTED_TARGET libavdevice)
pkg_check_modules(libavfilter   REQUIRED IMPORTED_TARGET libavfilter)
pkg_check_modules(libavformat   REQUIRED IMPORTED_TARGET libavformat)
pkg_check_modules(libavutil     REQUIRED IMPORTED_TARGET libavutil)
pkg_check_modules(libpostproc   REQUIRED IMPORTED_TARGET libpostproc)
pkg_check_modules(libswresample REQUIRED IMPORTED_TARGET libswresample)
pkg_check_modules(libswscale    REQUIRED IMPORTED_TARGET libswscale)

# 可执行程序输出路径
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

# 参与编译的文件
aux_source_directory(${CMAKE_SOURCE_DIR}/src SRC_FILES)

# 生成可执行程序
add_executable(${PROJECT_NAME} ${SRC_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)

# 链接依赖库
target_link_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_LIB_PATH})

target_include_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_HEAD_PATH})
target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::libavfilter)
target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::libavformat)
target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::libavcodec)
target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::libavutil)
target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::libswresample)
target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::libswscale)
target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::libpostproc)
target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::libavdevice)