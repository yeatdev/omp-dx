cmake_minimum_required(VERSION 3.20)
project("omp-dx" LANGUAGES C CXX VERSION 1.0.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(__BITSTREAM_NATIVE_END)

add_subdirectory(libs/rakhook)
add_subdirectory(libs/samp-api)

include_directories(libs/rakhook/lib/minhook/include)
include_directories(libs/rakhook/include)
include_directories(libs/samp-api/include)

add_library(${PROJECT_NAME} SHARED)
target_sources(${PROJECT_NAME} PRIVATE
	source/dllmain.cpp
    source/Plugin.cpp
)

set_target_properties(${PROJECT_NAME}
	PROPERTIES
	OUTPUT_NAME ${PROJECT_NAME}
	SUFFIX ".asi"
)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_link_libraries(${PROJECT_NAME} PUBLIC d3d9 minhook rakhook sampapi)