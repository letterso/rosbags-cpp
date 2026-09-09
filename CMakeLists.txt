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
set(ROSBAGS_MCAP_BUILDER_TAG "d6f3662b7204341797eaeca5ff97b8f659d02bda" CACHE STRING "MCAP CMake builder git tag, branch, or commit")

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

include(cmake/lz4Config.cmake.in)
include(cmake/zstdConfig.cmake.in)

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
    # mcap_builder discovers its optional compression dependencies via CMake
    # config packages, while Debian/Ubuntu's liblz4-dev and libzstd-dev expose
    # them through pkg-config.  Generate private compatibility packages that
    # adapt our already-validated PkgConfig targets to its required target
    # names.  They are only added to this configure's search path.
    set(ROSBAGS_MCAP_COMPAT_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/mcap_compat")
    file(MAKE_DIRECTORY
      "${ROSBAGS_MCAP_COMPAT_PREFIX}/lib/cmake/lz4"
      "${ROSBAGS_MCAP_COMPAT_PREFIX}/lib/cmake/zstd"
    )
    configure_file(
      cmake/lz4Config.cmake.in
      "${ROSBAGS_MCAP_COMPAT_PREFIX}/lib/cmake/lz4/lz4Config.cmake"
      @ONLY
    )
    configure_file(
      cmake/zstdConfig.cmake.in
      "${ROSBAGS_MCAP_COMPAT_PREFIX}/lib/cmake/zstd/zstdConfig.cmake"
      @ONLY
    )
    list(APPEND CMAKE_PREFIX_PATH "${ROSBAGS_MCAP_COMPAT_PREFIX}")

    include(FetchContent)
    FetchContent_Declare(
      mcap_builder
      GIT_REPOSITORY "${ROSBAGS_MCAP_BUILDER_REPOSITORY}"
      GIT_TAG "${ROSBAGS_MCAP_BUILDER_TAG}"
    )
    FetchContent_MakeAvailable(mcap_builder)
    if(TARGET mcap::mcap OR TARGET mcap)
      set(ROSBAGS_HAS_MCAP ON)
    else()
      message(FATAL_ERROR "MCAP was enabled, but mcap_builder did not provide an mcap target")
    endif()
  endif()
endif()

set(ROSBAGS_CPP_SOURCES
  src/rosbags.cpp
  src/rosbag1.cpp
  src/sqlite3.cpp
  src/codec.cpp
  src/codegen.cpp
  src/profiles.cpp
  src/mcap.cpp
)
add_library(rosbags_cpp ${ROSBAGS_CPP_SOURCES})
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
    add_library(rosbags_cpp::mcap_sdk UNKNOWN IMPORTED)
    set_target_properties(rosbags_cpp::mcap_sdk PROPERTIES
      IMPORTED_LOCATION "${ROSBAGS_MCAP_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${ROSBAGS_MCAP_INCLUDE_DIR}"
      INTERFACE_LINK_LIBRARIES "LZ4::lz4;zstd::libzstd")
    target_link_libraries(rosbags_cpp PRIVATE rosbags_cpp::mcap_sdk)
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

function(rosbags_enable_warnings target)
  target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
endfunction()
rosbags_enable_warnings(rosbags_cpp)
target_compile_features(rosbags_cpp PUBLIC cxx_std_17)
# mcap_builder installs its library into lib, independently of GNUInstallDirs.
file(RELATIVE_PATH ROSBAGS_MCAP_LIB_RELATIVE "/${CMAKE_INSTALL_LIBDIR}" "/lib")
if(APPLE)
  set_target_properties(rosbags_cpp PROPERTIES
    INSTALL_RPATH "@loader_path;@loader_path/${ROSBAGS_MCAP_LIB_RELATIVE}")
elseif(UNIX)
  set_target_properties(rosbags_cpp PROPERTIES
    INSTALL_RPATH "$ORIGIN;$ORIGIN/${ROSBAGS_MCAP_LIB_RELATIVE}")
endif()
include(cmake/rosbagsGenerateMessages.cmake)

if(ROSBAGS_BUILD_TOOLS)
  add_executable(rosbags-info tools/rosbags_info.cpp)
  target_link_libraries(rosbags-info PRIVATE rosbags_cpp)
  rosbags_enable_warnings(rosbags-info)
  add_executable(rosbags-read tools/rosbags_read.cpp)
  target_link_libraries(rosbags-read PRIVATE rosbags_cpp)
  rosbags_enable_warnings(rosbags-read)
  add_executable(rosbags-gen tools/rosbags_gen.cpp)
  add_executable(rosbags_cpp::rosbags-gen ALIAS rosbags-gen)
  target_link_libraries(rosbags-gen PRIVATE rosbags_cpp)
  rosbags_enable_warnings(rosbags-gen)
  file(RELATIVE_PATH ROSBAGS_BIN_TO_LIB "/${CMAKE_INSTALL_BINDIR}" "/${CMAKE_INSTALL_LIBDIR}")
  foreach(tool IN ITEMS rosbags-info rosbags-read rosbags-gen)
    if(APPLE)
      set_target_properties(${tool} PROPERTIES INSTALL_RPATH "@loader_path/${ROSBAGS_BIN_TO_LIB}")
    elseif(UNIX)
      set_target_properties(${tool} PROPERTIES INSTALL_RPATH "$ORIGIN/${ROSBAGS_BIN_TO_LIB}")
    endif()
    target_include_directories(${tool} SYSTEM PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/tools/third_party/cxxopts")
  endforeach()
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Cppcheck.cmake")

if(ROSBAGS_BUILD_TESTS)
  add_test(NAME rosbags_cpp_install_test
    COMMAND ${CMAKE_COMMAND}
      -DBUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}
      -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
      -DCONFIG=$<CONFIG>
      -DWITH_GENERATOR=${ROSBAGS_BUILD_TOOLS}
      "-DBIN_DIR=${CMAKE_INSTALL_BINDIR}"
      "-DEXE_SUFFIX=${CMAKE_EXECUTABLE_SUFFIX}"
      "-DSDK_PREFIX=${ROSBAGS_MCAP_ROOT}"
      "-DSDK_CONFIG_DIR=${mcap_DIR}"
      -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/install_test.cmake)
  set_tests_properties(rosbags_cpp_install_test PROPERTIES TIMEOUT 120)
  # doctest v2.5.3 is vendored as its official single header.  No doctest
  # library is linked; test_main.cpp provides the header-only implementation.
  set(ROSBAGS_DOCTEST_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests/third_party")
  function(rosbags_add_test target test_name)
    add_executable(${target} tests/test_main.cpp ${ARGN})
    target_include_directories(${target} PRIVATE "${ROSBAGS_DOCTEST_INCLUDE_DIR}")
    target_link_libraries(${target} PRIVATE rosbags_cpp)
    rosbags_enable_warnings(${target})
    add_test(NAME ${test_name} COMMAND ${target})
    set_tests_properties(${test_name} PROPERTIES TIMEOUT 60)
  endfunction()
  rosbags_add_test(rosbags_cpp_tests rosbags_cpp_project_tests tests/test_project.cpp)
  if(ROSBAGS_HAS_MCAP AND ROSBAGS_MCAP_EXTERNAL)
    target_link_libraries(rosbags_cpp_tests PRIVATE rosbags_cpp::mcap_sdk)
  endif()

  if(ROSBAGS_BUILD_TOOLS)
    rosbags_add_test(rosbags_cli_tests rosbags_cpp_cli_tests tests/test_cli.cpp)
    target_compile_definitions(rosbags_cli_tests PRIVATE
      ROSBAGS_INFO_PATH="$<TARGET_FILE:rosbags-info>"
      ROSBAGS_READ_PATH="$<TARGET_FILE:rosbags-read>"
      ROSBAGS_GEN_PATH="$<TARGET_FILE:rosbags-gen>"
      ROSBAGS_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/data")
    add_dependencies(rosbags_cli_tests rosbags-info rosbags-read rosbags-gen)
    set(ROSBAGS_CODEGEN_TEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated_test")
    set(ROSBAGS_CODEGEN_TEST_MESSAGES "${ROSBAGS_CODEGEN_TEST_DIR}/ros2_humble_messages.hpp")
    set(ROSBAGS_CODEGEN_TEST_REGISTRY "${ROSBAGS_CODEGEN_TEST_DIR}/ros2_humble_registry.hpp")
    add_custom_command(
      OUTPUT "${ROSBAGS_CODEGEN_TEST_MESSAGES}" "${ROSBAGS_CODEGEN_TEST_REGISTRY}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${ROSBAGS_CODEGEN_TEST_DIR}"
      COMMAND $<TARGET_FILE:rosbags-gen>
        --profile ros2_humble
        --input "${CMAKE_CURRENT_SOURCE_DIR}/tests/data/dependencies"
        --output "${ROSBAGS_CODEGEN_TEST_DIR}"
      DEPENDS rosbags-gen
        tests/data/dependencies/msg/AEnvelope.msg
        tests/data/dependencies/msg/ZPayload.msg
      VERBATIM
    )
    rosbags_add_test(rosbags_codegen_compile_test rosbags_cpp_codegen_compile_test
      tests/test_codegen_compile.cpp
      "${ROSBAGS_CODEGEN_TEST_MESSAGES}"
      "${ROSBAGS_CODEGEN_TEST_REGISTRY}"
    )
    target_include_directories(rosbags_codegen_compile_test PRIVATE "${ROSBAGS_CODEGEN_TEST_DIR}")
  endif()
endif()

install(TARGETS rosbags_cpp EXPORT rosbags_cppTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
if(ROSBAGS_BUILD_TOOLS)
  install(TARGETS rosbags-info rosbags-read rosbags-gen EXPORT rosbags_cppTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
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
  cmake/rosbagsGenerateMessages.cmake
  "${CMAKE_CURRENT_BINARY_DIR}/rosbags_cppConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/rosbags_cppConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/rosbags_cpp"
)

install(FILES cmake/lz4Config.cmake.in
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/rosbags_cpp" RENAME lz4Config.cmake)
install(FILES cmake/zstdConfig.cmake.in
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/rosbags_cpp" RENAME zstdConfig.cmake)

message(STATUS "rosbags_cpp MCAP support: ${ROSBAGS_HAS_MCAP}")
message(STATUS "rosbags_cpp BZ2 support: ${ROSBAGS_HAS_BZ2}")
