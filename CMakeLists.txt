cmake_minimum_required(VERSION 4.2)

PROJECT(Mamont VERSION 0.0.1 LANGUAGES C CXX)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)


include("cmake/ProjectSettings.cmake")

set(CMAKE_CONFIGURATION_TYPES
    "Debug"
    "Release"
    "Distribution"
    CACHE STRING "" FORCE
)

SET(Boost_NO_WARN_NEW_VERSIONS ON)
IF (WIN32)
  ADD_DEFINITIONS(-DBOOST_USE_WINAPI_VERSION=0x0A00)
  ADD_DEFINITIONS(-D_WIN32_WINNT=0x0A00)
ENDIF (WIN32)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/bin"
)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/lib"
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/lib"
)

MACRO(header_directories return_list includes_base_folder extention )
  FILE(GLOB_RECURSE new_list ${includes_base_folder}/*.${extention})
  SET(dir_list "")
  FOREACH(file_path ${new_list})
    GET_FILENAME_COMPONENT(dir_path ${file_path} PATH)
    SET(dir_list ${dir_list} ${dir_path})
  ENDFOREACH()
  LIST(REMOVE_DUPLICATES dir_list)
  SET(${return_list} ${dir_list})
ENDMACRO()

header_directories(INCLUDES ${PROJECT_SOURCE_DIR}/include/ hpp)

message("src files:")
foreach(file ${SRCS})
  message(STATUS ${file})
endforeach()

add_subdirectory("MamontEngine")
add_subdirectory("MamontEditor")