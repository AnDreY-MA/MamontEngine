set(PROJECT_NAME MamontEditor)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB src "src/*.h" "src/*.cpp" "src/*.hpp")
file(GLOB panels "src/Panels/*.h" "src/Panels/*.cpp" "src/Panels/*.hpp")
source_group("src" FILES ${src} ${panels})

set (EDITOR_SOURCE_FILES ${src} ${panels})

add_executable(${PROJECT_NAME} ${EDITOR_SOURCE_FILES} )

set_target_properties(${PROJECT_NAME} PROPERTIES
    TARGET_NAME_DEBUG "MamontEditor"
    TARGET_NAME_RELEASE "MamontEditor"
    TARGET_NAME_Distribution "MamontEditor"
    )

target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/../MamontEngine/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/../MamontEngine/vendor"
    "${CMAKE_CURRENT_SOURCE_DIR}/../MamontEngine/vendor/GLFW/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../MamontEngine/vendor/glm"
    )

target_compile_definitions(${PROJECT_NAME} PRIVATE
    "$<$<CONFIG:Debug>:"
            "MM_DEBUG;"
            "_DEBUG;"
        ">"
        "$<$<CONFIG:Release>:"
            "MM_RELEASE;"
            "NDEBUG;"
        ">"
        "$<$<CONFIG:Distribution>:"
            "MM_DISTRIBUTION;"
            "NDEBUG"
        ">"
    )

if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/MamontEditor")
    target_compile_options(${PROJECT_NAME} PRIVATE PRIVATE /std:c++20 /permissive-)
    target_link_options(${PROJECT_NAME} PRIVATE PRIVATE /DEBUG:FULL)
	set(CMAKE_SHARED_LINKER_FLAGS_DISTRIBUTION ${CMAKE_SHARED_LINKER_FLAGS})
    set(CMAKE_EXE_LINKER_FLAGS_DISTRIBUTION ${CMAKE_EXE_LINKER_FLAGS})
endif()

if (MSVC AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_definitions("/MP")
endif()

# Link with oxylus.
target_link_libraries(${PROJECT_NAME} PRIVATE
    MamontEngine
)
