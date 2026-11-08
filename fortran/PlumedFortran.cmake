# Compile the installed wrapper sources with the consumer's Fortran compiler.
# Include Plumed.cmake.shared (or another linking variant) before calling:
#   plumed_add_fortran_bindings(bindings MODULES LEGACY F08)
# Link applications to bindings; its module include path propagates transitively.
# Keep merely including the historical link files usable by older CMake.
# The opt-in helper itself requires CMake 3.16 or newer.
if(COMMAND plumed_add_fortran_bindings)
  return()
endif()

function(plumed_add_fortran_bindings target)
  cmake_parse_arguments(PARSE_ARGV 1 bindings "" "" "MODULES")
  if(bindings_UNPARSED_ARGUMENTS OR bindings_KEYWORDS_MISSING_VALUES)
    message(FATAL_ERROR "plumed_add_fortran_bindings: expected MODULES LEGACY and/or F08")
  endif()
  if(NOT bindings_MODULES)
    set(bindings_MODULES F08)
  endif()
  get_property(languages GLOBAL PROPERTY ENABLED_LANGUAGES)
  if(NOT "Fortran" IN_LIST languages)
    message(FATAL_ERROR "plumed_add_fortran_bindings requires the Fortran language to be enabled")
  endif()
  set(sources)
  list(REMOVE_DUPLICATES bindings_MODULES)
  foreach(module IN LISTS bindings_MODULES)
    if(module STREQUAL "LEGACY")
      set(source "${PLUMED_FORTRAN}")
    elseif(module STREQUAL "F08")
      set(source "${PLUMED_FORTRAN08}")
    else()
      message(FATAL_ERROR "Unknown PLUMED Fortran module '${module}'; choose LEGACY or F08")
    endif()
    if(NOT EXISTS "${source}")
      message(FATAL_ERROR "PLUMED Fortran source '${source}' is unavailable; include a Plumed.cmake configuration first")
    endif()
    list(APPEND sources "${source}")
  endforeach()
  # The linking variants export PLUMED_LOAD. The full configuration exports
  # several alternatives; shared linking is its portable default here.
  if(PLUMED_LOAD)
    set(link_libraries ${PLUMED_LOAD})
  elseif(PLUMED_SHARED_LOAD)
    set(link_libraries ${PLUMED_SHARED_LOAD})
  elseif(PLUMED_STATIC_LOAD)
    # The Windows CMake installation exports a static kernel only.
    set(link_libraries ${PLUMED_STATIC_LOAD})
  else()
    message(FATAL_ERROR "No PLUMED link configuration; include Plumed.cmake or a linking variant first")
  endif()
  add_library(${target} STATIC ${sources})
  set(module_dir "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${target}.dir/modules")
  file(MAKE_DIRECTORY "${module_dir}")
  set_target_properties(${target} PROPERTIES
    Fortran_MODULE_DIRECTORY "${module_dir}"
    POSITION_INDEPENDENT_CODE ON)
  target_include_directories(${target} PUBLIC "${module_dir}")
  target_link_libraries(${target} PUBLIC ${link_libraries})
endfunction()
