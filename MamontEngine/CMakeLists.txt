# Define project name
set(PROJECT_NAME MamontEngine)


# Gather source files
file(GLOB src "src/*.h" "src/*.cpp" "src/*.hpp")
#file(GLOB src "src/*.ixx" "src/*.cxx")
source_group("src" FILES ${src})

file(GLOB_RECURSE Core "src/Core/*.h" "src/Core/*.hpp" "src/Core/*.cpp")
source_group("src/Core" FILES ${Core})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(SOURCE_FILES ${src} ${Core} ${PlatformsWin}  ${Events})
set(ALL_FILES ${SOURCE_FILES})


# Add library
add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES} "src/Core/VkInitializers.h" "src/Core/VkInitializers.cpp" "src/Core/VkImages.h" "src/Core/VkImages.cpp")

target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor")

# Compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
  "$<$<CONFIG:Debug>:MM_DEBUG;_DEBUG;>"
  "$<$<CONFIG:Release>:MM_RELEASE;NDEBUG;>"
  "$<$<CONFIG:Distribution>:MM_DISTRIBUTION;NDEBUG>"
  "_CRT_SECURE_NO_WARNINGS"
  "GLFW_INCLUDE_NONE"
  "_SILENCE_ALL_CXX20_DEPRECATION_WARNINGS"
  "GLM_FORCE_DEPTH_ZERO_TO_ONE"
  "GLM_ENABLE_EXPERIMENTAL"
)

# Compiler-specific flags
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "MM_COMPILER_CLANG")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "MM_COMPILER_GCC")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "MM_COMPILER_INTEL")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "MM_COMPILER_MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE /std:c++20 /permissive- /W3 /EHsc)
    target_compile_options(${PROJECT_NAME} PRIVATE /MP /bigobj)

endif()

# Check if CPM is available, otherwise download it
if (NOT EXISTS ${CMAKE_SOURCE_DIR}/cpm.cmake)
  file(DOWNLOAD https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.38.7/CPM.cmake ${CMAKE_SOURCE_DIR}/cpm.cmake SHOW_PROGRESS)
endif()

# Include CPM
include(${CMAKE_SOURCE_DIR}/cpm.cmake)
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1) # hack for removing auto generated ctest targets

# Vulkan
if (VULKAN_SDK)
    message("Setting VULKAN_SDK env var to " ${VULKAN_SDK})
    set(ENV{VULKAN_SDK} ${VULKAN_SDK})
endif()

find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})

# glm
add_subdirectory("vendor/glm")

#glf
#include_directories("vendor/glfw-3.4/include")
#target_link_directories(${PROJECT_NAME} PUBLIC "vendor/glfw-3.4")


add_subdirectory("vendor/vkbootstrap")
target_link_libraries(${PROJECT_NAME} PUBLIC vk-bootstrap)
set_target_properties(vk-bootstrap PROPERTIES FOLDER "vendor")
set_property(TARGET vk-bootstrap PROPERTY CXX_STANDARD 20)
add_subdirectory("vendor/SDL")
target_link_libraries(${PROJECT_NAME} PUBLIC SDL3::SDL3)

add_subdirectory("vendor/fmt" EXCLUDE_FROM_ALL)
target_link_libraries(${PROJECT_NAME} PUBLIC fmt::fmt)

#target_include_directories(${PROJECT_NAME} PUBLIC "vendor/sdl/include")
#sdl
#add_subdirectory("vendor/sdl" EXCLUDE_FROM_ALL)
#target_link_libraries(${PROJECT_NAME} PUBLIC SDL2::SDL2)

set(PCH_FILE src/pch.h)
target_precompile_headers(${PROJECT_NAME} PUBLIC ${PCH_FILE})
