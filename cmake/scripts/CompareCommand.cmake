cmake_policy(PUSH)
cmake_policy(SET CMP0007 NEW)
cmake_policy(SET CMP0121 NEW)

if(NOT DEFINED OUTPUT_FILE OR NOT DEFINED EXPECTED_FILE OR NOT DEFINED COMMAND)
  message(FATAL_ERROR "Usage: cmake -DOUTPUT_FILE=... -DEXPECTED_FILE=... -DCOMMAND=\"cmd args\" -P CompareCommand.cmake")
endif()

# Run the command, capturing output
separate_arguments(COMMAND_LIST UNIX_COMMAND "${COMMAND}")
set(ENV{SOURCE_DATE_EPOCH} 0)
execute_process(COMMAND ${COMMAND_LIST}
  RESULT_VARIABLE run_result
  OUTPUT_FILE "${OUTPUT_FILE}"
)
if(NOT run_result EQUAL 0)
  message(FATAL_ERROR "Command failed (${run_result}): ${COMMAND}")
endif()

# Compare files
execute_process(
  COMMAND ${CMAKE_COMMAND} -E compare_files "${OUTPUT_FILE}" "${EXPECTED_FILE}"
  RESULT_VARIABLE cmp_result
)

if(NOT cmp_result EQUAL 0)
  if(WIN32)
    message(WARNING "Files differ (no diff available on Windows).")
  else()
    # Show unified diff on Unix-like systems
    execute_process(
      COMMAND diff -u "${EXPECTED_FILE}" "${OUTPUT_FILE}"
      RESULT_VARIABLE diff_result
      OUTPUT_VARIABLE diff_out
      ERROR_VARIABLE diff_err
    )
    message("${diff_out}${diff_err}")
  endif()
  message(FATAL_ERROR "Test failed: ${OUTPUT_FILE} differs from ${EXPECTED_FILE}")
endif()

cmake_policy(POP)
