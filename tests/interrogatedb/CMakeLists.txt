function(idb_test file)
  get_filename_component(name ${file} NAME_WE)
  set(extra_args)
  if(file MATCHES "[.][ch](pp|xx)$")
    set(extra_args -D__cplusplus)
  endif()

  file(STRINGS "${file}" matched_lines REGEX "//\\s*FLAGS:\\s*(.+)")
  if(matched_lines)
    string(REGEX MATCH "//\\s*FLAGS:\\s*(.+)" _ "" "${matched_lines}")
    set(extra_args "${extra_args} ${CMAKE_MATCH_1}")
  endif()

  add_test(NAME ${name}
    COMMAND ${CMAKE_COMMAND}
            "-DOUTPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/${name}.gen.in"
            "-DEXPECTED_FILE=${CMAKE_CURRENT_SOURCE_DIR}/${name}.in"
            "-DCOMMAND=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/interrogate ${extra_args} -od \"${CMAKE_CURRENT_BINARY_DIR}/${name}.gen.in\" -oc \"${CMAKE_CURRENT_BINARY_DIR}/${name}.gen.cxx\" \"-S${PROJECT_SOURCE_DIR}/parser-inc\" \"${CMAKE_CURRENT_SOURCE_DIR}/${file}\""
            -P "${PROJECT_SOURCE_DIR}/cmake/scripts/CompareCommand.cmake"
  )
endfunction()

idb_test(item_assignment.h)
