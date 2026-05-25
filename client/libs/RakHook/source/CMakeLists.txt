add_library(rakhook STATIC)
add_library(rakhook::rakhook ALIAS rakhook)

target_include_directories(rakhook PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(rakhook PUBLIC cxx_std_20)
target_link_libraries(rakhook PUBLIC minhook)

target_sources(rakhook PRIVATE
    "RakHook/offsets.cpp"
    "RakHook/rakhook.cpp"
    "RakHook/samp.cpp"

    "RakNet/BitStream.cpp"
    "RakNet/DS_HuffmanEncodingTree.cpp"
    "RakNet/PluginInterface.cpp"
    "RakNet/StringCompressor.cpp"
)