cmake_minimum_required(VERSION 3.19)
project(omp-dx LANGUAGES C CXX VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
find_package(Threads REQUIRED)

add_subdirectory(sdk)
add_subdirectory(network)

include_directories(
	.
	pawn/source
	pawn/source/linux
)

add_definitions(
	-DHAVE_STDINT_H=1
	-DPAWN_CELL_SIZE=32
)

add_library(${PROJECT_NAME} SHARED
	main.cpp
	natives.cpp
	dx-renderer.cpp
	dx-component.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    OMP-SDK
    OMP-Network
    Threads::Threads
)

