set(INTERROGATE_HEADERS
  functionRemap.h
  functionWriter.h
  functionWriterPtrFromPython.h functionWriterPtrToPython.h
  functionWriters.h
  interfaceMaker.h
  interfaceMakerC.h
  interfaceMakerPython.h interfaceMakerPythonObj.h
  interfaceMakerPythonSimple.h
  interfaceMakerPythonNative.h
  interrogate.h interrogateBuilder.h parameterRemap.I
  parameterRemap.h
  parameterRemapBasicStringPtrToString.h
  parameterRemapBasicStringRefToString.h
  parameterRemapBasicStringToString.h
  parameterRemapCharStarToString.h
  parameterRemapConcreteToPointer.h
  parameterRemapConstToNonConst.h parameterRemapEnumToInt.h
  parameterRemapPTToPointer.h
  parameterRemapReferenceToConcrete.h
  parameterRemapReferenceToPointer.h parameterRemapThis.h
  parameterRemapToString.h
  parameterRemapHandleToInt.h
  parameterRemapUnchanged.h
  typeManager.h
)

set(INTERROGATE_SOURCES
  functionRemap.cxx
  functionWriter.cxx
  functionWriterPtrFromPython.cxx functionWriterPtrToPython.cxx
  functionWriters.cxx
  interfaceMaker.cxx
  interfaceMakerC.cxx
  interfaceMakerPython.cxx interfaceMakerPythonObj.cxx
  interfaceMakerPythonSimple.cxx
  interfaceMakerPythonNative.cxx
  interrogate.cxx interrogateBuilder.cxx parameterRemap.cxx
  parameterRemapBasicStringPtrToString.cxx
  parameterRemapBasicStringRefToString.cxx
  parameterRemapBasicStringToString.cxx
  parameterRemapCharStarToString.cxx
  parameterRemapConcreteToPointer.cxx
  parameterRemapConstToNonConst.cxx
  parameterRemapEnumToInt.cxx parameterRemapPTToPointer.cxx
  parameterRemapReferenceToConcrete.cxx
  parameterRemapReferenceToPointer.cxx parameterRemapThis.cxx
  parameterRemapToString.cxx
  parameterRemapHandleToInt.cxx
  parameterRemapUnchanged.cxx
  typeManager.cxx
)

set(INTERROGATE_PREAMBLE_PYTHON_NATIVE_HEADERS
  ../interrogatedb/py_panda.h
  ../interrogatedb/py_panda.I
  ../interrogatedb/py_support.h
  ../interrogatedb/py_support.I
  ../interrogatedb/py_compat.h
  ../interrogatedb/py_wrappers.h
  ../interrogatedb/extension.h
)

set(INTERROGATE_PREAMBLE_PYTHON_NATIVE_SOURCES
  ../interrogatedb/py_panda.cxx
  ../interrogatedb/py_support.cxx
  ../interrogatedb/py_compat.cxx
  ../interrogatedb/py_wrappers.cxx
  ../interrogatedb/dtool_super_base.cxx
)

# Python preamble for interrogate_module
set(_preamble_sources)
set(_preamble_headers)
foreach(_file ${INTERROGATE_PREAMBLE_PYTHON_NATIVE_HEADERS})
  # Resolve symlinks, use absolute path
  get_filename_component(_file "${_file}" REALPATH)
  list(APPEND _preamble_headers "${_file}")
  list(APPEND _preamble_sources "${_file}")
endforeach(_file)
foreach(_file ${INTERROGATE_PREAMBLE_PYTHON_NATIVE_SOURCES})
  # Resolve symlinks, use absolute path
  get_filename_component(_file "${_file}" REALPATH)
  list(APPEND _preamble_sources "${_file}")
endforeach(_file)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/interrogate_preamble_python_native.h
  COMMAND ${CMAKE_COMMAND}
    -D OUTPUT_FILE="${CMAKE_CURRENT_BINARY_DIR}/interrogate_preamble_python_native.h"
    -D INPUT_FILES="${_preamble_headers}"
    -D PREPEND_LINE_DIRECTIVE=YES
    -D REPLACE_INCLUDES=YES
    -P ${PROJECT_SOURCE_DIR}/cmake/scripts/MergeIncludes.cmake
  DEPENDS ${_preamble_headers}
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/interrogate_preamble_python_native.cxx
  COMMAND ${CMAKE_COMMAND}
    -D OUTPUT_FILE="${CMAKE_CURRENT_BINARY_DIR}/interrogate_preamble_python_native.cxx"
    -D INPUT_FILES="${_preamble_sources}"
    -P ${PROJECT_SOURCE_DIR}/cmake/scripts/MergeIncludes.cmake
  DEPENDS ${_preamble_sources} ${_preamble_headers}
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/interrogate_preamble_python_native.h.cxx
  COMMAND ${CMAKE_COMMAND}
    -D OUTPUT_FILE="${CMAKE_CURRENT_BINARY_DIR}/interrogate_preamble_python_native.h.cxx"
    -D INPUT_FILES="${CMAKE_CURRENT_BINARY_DIR}/interrogate_preamble_python_native.h"
    -D SYMBOL_NAME="interrogate_preamble_python_native_h"
    -D PREPEND_LINE_DIRECTIVE=YES
    -D REPLACE_INCLUDES=YES
    -P ${PROJECT_SOURCE_DIR}/cmake/scripts/ConcatenateToCXX.cmake
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/interrogate_preamble_python_native.h"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/interrogate_preamble_python_native.cxx.cxx
  COMMAND ${CMAKE_COMMAND}
    -D OUTPUT_FILE="${CMAKE_CURRENT_BINARY_DIR}/interrogate_preamble_python_native.cxx.cxx"
    -D INPUT_FILES="${CMAKE_CURRENT_BINARY_DIR}/interrogate_preamble_python_native.cxx"
    -D SYMBOL_NAME="interrogate_preamble_python_native"
    -P ${PROJECT_SOURCE_DIR}/cmake/scripts/ConcatenateToCXX.cmake
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/interrogate_preamble_python_native.cxx"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

composite_sources(interrogate INTERROGATE_SOURCES)
add_executable(interrogate ${INTERROGATE_SOURCES}
  ${CMAKE_CURRENT_BINARY_DIR}/interrogate_preamble_python_native.h.cxx)
target_link_libraries(interrogate cppParser interrogatedb)

# parse_file utility
add_executable(parse_file parse_file.cxx)
target_link_libraries(parse_file cppParser)

add_executable(interrogate_module interrogate_module.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/interrogate_preamble_python_native.cxx.cxx)
target_link_libraries(interrogate_module cppParser interrogatedb)

if(NOT CMAKE_CROSSCOMPILING)
  add_executable(host_interrogate ALIAS interrogate)
  add_executable(host_interrogate_module ALIAS interrogate_module)

else()
  # When cross-compiling, we must find the host Interrogate.
  # TODO: We should also try to find_package(Panda3D ...) before falling back
  # to searching manually.
  find_program(HOST_PATH_INTERROGATE interrogate)
  find_program(HOST_PATH_INTERROGATE_MODULE interrogate_module)

  add_executable(host_interrogate IMPORTED GLOBAL)
  if(HOST_PATH_INTERROGATE)
    set_target_properties(host_interrogate PROPERTIES
      IMPORTED_LOCATION "${HOST_PATH_INTERROGATE}")
  endif()

  add_executable(host_interrogate_module IMPORTED GLOBAL)
  if(HOST_PATH_INTERROGATE_MODULE)
    set_target_properties(host_interrogate_module PROPERTIES
      IMPORTED_LOCATION "${HOST_PATH_INTERROGATE_MODULE}")
  endif()

endif()

if(SKBUILD)
  install(TARGETS interrogate COMPONENT tools DESTINATION interrogate)
  install(TARGETS interrogate_module COMPONENT tools DESTINATION interrogate_module)
else()
  install(TARGETS interrogate interrogate_module COMPONENT tools DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
