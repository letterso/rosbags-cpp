include_guard(GLOBAL)

# Produces an INTERFACE target that carries generated headers and their build
# dependency. INPUTS must contain the complete closure of custom definitions.
function(rosbags_generate_messages target)
  cmake_parse_arguments(PARSE_ARGV 1 ARG "" "PROFILE;GENERATOR" "INPUTS")
  if(ARG_UNPARSED_ARGUMENTS OR ARG_KEYWORDS_MISSING_VALUES OR
      NOT ARG_PROFILE OR NOT ARG_INPUTS)
    message(FATAL_ERROR "rosbags_generate_messages requires PROFILE and INPUTS")
  endif()
  if(NOT ARG_PROFILE MATCHES "^[A-Za-z_][A-Za-z0-9_]*$")
    message(FATAL_ERROR "PROFILE must be a C++ identifier")
  endif()
  set(cpp_keywords
    alignas alignof and and_eq asm auto bitand bitor bool break case catch char
    char16_t char32_t class compl const constexpr const_cast continue decltype
    default delete do double dynamic_cast else enum explicit export extern false
    float for friend goto if inline int long mutable namespace new noexcept not
    not_eq nullptr operator or or_eq private protected public register
    reinterpret_cast return short signed sizeof static static_assert static_cast
    struct switch template this thread_local throw true try typedef typeid
    typename union unsigned using virtual void volatile wchar_t while xor xor_eq)
  if(ARG_PROFILE IN_LIST cpp_keywords)
    message(FATAL_ERROR "PROFILE must not be a C++ keyword")
  endif()
  if(ARG_GENERATOR)
    if(NOT IS_ABSOLUTE "${ARG_GENERATOR}" OR NOT EXISTS "${ARG_GENERATOR}" OR IS_DIRECTORY "${ARG_GENERATOR}")
      message(FATAL_ERROR "GENERATOR must name an existing absolute host executable")
    endif()
    set(generator "${ARG_GENERATOR}")
  elseif(CMAKE_CROSSCOMPILING)
    message(FATAL_ERROR "Cross compilation requires GENERATOR pointing to a host rosbags-gen")
  elseif(TARGET rosbags_cpp::rosbags-gen)
    set(generator rosbags_cpp::rosbags-gen)
  else()
    message(FATAL_ERROR "rosbags-gen is unavailable; build/install tools or provide GENERATOR")
  endif()

  set(inputs)
  set(definitions)
  foreach(input IN LISTS ARG_INPUTS)
    get_filename_component(input "${input}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    if(NOT EXISTS "${input}")
      message(FATAL_ERROR "Message input does not exist: ${input}")
    endif()
    list(APPEND inputs --input "${input}")
    if(IS_DIRECTORY "${input}")
      file(GLOB_RECURSE nested CONFIGURE_DEPENDS "${input}/*.msg" "${input}/*.idl")
      list(APPEND definitions ${nested})
    else()
      list(APPEND definitions "${input}")
    endif()
  endforeach()
  set(output "${CMAKE_CURRENT_BINARY_DIR}/${target}_generated")
  # Track additions and removals as well as edits of nested input definitions.
  string(JOIN "\n" manifest_content ${definitions})
  file(GENERATE OUTPUT "${output}/definitions.list" CONTENT "${manifest_content}\n")
  set(headers "${output}/${ARG_PROFILE}_messages.hpp" "${output}/${ARG_PROFILE}_registry.hpp")
  add_custom_command(OUTPUT ${headers}
    COMMAND ${generator} --profile "${ARG_PROFILE}" ${inputs} --output "${output}"
    DEPENDS ${generator} ${definitions} "${output}/definitions.list"
    VERBATIM)
  add_custom_target(${target}_generate DEPENDS ${headers})
  add_library(${target} INTERFACE)
  add_dependencies(${target} ${target}_generate)
  target_include_directories(${target} INTERFACE "${output}")
  target_link_libraries(${target} INTERFACE rosbags_cpp::rosbags_cpp)
endfunction()
