# Define project name
set(PROJECT_NAME MamontEngine)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Gather source files
file(GLOB src "src/*.h" "src/*.cpp" "src/*.hpp")
file(GLOB_RECURSE Core "src/Core/*.h" "src/Core/*.hpp" "src/Core/*.cpp")
file(GLOB_RECURSE Renderer "src/Renderer/*.h" "src/Graphics/*.hpp" "src/Graphics/*.cpp")
set(SOURCE_FILES ${src} ${Core} ${Renderer})

# Add library
add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor")

# Compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
  "$<$<CONFIG:Debug>:MM_DEBUG;_DEBUG>"
  "$<$<CONFIG:Release>:MM_RELEASE;NDEBUG>"
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
    target_compile_options(${PROJECT_NAME} PRIVATE /std:c++20 /permissive- /W4 /EHsc /MP /bigobj)
endif()

# CPM setup
if (NOT EXISTS ${CMAKE_SOURCE_DIR}/cpm.cmake)
  file(DOWNLOAD https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.38.7/CPM.cmake ${CMAKE_SOURCE_DIR}/cpm.cmake SHOW_PROGRESS)
endif()
include(${CMAKE_SOURCE_DIR}/cpm.cmake)

# Vulkan setup
if (VULKAN_SDK)
    set(ENV{VULKAN_SDK} ${VULKAN_SDK})
endif()
find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan)

# GLSL Compilation
find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)
file(GLOB_RECURSE GLSL_SOURCE_FILES "${PROJECT_SOURCE_DIR}/src/Shaders/*.frag" "${PROJECT_SOURCE_DIR}/src/Shaders/*.vert" "${PROJECT_SOURCE_DIR}/src/Shaders/*.comp")
foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_SOURCE_DIR}/src/Shaders/${FILE_NAME}.spv")
  add_custom_command(OUTPUT ${SPIRV} COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV} DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach()
add_custom_target(Shaders DEPENDS ${SPIRV_BINARY_FILES})

# Vendor dependencies
add_subdirectory("vendor/glm")
add_subdirectory("vendor/vkbootstrap")
add_subdirectory("vendor/SDL")
add_subdirectory("vendor/fmt" EXCLUDE_FROM_ALL)
add_subdirectory("vendor/vma" EXCLUDE_FROM_ALL)

target_link_libraries(${PROJECT_NAME} PUBLIC vk-bootstrap SDL3::SDL3 fmt::fmt GPUOpen::VulkanMemoryAllocator)

# imgui
add_library(imgui STATIC 
    vendor/imgui/imgui.cpp 
    vendor/imgui/imgui_demo.cpp 
    vendor/imgui/imgui_draw.cpp 
    vendor/imgui/imgui_widgets.cpp 
    vendor/imgui/imgui_tables.cpp 
    vendor/imgui/imgui_impl_vulkan.cpp 
    vendor/imgui/imgui_impl_sdl3.cpp "src/Core/FrameData.h")
target_include_directories(imgui PUBLIC vendor/imgui)
target_link_libraries(${PROJECT_NAME} PUBLIC imgui)
target_link_libraries(imgui PUBLIC Vulkan::Vulkan SDL3::SDL3)

#fast gltf
add_subdirectory("vendor/fastgltf")
target_link_libraries(${PROJECT_NAME} PUBLIC fastgltf::fastgltf)

#stb
add_subdirectory("vendor/stb")
target_link_libraries(${PROJECT_NAME} PUBLIC stb::image)


# PCH
set(PCH_FILE src/pch.h)
target_precompile_headers(${PROJECT_NAME} PUBLIC ${PCH_FILE})

add_definitions(-DPROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}")
message("PROJECT_ROOT_DIR" = ${CMAKE_SOURCE_DIR})