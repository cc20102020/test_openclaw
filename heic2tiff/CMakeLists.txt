cmake_minimum_required(VERSION 3.16)
project(heic2tiff VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBHEIF REQUIRED IMPORTED_TARGET libheif)
pkg_check_modules(LIBTIFF REQUIRED IMPORTED_TARGET libtiff-4)

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
target_link_libraries(heic2tiff PRIVATE PkgConfig::LIBHEIF PkgConfig::LIBTIFF)
target_compile_options(heic2tiff PRIVATE -Wall -Wextra -Wpedantic)

enable_testing()

set(HEIC2TIFF_CLI_TEST_DRIVER ${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_tests.cmake)
set(HEIC2TIFF_INVALID_FIXTURE ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/not_heic.txt)

function(add_heic2tiff_cli_test test_name)
  add_test(
    NAME ${test_name}
    COMMAND ${CMAKE_COMMAND}
      -DH2T_EXE=$<TARGET_FILE:heic2tiff>
      -DTEST_CASE=${test_name}
      -DINVALID_FIXTURE=${HEIC2TIFF_INVALID_FIXTURE}
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
