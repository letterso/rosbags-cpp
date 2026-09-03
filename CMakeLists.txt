cmake_minimum_required(VERSION 3.16)
project(rosbags_cpp VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CTest)
include(CMakePackageConfigHelpers)

option(ROSBAGS_BUILD_TESTS "Build rosbags_cpp tests" ON)
option(ROSBAGS_BUILD_TOOLS "Build rosbags_cpp command line tools" ON)
option(ROSBAGS_ENABLE_MCAP "Enable MCAP backend and fetch/build MCAP when unavailable" ON)
option(ROSBAGS_ENABLE_BZ2 "Build BZ2 support when the development library is available" ON)
set(ROSBAGS_MCAP_ROOT "" CACHE PATH "Optional prefix containing the official MCAP C++ SDK")
set(ROSBAGS_MCAP_BUILDER_REPOSITORY "https://github.com/olympus-robotics/mcap_builder.git" CACHE STRING "MCAP CMake builder repository")
set(ROSBAGS_MCAP_BUILDER_TAG "main" CACHE STRING "MCAP CMake builder git tag, branch, or commit")

if(ROSBAGS_ENABLE_MCAP AND CMAKE_VERSION VERSION_LESS 3.22.1)
  message(FATAL_ERROR "ROSBAGS_ENABLE_MCAP requires CMake 3.22.1 or newer for the mcap_builder FetchContent workflow")
endif()

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

set(ROSBAGS_HAS_MCAP OFF)
set(ROSBAGS_MCAP_EXTERNAL OFF)
if(ROSBAGS_ENABLE_MCAP)
  if(ROSBAGS_MCAP_ROOT)
    find_path(ROSBAGS_MCAP_INCLUDE_DIR NAMES mcap/reader.hpp
      PATHS "${ROSBAGS_MCAP_ROOT}/include/mcap_vendor" "${ROSBAGS_MCAP_ROOT}/include"
      NO_DEFAULT_PATH)
    find_library(ROSBAGS_MCAP_LIBRARY NAMES mcap PATHS "${ROSBAGS_MCAP_ROOT}/lib" NO_DEFAULT_PATH)
    if(ROSBAGS_MCAP_INCLUDE_DIR AND ROSBAGS_MCAP_LIBRARY)
      set(ROSBAGS_HAS_MCAP ON)
      set(ROSBAGS_MCAP_EXTERNAL ON)
    endif()
  else()
    find_package(mcap CONFIG QUIET)
    if((mcap_FOUND OR MCAP_FOUND) AND (TARGET mcap::mcap OR TARGET mcap))
      set(ROSBAGS_HAS_MCAP ON)
    endif()
  endif()
  if(NOT ROSBAGS_HAS_MCAP)
    include(FetchContent)
    FetchContent_Declare(
      mcap_builder
      GIT_REPOSITORY "${ROSBAGS_MCAP_BUILDER_REPOSITORY}"
      GIT_TAG "${ROSBAGS_MCAP_BUILDER_TAG}"
      GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(mcap_builder)
    if(TARGET mcap::mcap OR TARGET mcap)
      set(ROSBAGS_HAS_MCAP ON)
    else()
      message(FATAL_ERROR "MCAP was enabled, but mcap_builder did not provide an mcap target")
    endif()
  endif()
endif()

add_library(rosbags_cpp
  src/rosbags.cpp
  src/rosbag1.cpp
  src/sqlite3.cpp
  src/codec.cpp
  src/codegen.cpp
  src/profiles.cpp
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
if(ROSBAGS_HAS_MCAP)
  if(ROSBAGS_MCAP_EXTERNAL)
    target_include_directories(rosbags_cpp PUBLIC "${ROSBAGS_MCAP_INCLUDE_DIR}")
    target_link_libraries(rosbags_cpp PUBLIC "${ROSBAGS_MCAP_LIBRARY}")
  elseif(TARGET mcap::mcap)
    target_link_libraries(rosbags_cpp PUBLIC mcap::mcap)
  elseif(TARGET mcap)
    target_link_libraries(rosbags_cpp PUBLIC mcap)
  endif()
endif()
target_compile_definitions(rosbags_cpp
  PUBLIC ROSBAGS_HAS_BZ2=$<BOOL:${ROSBAGS_HAS_BZ2}>
         ROSBAGS_HAS_MCAP=$<BOOL:${ROSBAGS_HAS_MCAP}>
)
target_compile_options(rosbags_cpp PRIVATE -Wall -Wextra -Wpedantic)

if(ROSBAGS_BUILD_TOOLS)
  add_executable(rosbags-info tools/rosbags_info.cpp)
  target_link_libraries(rosbags-info PRIVATE rosbags_cpp)
  add_executable(rosbags-read tools/rosbags_read.cpp)
  target_link_libraries(rosbags-read PRIVATE rosbags_cpp)
  add_executable(rosbags-gen tools/rosbags_gen.cpp)
  target_link_libraries(rosbags-gen PRIVATE rosbags_cpp)
endif()

if(ROSBAGS_BUILD_TESTS)
  # doctest v2.5.3 is vendored as its official single header.  No doctest
  # library is linked; test_main.cpp provides the header-only implementation.
  set(ROSBAGS_DOCTEST_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests/third_party")
  add_executable(rosbags_cpp_tests tests/test_main.cpp tests/test_project.cpp)
  target_include_directories(rosbags_cpp_tests PRIVATE "${ROSBAGS_DOCTEST_INCLUDE_DIR}")
  target_link_libraries(rosbags_cpp_tests PRIVATE rosbags_cpp)
  target_compile_options(rosbags_cpp_tests PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME rosbags_cpp_project_tests COMMAND rosbags_cpp_tests)
endif()

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

message(STATUS "rosbags_cpp MCAP support: ${ROSBAGS_HAS_MCAP}")
message(STATUS "rosbags_cpp BZ2 support: ${ROSBAGS_HAS_BZ2}")
