cmake_minimum_required(VERSION 3.15)

#set(PROJECT_N Mamont)
PROJECT(Mamont VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_VS_PLATFORM_NAME "x64")


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

#file(GLOB_RECURSE SRCS ${PROJECT_SOURCE_DIR}/src/*.cpp)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# a macro that gets all of the header containing directories. 
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

# using said macro.
header_directories(INCLUDES ${PROJECT_SOURCE_DIR}/include/ hpp)

message("src files:")
foreach(file ${SRCS})
        message(STATUS ${file})
endforeach()

message("include directories:")
foreach(dir ${INCLUDES})
        message(STATUS ${dir})
endforeach()

#add_executable(${PROJECT_N} ${SRCS})
add_subdirectory("MamontEngine")
add_subdirectory("MamontEditor")

#target_include_directories(${PROJECT_N} PUBLIC include)
