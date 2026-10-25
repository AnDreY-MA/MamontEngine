# Define project name
set(PROJECT_NAME MamontEngine)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Gather source files
file(GLOB src "src/*.h" "src/*.cpp" "src/*.hpp")
file(GLOB_RECURSE Core "src/Core/*.h" "src/Core/*.hpp" "src/Core/*.cpp")
file(GLOB_RECURSE Renderer "src/Graphics/*.h" "src/Graphics/*.hpp" "src/Graphics/*.cpp")
file(GLOB_RECURSE Utils "src/Utils/*.h" "src/Utils/*.hpp" "src/Utils/*.cpp")
file(GLOB_RECURSE ECS "src/ECS/*.h" "src/ECS/*.hpp" "src/ECS/*.cpp")
file(GLOB_RECURSE UI "src/UI/*.h" "src/UI/*.hpp" "src/UI/*.cpp")
file(GLOB_RECURSE Physics "src/Physics/*.h" "src/Physics/*.hpp" "src/Physics/*.cpp")
file(GLOB_RECURSE Audio "src/Audio/*.h" "src/Audio/*.hpp" "src/Audio/*.cpp")
file(GLOB_RECURSE Math "src/Math/*.h" "src/Math/*.hpp" "src/Math/*.cpp")
file(GLOB_RECURSE Asset "src/Asset/*.h" "src/Asset/*.hpp" "src/Asset/*.cpp")
file(GLOB_RECURSE Scripting "src/Scripting/*.h" "src/Scripting/*.hpp" "src/Scripting/*.cpp")
set(SOURCE_FILES ${src} ${Core} ${Renderer} ${Devices} ${Materials} ${Utils} ${ECS} ${UI} ${Math} ${Asset} ${ECSComps} ${Physics} ${Scripting} ${Audio})

# Add library
add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor)

target_compile_definitions(${PROJECT_NAME} PUBLIC
  "$<$<CONFIG:Debug>:MM_DEBUG;_DEBUG>"
  "$<$<CONFIG:Release>:MM_RELEASE;NDEBUG>"
  "$<$<CONFIG:Distribution>:MM_DISTRIBUTION;NDEBUG>"
  "_CRT_SECURE_NO_WARNINGS"
  "_SILENCE_ALL_CXX20_DEPRECATION_WARNINGS"
  "GLM_FORCE_DEPTH_ZERO_TO_ONE"
  "GLM_ENABLE_EXPERIMENTAL"
  "$<$<CONFIG:Debug>:TRACY_ENABLE>"
  "$<$<CONFIG:Release>:TRACY_ENABLE>"
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_definitions(${PROJECT_NAME} PRIVATE "MM_COMPILER_CLANG")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_definitions(${PROJECT_NAME} PRIVATE "MM_COMPILER_GCC")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  target_compile_definitions(${PROJECT_NAME} PRIVATE "MM_COMPILER_INTEL")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_definitions(${PROJECT_NAME} PRIVATE "MM_COMPILER_MSVC")
  target_compile_options(${PROJECT_NAME} PRIVATE /std:c++23 /permissive- /W4 /EHsc /MP /bigobj)
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

#option(TRACY_ENABLE "" ON)

# Vendor dependencies
add_subdirectory("vendor/glm")
add_subdirectory("vendor/vkbootstrap")
add_subdirectory("vendor/sdl" EXCLUDE_FROM_ALL)
add_subdirectory("vendor/fmt" EXCLUDE_FROM_ALL)
add_subdirectory("vendor/vma" EXCLUDE_FROM_ALL)
add_subdirectory("vendor/entt" EXCLUDE_FROM_ALL)
add_subdirectory("vendor/tracy" EXCLUDE_FROM_ALL)
add_subdirectory("vendor/meshoptimizer" EXCLUDE_FROM_ALL)

include_directories(${PROJECT_NAME} meshoptimizer)

include_directories(${PROJECT_NAME} tracy/public)

target_include_directories(${PROJECT_NAME}
    SYSTEM PRIVATE
    "${STEAMAUDIO_INCLUDE_DIR}"
)

target_link_libraries(${PROJECT_NAME} PUBLIC vk-bootstrap SDL3::SDL3 fmt::fmt GPUOpen::VulkanMemoryAllocator EnTT::EnTT glm::glm Tracy::TracyClient meshoptimizer soloud)


#SoLud
set(SOLOUD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/vendor/soloud")

file(GLOB_RECURSE SOLOUD_SOURCES CONFIGURE_DEPENDS
    "${SOLOUD_ROOT}/src/core/*.cpp"
    "${SOLOUD_ROOT}/src/filter/*.cpp"
    "${SOLOUD_ROOT}/src/audiosource/wav/*.cpp"
    "${SOLOUD_ROOT}/src/audiosource/wav/*.c"
    "${SOLOUD_ROOT}/src/audiosource/monotone/*.cpp"
    "${SOLOUD_ROOT}/src/audiosource/speech/*.cpp"
    "${SOLOUD_ROOT}/src/audiosource/sfxr/*.cpp"
    "${SOLOUD_ROOT}/src/audiosource/vic/*.cpp"
    "${SOLOUD_ROOT}/src/backend/miniaudio/*.cpp"
)

add_library(soloud STATIC ${SOLOUD_SOURCES})

target_include_directories(soloud
    PUBLIC  "${SOLOUD_ROOT}/include"
    PRIVATE "${SOLOUD_ROOT}/src"
)

target_compile_definitions(soloud PUBLIC
    WITH_MINIAUDIO=1
)

# imgui
add_library(imgui STATIC
    vendor/imgui/imgui.h
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_demo.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/backends/imgui_impl_vulkan.cpp
    vendor/imgui/backends/imgui_impl_sdl3.cpp)
target_include_directories(imgui PUBLIC vendor/imgui)
target_link_libraries(imgui PUBLIC Vulkan::Vulkan SDL3::SDL3)
target_link_libraries(${PROJECT_NAME} PUBLIC imgui)

#ktx
add_subdirectory("vendor/ktx")
target_link_libraries(${PROJECT_NAME} PUBLIC ktx)

#fast gltf
add_subdirectory("vendor/fastgltf")
target_link_libraries(${PROJECT_NAME} PUBLIC fastgltf::fastgltf)

#lua
add_subdirectory("vendor/lua")
target_link_libraries(${PROJECT_NAME} PUBLIC lua)

#sol
target_include_directories(${PROJECT_NAME} PUBLIC "vendor/sol")
#target_link_libraries(${PROJECT_NAME} PUBLIC sol2::sol2)

#stb
add_subdirectory("vendor/stb")
target_link_libraries(${PROJECT_NAME} PUBLIC stb::image)

#cereal
if(NOT TARGET cereal)
  add_subdirectory("vendor/cereal")
  target_link_libraries(${PROJECT_NAME} PUBLIC cereal::cereal)
endif()

add_library(ImGuizmo STATIC
    vendor/ImGuizmo/ImGuizmo.cpp
    vendor/ImGuizmo/ImSequencer.cpp
    vendor/ImGuizmo/GraphEditor.cpp
    vendor/ImGuizmo/ImCurveEdit.cpp
    vendor/ImGuizmo/ImGradient.cpp)

target_include_directories(ImGuizmo PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/ImGuizmo
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui
)
target_compile_definitions(ImGuizmo PRIVATE
    IMGUI_DEFINE_MATH_OPERATORS
)

target_link_libraries(ImGuizmo PUBLIC imgui)

target_link_libraries(${PROJECT_NAME} PUBLIC ImGuizmo)

FetchContent_Declare(glslang
	GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
	GIT_TAG a8d39f97cdf08cf7eac4ae40dfe7b41420a3be30)
set(ENABLE_CTEST            OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
set(ENABLE_HLSL              ON CACHE BOOL "" FORCE)
set(SKIP_GLSLANG_INSTALL     ON CACHE BOOL "" FORCE)

#spirv
FetchContent_Declare(spirv-cross
	GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross.git
	GIT_TAG vulkan-sdk-1.4.321.0)
set(SPIRV_CROSS_CLI          OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_GLSL   ON CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_HLSL  OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_MSL   OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(spirv-headers
	GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers
	GIT_TAG vulkan-sdk-1.4.321.0)
set(SPIRV_HEADERS_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
set(SPIRV_HEADERS_SKIP_INSTALL  ON CACHE BOOL "" FORCE)

FetchContent_Declare(spirv-tools
	GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools
  GIT_TAG vulkan-sdk-1.4.321.0)
set(SKIP_SPIRV_TOOLS_INSTALL  ON CACHE BOOL "" FORCE)
set(SPIRV_BUILD_FUZZER       OFF CACHE BOOL "" FORCE)
set(SPIRV_SKIP_EXECUTABLES    ON CACHE BOOL "" FORCE)
set(SPIRV_SKIP_TESTS          ON CACHE BOOL "" FORCE)
set(SPIRV_WARN_EVERYTHING    OFF CACHE BOOL "" FORCE)
set(SPIRV_WERROR             OFF CACHE BOOL "" FORCE)

FetchContent_Declare(shaderc
	GIT_REPOSITORY https://github.com/google/shaderc.git
	GIT_TAG v2026.4)
set(SHADERC_ENABLE_WERROR_COMPILE OFF CACHE BOOL "" FORCE)
set(SHADERC_SKIP_COPYRIGHT_CHECK   ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_EXAMPLES          ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_INSTALL           ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_TESTS             ON CACHE BOOL "" FORCE)


FetchContent_MakeAvailable( spirv-headers spirv-tools spirv-cross glslang shaderc)

set_target_properties(glslc                         PROPERTIES EXCLUDE_FROM_ALL ON EXCLUDE_FROM_DEFAULT_BUILD ON)
set_target_properties(glslc_exe                     PROPERTIES EXCLUDE_FROM_ALL ON EXCLUDE_FROM_DEFAULT_BUILD ON)
#set_target_properties(shaderc_combined_genfile      PROPERTIES EXCLUDE_FROM_ALL ON EXCLUDE_FROM_DEFAULT_BUILD ON)
set_target_properties(shaderc_combined-pkg-config   PROPERTIES EXCLUDE_FROM_ALL ON EXCLUDE_FROM_DEFAULT_BUILD ON)
set_target_properties(shaderc_shared                PROPERTIES EXCLUDE_FROM_ALL ON EXCLUDE_FROM_DEFAULT_BUILD ON)
set_target_properties(shaderc_static-pkg-config     PROPERTIES EXCLUDE_FROM_ALL ON EXCLUDE_FROM_DEFAULT_BUILD ON)
set_target_properties(shaderc-pkg-config            PROPERTIES EXCLUDE_FROM_ALL ON EXCLUDE_FROM_DEFAULT_BUILD ON)
set_target_properties(spirv-cross-c                 PROPERTIES EXCLUDE_FROM_ALL ON EXCLUDE_FROM_DEFAULT_BUILD ON)
set_target_properties(spirv-cross-reflect           PROPERTIES EXCLUDE_FROM_ALL ON EXCLUDE_FROM_DEFAULT_BUILD ON)
set_target_properties(spirv-cross-util              PROPERTIES EXCLUDE_FROM_ALL ON EXCLUDE_FROM_DEFAULT_BUILD ON)
set_target_properties(SPIRV-Tools-shared            PROPERTIES EXCLUDE_FROM_ALL ON EXCLUDE_FROM_DEFAULT_BUILD ON)
set_target_properties(SPVRemapper                   PROPERTIES EXCLUDE_FROM_ALL ON EXCLUDE_FROM_DEFAULT_BUILD ON)

target_link_libraries(${PROJECT_NAME} PRIVATE shaderc spirv-cross-cpp)

# PCH
set(PCH_FILE src/pch.h)
target_precompile_headers(${PROJECT_NAME} PUBLIC ${PCH_FILE})

add_definitions(-DPROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}")