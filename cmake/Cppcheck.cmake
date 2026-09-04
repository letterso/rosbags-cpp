option(ROSBAGS_ENABLE_CPPCHECK "Enable cppcheck static analysis for rosbags-cpp implementation" OFF)

if(ROSBAGS_ENABLE_CPPCHECK)
  set(ROSBAGS_CPPCHECK_EXECUTABLE "/usr/local/bin/cppcheck")
  if(NOT EXISTS "${ROSBAGS_CPPCHECK_EXECUTABLE}")
    message(FATAL_ERROR "ROSBAGS_ENABLE_CPPCHECK requires /usr/local/bin/cppcheck")
  endif()

  if(ROSBAGS_HAS_BZ2)
    set(ROSBAGS_CPPCHECK_HAS_BZ2 1)
  else()
    set(ROSBAGS_CPPCHECK_HAS_BZ2 0)
  endif()
  if(ROSBAGS_HAS_MCAP)
    set(ROSBAGS_CPPCHECK_HAS_MCAP 1)
  else()
    set(ROSBAGS_CPPCHECK_HAS_MCAP 0)
  endif()

  set(ROSBAGS_CPPCHECK_SOURCES
    ${ROSBAGS_CPP_SOURCES}
    tools/rosbags_info.cpp
    tools/rosbags_read.cpp
    tools/rosbags_gen.cpp
  )

  # Pass an explicit production-source allowlist.  In particular, do not scan
  # vendored doctest, FetchContent dependencies, tests, or the Python subtree.
  add_custom_target(rosbags_cpp_cppcheck
    COMMAND "${ROSBAGS_CPPCHECK_EXECUTABLE}"
      --enable=warning,performance,portability
      --error-exitcode=2
      --inline-suppr
      --std=c++17
      --quiet
      --suppress=missingIncludeSystem
      "-DROSBAGS_HAS_BZ2=${ROSBAGS_CPPCHECK_HAS_BZ2}"
      "-DROSBAGS_HAS_MCAP=${ROSBAGS_CPPCHECK_HAS_MCAP}"
      -I "${CMAKE_CURRENT_SOURCE_DIR}/include"
      ${ROSBAGS_CPPCHECK_SOURCES}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Run cppcheck on rosbags-cpp production implementation"
    VERBATIM
  )
endif()
