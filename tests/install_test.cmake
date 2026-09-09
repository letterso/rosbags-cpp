# The consumer must not accidentally use the producer's runtime or ROS setup.
foreach(variable LD_LIBRARY_PATH DYLD_LIBRARY_PATH CMAKE_PREFIX_PATH AMENT_PREFIX_PATH COLCON_PREFIX_PATH ROS_PACKAGE_PATH ROS_DISTRO)
  unset(ENV{${variable}})
endforeach()
function(run)
  execute_process(COMMAND ${ARGV} RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "Command failed (${result}): ${ARGV}\n${output}\n${error}")
  endif()
endfunction()
set(config_args)
set(ctest_config_args)
if(CONFIG)
  set(config_args --config "${CONFIG}")
  set(ctest_config_args -C "${CONFIG}")
endif()
set(work "${BUILD_DIR}/install-test")
file(REMOVE_RECURSE "${work}")
file(MAKE_DIRECTORY "${work}")
run("${CMAKE_COMMAND}" --install "${BUILD_DIR}" --prefix "${work}/original" ${config_args})
set(prefix "${work}/relocated prefix")
file(RENAME "${work}/original" "${prefix}")
file(COPY "${SOURCE_DIR}/tests/install/" DESTINATION "${work}/consumer")
run("${CMAKE_COMMAND}" -S "${work}/consumer" -B "${work}/build"
  "-DCMAKE_PREFIX_PATH=${prefix}" "-Dmcap_DIR=${SDK_CONFIG_DIR}" "-DROSBAGS_MCAP_ROOT=${SDK_PREFIX}"
  "-DWITH_GENERATOR=${WITH_GENERATOR}" -DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF
  -DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=OFF)
run("${CMAKE_COMMAND}" --build "${work}/build" ${config_args} -j2)
run("${CMAKE_CTEST_COMMAND}" --test-dir "${work}/build" ${ctest_config_args} --output-on-failure)
if(WITH_GENERATOR)
  # Native builds can explicitly choose a host tool too. Reconfigure with
  # existing dependency targets to cover parent-project integration.
  run("${CMAKE_COMMAND}" -S "${work}/consumer" -B "${work}/build"
    -DPREEXISTING_DEPENDENCIES=ON
    "-DHOST_GENERATOR=${prefix}/${BIN_DIR}/rosbags-gen${EXE_SUFFIX}")
  # Change a nested definition and require both regeneration and successful decode.
  run("${CMAKE_COMMAND}" -E sleep 1)
  file(APPEND "${work}/consumer/definitions/example/msg/Payload.msg" "uint32 extra\n")
  file(READ "${work}/consumer/main.cpp" main)
  file(WRITE "${work}/consumer/main.cpp" "#define ADDED_FIELD\n${main}")
  run("${CMAKE_COMMAND}" --build "${work}/build" ${config_args} -j2)
  run("${CMAKE_CTEST_COMMAND}" --test-dir "${work}/build" ${ctest_config_args} --output-on-failure)
endif()

if(WITH_GENERATOR)
  # A directory input must also notice a removed definition.
  run("${CMAKE_COMMAND}" -E sleep 1)
  file(WRITE "${work}/consumer/definitions/example/msg/Unused.msg" "uint8 value\n")
  run("${CMAKE_COMMAND}" --build "${work}/build" ${config_args} -j2)
  file(READ "${work}/build/custom_generated/application_messages.hpp" generated)
  if(NOT generated MATCHES "struct Unused")
    message(FATAL_ERROR "New definition was not generated")
  endif()
  run("${CMAKE_COMMAND}" -E sleep 1)
  file(REMOVE "${work}/consumer/definitions/example/msg/Unused.msg")
  run("${CMAKE_COMMAND}" --build "${work}/build" ${config_args} -j2)
  file(READ "${work}/build/custom_generated/application_messages.hpp" generated)
  if(generated MATCHES "struct Unused")
    message(FATAL_ERROR "Removed definition remains in generated headers")
  endif()
endif()

if(WITH_GENERATOR)
  execute_process(COMMAND "${CMAKE_COMMAND}" -S "${work}/consumer" -B "${work}/cross"
    -DCMAKE_SYSTEM_NAME=Linux "-DCMAKE_PREFIX_PATH=${prefix}"
    "-Dmcap_DIR=${SDK_CONFIG_DIR}" "-DROSBAGS_MCAP_ROOT=${SDK_PREFIX}" -DWITH_GENERATOR=ON
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
  if(result EQUAL 0 OR NOT error MATCHES "Cross compilation requires GENERATOR")
    message(FATAL_ERROR "Cross build did not reject a target-architecture generator: ${output} ${error}")
  endif()
  run("${CMAKE_COMMAND}" -S "${work}/consumer" -B "${work}/cross"
    "-DHOST_GENERATOR=${prefix}/${BIN_DIR}/rosbags-gen${EXE_SUFFIX}")
endif()

if(WITH_GENERATOR)
  execute_process(COMMAND "${CMAKE_COMMAND}" -S "${work}/consumer" -B "${work}/invalid-profile"
    "-DCMAKE_PREFIX_PATH=${prefix}" "-Dmcap_DIR=${SDK_CONFIG_DIR}"
    "-DROSBAGS_MCAP_ROOT=${SDK_PREFIX}" -DWITH_GENERATOR=ON -DMESSAGE_PROFILE=class
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
  if(result EQUAL 0 OR NOT error MATCHES "PROFILE must not be a C[+][+] keyword")
    message(FATAL_ERROR "Keyword profile was not rejected: ${output} ${error}")
  endif()
endif()
