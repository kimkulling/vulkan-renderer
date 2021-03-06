add_executable(inexor-vulkan-renderer-example main.cpp)

set_target_properties(
    inexor-vulkan-renderer-example PROPERTIES

    CXX_EXTENSIONS OFF
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

if(${CMAKE_GENERATOR} MATCHES "Visual Studio.*")
    target_compile_options(inexor-vulkan-renderer-example PRIVATE "/MP")
    set_target_properties(
        inexor-vulkan-renderer-example
        PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
endif()

# enable exceptions when using MSVC toolchain, makes Clang on windows possible
if(MSVC)
    target_compile_options(inexor-vulkan-renderer-example PRIVATE "-EHs")
endif()

target_link_libraries(
    inexor-vulkan-renderer-example

    PRIVATE
    inexor-vulkan-renderer
)

set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "inexor-vulkan-renderer-example")
