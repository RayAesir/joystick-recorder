cmake_minimum_required(VERSION 3.25)
project(joystick-recorder VERSION 1.0.0)

if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
	message(FATAL_ERROR "in-source builds are not allowed")
endif()

find_package(CLI11 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(cereal CONFIG REQUIRED)

# gather all output in the local bin folder
# generator expressions force Visual Studio to use specified path
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<0:>${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $<0:>${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY $<0:>${CMAKE_SOURCE_DIR}/bin)

include(cmake/sourcelist.cmake)
add_executable(${PROJECT_NAME} ${SOURCES})

include(cmake/flags.cmake)

target_include_directories(${PROJECT_NAME} PRIVATE
    /usr/include/libevdev-1.0
    /usr/include/libusb-1.0
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    /usr/lib/libevdev.so
    /usr/lib/libusb-1.0.so
    CLI11::CLI11
    spdlog::spdlog
    cereal::cereal
)
