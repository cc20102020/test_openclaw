# CTest driver for heic2tiff command-line behavior.
# Invoke with:
#   cmake -DH2T_EXE=/path/to/heic2tiff -DTEST_CASE=<case> -P tests/cli_tests.cmake

if(NOT DEFINED H2T_EXE)
  message(FATAL_ERROR "H2T_EXE is required")
endif()
if(NOT DEFINED TEST_CASE)
  message(FATAL_ERROR "TEST_CASE is required")
endif()

function(assert_equal name expected actual)
  if(NOT "${expected}" STREQUAL "${actual}")
    message(FATAL_ERROR "${name}: expected '${expected}', got '${actual}'")
  endif()
endfunction()

function(assert_contains name haystack needle)
  string(FIND "${haystack}" "${needle}" pos)
  if(pos EQUAL -1)
    message(FATAL_ERROR "${name}: expected output to contain '${needle}'\nActual output:\n${haystack}")
  endif()
endfunction()

function(assert_file_missing path)
  if(EXISTS "${path}")
    message(FATAL_ERROR "expected no output file at ${path}")
  endif()
endfunction()

function(assert_file_exists path)
  if(NOT EXISTS "${path}")
    message(FATAL_ERROR "expected output file at ${path}")
  endif()
  file(SIZE "${path}" size)
  if(size LESS 8)
    message(FATAL_ERROR "expected non-empty output file at ${path}")
  endif()
endfunction()

function(run_case expected_result expected_stdout expected_stderr)
  set(options)
  set(one_value_args OUTPUT_FILE)
  set(multi_value_args ARGS)
  cmake_parse_arguments(RUN "" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if(RUN_OUTPUT_FILE AND EXISTS "${RUN_OUTPUT_FILE}")
    file(REMOVE "${RUN_OUTPUT_FILE}")
  endif()

  execute_process(
    COMMAND "${H2T_EXE}" ${RUN_ARGS}
    RESULT_VARIABLE actual_result
    OUTPUT_VARIABLE actual_stdout
    ERROR_VARIABLE actual_stderr
  )

  assert_equal("${TEST_CASE} exit code" "${expected_result}" "${actual_result}")
  if(NOT "${expected_stdout}" STREQUAL "")
    assert_contains("${TEST_CASE} stdout" "${actual_stdout}" "${expected_stdout}")
  endif()
  if(NOT "${expected_stderr}" STREQUAL "")
    assert_contains("${TEST_CASE} stderr" "${actual_stderr}" "${expected_stderr}")
  endif()
  if(RUN_OUTPUT_FILE)
    assert_file_missing("${RUN_OUTPUT_FILE}")
  endif()
endfunction()

set(tmp_out "${CMAKE_CURRENT_BINARY_DIR}/${TEST_CASE}.tiff")

if(TEST_CASE STREQUAL "help")
  run_case(0 "Usage:" "" ARGS --help)
  run_case(0 "input.heic output.tiff" "" ARGS --help)
elseif(TEST_CASE STREQUAL "short_help")
  run_case(0 "Usage:" "" ARGS -h)
elseif(TEST_CASE STREQUAL "version")
  run_case(0 "heic2tiff 0.1.0" "" ARGS --version)
elseif(TEST_CASE STREQUAL "short_version")
  run_case(0 "heic2tiff 0.1.0" "" ARGS -V)
elseif(TEST_CASE STREQUAL "no_args")
  run_case(2 "" "Usage:")
elseif(TEST_CASE STREQUAL "missing_output_arg")
  run_case(2 "" "Usage:" ARGS input.heic)
elseif(TEST_CASE STREQUAL "too_many_args")
  run_case(2 "" "Usage:" ARGS input.heic output.tiff extra)
elseif(TEST_CASE STREQUAL "unknown_option")
  run_case(2 "" "Usage:" ARGS --bogus)
elseif(TEST_CASE STREQUAL "same_input_output")
  run_case(2 "" "invalid arguments" ARGS same.heic same.heic)
elseif(TEST_CASE STREQUAL "missing_input_file")
  run_case(3 "" "HEIC decode failed" ARGS "${CMAKE_CURRENT_BINARY_DIR}/does-not-exist.heic" "${tmp_out}" OUTPUT_FILE "${tmp_out}")
elseif(TEST_CASE STREQUAL "invalid_input_file")
  if(NOT DEFINED INVALID_FIXTURE)
    message(FATAL_ERROR "INVALID_FIXTURE is required for invalid_input_file")
  endif()
  run_case(3 "" "HEIC decode failed" ARGS "${INVALID_FIXTURE}" "${tmp_out}" OUTPUT_FILE "${tmp_out}")
elseif(TEST_CASE STREQUAL "valid_sample_conversion")
  if(NOT DEFINED SAMPLE_FIXTURE)
    message(FATAL_ERROR "SAMPLE_FIXTURE is required for valid_sample_conversion")
  endif()
  if(EXISTS "${tmp_out}")
    file(REMOVE "${tmp_out}")
  endif()
  execute_process(
    COMMAND "${H2T_EXE}" "${SAMPLE_FIXTURE}" "${tmp_out}"
    RESULT_VARIABLE actual_result
    OUTPUT_VARIABLE actual_stdout
    ERROR_VARIABLE actual_stderr
  )
  assert_equal("${TEST_CASE} exit code" "0" "${actual_result}")
  assert_file_exists("${tmp_out}")
  file(READ "${tmp_out}" tiff_magic OFFSET 0 LIMIT 4 HEX)
  if(NOT tiff_magic MATCHES "^(49492a00|4d4d002a)")
    message(FATAL_ERROR "${TEST_CASE}: output is not a TIFF file; magic=${tiff_magic}; stderr=${actual_stderr}")
  endif()
  file(REMOVE "${tmp_out}")
else()
  message(FATAL_ERROR "unknown TEST_CASE '${TEST_CASE}'")
endif()
