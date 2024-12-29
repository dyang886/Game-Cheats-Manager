set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_library(Common STATIC ${COMMON_SOURCES})

# Include directories for headers
target_include_directories(Common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Add source files
file(GLOB COMMON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)
target_sources(Common PRIVATE ${COMMON_SOURCES})
