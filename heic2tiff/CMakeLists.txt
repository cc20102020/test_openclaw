cmake_minimum_required(VERSION 3.16)
project(heic2tiff VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(heic2tiff
  src/main.c
  src/cli.c
  src/error.c
  src/heic_decode.c
  src/pixel_convert.c
  src/tiff_write.c
  src/arch/pixel_convert_generic.c
  src/arch/pixel_convert_x86_64.c
  src/arch/pixel_convert_aarch64.c
)

target_include_directories(heic2tiff PRIVATE src)
option(H2T_STATIC_STANDALONE "Build a fully static standalone heic2tiff binary" OFF)
set(H2T_STATIC_PREFIX "" CACHE PATH "Prefix containing static libheif/libde265 dependencies")

if(H2T_STATIC_STANDALONE)
  if(NOT H2T_STATIC_PREFIX)
    message(FATAL_ERROR "H2T_STATIC_PREFIX is required when H2T_STATIC_STANDALONE=ON")
  endif()
  set(_H2T_ORIGINAL_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
  set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
  find_library(SHARPYUV_STATIC_LIBRARY sharpyuv REQUIRED)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ${_H2T_ORIGINAL_FIND_LIBRARY_SUFFIXES})
  set(H2T_HEIF_LIBS
    ${H2T_STATIC_PREFIX}/lib/libheif.a
    ${H2T_STATIC_PREFIX}/lib/libde265.a
    ${SHARPYUV_STATIC_LIBRARY}
    stdc++
    m
    pthread
  )
  target_link_options(heic2tiff PRIVATE -static)
else()
  find_library(HEIF_LIBRARY heif REQUIRED)
  set(H2T_HEIF_LIBS ${HEIF_LIBRARY})
endif()

target_link_libraries(heic2tiff PRIVATE ${H2T_HEIF_LIBS})
target_compile_options(heic2tiff PRIVATE -Wall -Wextra -Wpedantic)

add_executable(dimension_limits_test
  tests/dimension_limits_test.c
  src/heic_decode.c
)
target_include_directories(dimension_limits_test PRIVATE src)
target_link_libraries(dimension_limits_test PRIVATE ${H2T_HEIF_LIBS})
target_compile_options(dimension_limits_test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME dimension_limits COMMAND dimension_limits_test)

enable_testing()

set(HEIC2TIFF_CLI_TEST_DRIVER ${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_tests.cmake)
set(HEIC2TIFF_INVALID_FIXTURE ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/not_heic.txt)
set(HEIC2TIFF_SAMPLE_FIXTURE ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/example.heic)
set(HEIC2TIFF_SAMPLE1_FIXTURE ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sample1.heic)

function(add_heic2tiff_cli_test test_name)
  add_test(
    NAME ${test_name}
    COMMAND ${CMAKE_COMMAND}
      -DH2T_EXE=$<TARGET_FILE:heic2tiff>
      -DTEST_CASE=${test_name}
      -DINVALID_FIXTURE=${HEIC2TIFF_INVALID_FIXTURE}
      -DSAMPLE_FIXTURE=${HEIC2TIFF_SAMPLE_FIXTURE}
      -DSAMPLE1_FIXTURE=${HEIC2TIFF_SAMPLE1_FIXTURE}
      -P ${HEIC2TIFF_CLI_TEST_DRIVER}
  )
endfunction()

add_heic2tiff_cli_test(help)
add_heic2tiff_cli_test(short_help)
add_heic2tiff_cli_test(version)
add_heic2tiff_cli_test(short_version)
add_heic2tiff_cli_test(no_args)
add_heic2tiff_cli_test(missing_output_arg)
add_heic2tiff_cli_test(too_many_args)
add_heic2tiff_cli_test(unknown_option)
add_heic2tiff_cli_test(same_input_output)
add_heic2tiff_cli_test(missing_input_file)
add_heic2tiff_cli_test(invalid_input_file)
add_heic2tiff_cli_test(valid_sample_conversion)
add_heic2tiff_cli_test(sample1_regression_conversion)
