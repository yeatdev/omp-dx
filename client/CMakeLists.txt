cmake_minimum_required(VERSION 3.20)
project("omp-dx" LANGUAGES C CXX VERSION 0.0.1)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Threads REQUIRED)
option(OMP_DX_DEBUG_SPAM_TEST "Enable F10 client-side random DX event spam test." OFF)

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
if(OMP_DX_DEBUG_SPAM_TEST)
	target_compile_definitions(${PROJECT_NAME} PRIVATE OMP_DX_DEBUG_SPAM_TEST)
endif()
target_link_libraries(${PROJECT_NAME} PUBLIC d3d9 bcrypt minhook rakhook sampapi Threads::Threads)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${PROJECT_SOURCE_DIR}/omp-dx"
		"$<TARGET_FILE_DIR:${PROJECT_NAME}>/omp-dx"
)
