cmake_minimum_required(VERSION 3.16)
project(rosbags_cpp VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CTest)
include(CMakePackageConfigHelpers)

option(ROSBAGS_BUILD_TESTS "Build rosbags_cpp tests" ON)
option(ROSBAGS_BUILD_TOOLS "Build rosbags_cpp command line tools" ON)
option(ROSBAGS_ENABLE_BZ2 "Build BZ2 support when the development library is available" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SQLite3 REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LZ4 REQUIRED IMPORTED_TARGET liblz4)
pkg_check_modules(ZSTD REQUIRED IMPORTED_TARGET libzstd)

set(ROSBAGS_HAS_BZ2 OFF)
if(ROSBAGS_ENABLE_BZ2)
  find_package(BZip2 QUIET)
  if(BZIP2_FOUND)
    set(ROSBAGS_HAS_BZ2 ON)
  endif()
endif()

add_library(rosbags_cpp
  src/rosbags.cpp
  src/rosbag1.cpp
  src/sqlite3.cpp
  src/codec.cpp
  src/mcap.cpp
)
add_library(rosbags_cpp::rosbags_cpp ALIAS rosbags_cpp)

target_include_directories(rosbags_cpp
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(rosbags_cpp
  PUBLIC SQLite::SQLite3 yaml-cpp PkgConfig::LZ4 PkgConfig::ZSTD
)
if(ROSBAGS_HAS_BZ2)
  target_link_libraries(rosbags_cpp PUBLIC BZip2::BZip2)
endif()
target_compile_definitions(rosbags_cpp
  PUBLIC ROSBAGS_HAS_BZ2=$<BOOL:${ROSBAGS_HAS_BZ2}>
         ROSBAGS_HAS_MCAP=0
)
target_compile_options(rosbags_cpp PRIVATE -Wall -Wextra -Wpedantic)

install(TARGETS rosbags_cpp EXPORT rosbags_cppTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT rosbags_cppTargets NAMESPACE rosbags_cpp:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rosbags_cpp)
configure_package_config_file(
  cmake/rosbags_cppConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/rosbags_cppConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/rosbags_cpp"
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/rosbags_cppConfigVersion.cmake"
  COMPATIBILITY SameMajorVersion
)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/rosbags_cppConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/rosbags_cppConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/rosbags_cpp"
)

message(STATUS "rosbags_cpp BZ2 support: ${ROSBAGS_HAS_BZ2}")
