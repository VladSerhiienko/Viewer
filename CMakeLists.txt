cmake_minimum_required(VERSION 3.4.1)
project (viewer)

message(STATUS "CMAKE_SYSTEM_INFO_FILE = ${CMAKE_SYSTEM_INFO_FILE}")
message(STATUS "CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR = ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "CMAKE_SYSTEM = ${CMAKE_SYSTEM}")
message(STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR = ${CMAKE_BINARY_DIR}")

add_definitions("-g")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")

set(
    shared_vk_source_files
    ${CMAKE_SOURCE_DIR}/src/shared/vk/BufferPools.Vulkan.cpp
    ${CMAKE_SOURCE_DIR}/src/shared/vk/CommandQueue.Vulkan.cpp
    ${CMAKE_SOURCE_DIR}/src/shared/vk/DescriptorPool.Vulkan.cpp
    ${CMAKE_SOURCE_DIR}/src/shared/vk/GraphicsDevice.Vulkan.cpp
    ${CMAKE_SOURCE_DIR}/src/shared/vk/GraphicsManager.Vulkan.cpp
    ${CMAKE_SOURCE_DIR}/src/shared/vk/QueuePools.Vulkan.cpp
    ${CMAKE_SOURCE_DIR}/src/shared/vk/Swapchain.Vulkan.cpp
)

set(
    shared_vk_ext_source_files
    ${CMAKE_SOURCE_DIR}/src/shared/vk_ext/ShaderCompiler.Vulkan.cpp
    ${CMAKE_SOURCE_DIR}/src/shared/vk_ext/ImageLoader.Vulkan.cpp
    ${CMAKE_SOURCE_DIR}/src/shared/vk_ext/SamplerManager.Vulkan.cpp
)

set(
    shared_sdl_source_files
    ${CMAKE_SOURCE_DIR}/src/shared/sdl/AppMainSdl.cpp
    ${CMAKE_SOURCE_DIR}/src/shared/sdl/InputSdl.cpp
    ${CMAKE_SOURCE_DIR}/src/shared/sdl/StopwatchSdl.cpp
    ${CMAKE_SOURCE_DIR}/src/shared/sdl/AppSurfaceSdlBase.cpp
    ${CMAKE_SOURCE_DIR}/src/shared/sdl/NuklearSdlBase.cpp
)

set(
    shared_platform_source_files
    # ${CMAKE_SOURCE_DIR}/src/shared/platform/FbxViewerOS.cpp
    ${CMAKE_SOURCE_DIR}/src/shared/platform/FileTracker.cpp
)

set(
    shared_source_files
    # ${CMAKE_SOURCE_DIR}/src/shared/App.cpp     
    ${CMAKE_SOURCE_DIR}/src/shared/CityHash.cpp      
    ${CMAKE_SOURCE_DIR}/src/shared/AppBase.cpp 
    ${CMAKE_SOURCE_DIR}/src/shared/DebugRendererVk.cpp 
    ${CMAKE_SOURCE_DIR}/src/shared/AppState.cpp 
    ${CMAKE_SOURCE_DIR}/src/shared/AppSurfaceSdlVk.cpp  
    ${CMAKE_SOURCE_DIR}/src/shared/NuklearSdlVk.cpp
)

set(
    viewer_source_files
    ${shared_platform_source_files}
    ${shared_sdl_source_files}
    ${shared_vk_source_files}
    ${shared_vk_ext_source_files}
    ${shared_source_files}
    ${CMAKE_SOURCE_DIR}/src/viewer/ViewerApp.cpp
    ${CMAKE_SOURCE_DIR}/src/viewer/SceneRendererVk.cpp
    ${CMAKE_SOURCE_DIR}/src/viewer/SkyboxRendererVk.cpp
)

add_library(
    lodepng
    STATIC
    ${CMAKE_SOURCE_DIR}/third_party/lodepng/lodepng.cpp
)

target_include_directories(
    lodepng
    PUBLIC
    ${CMAKE_SOURCE_DIR}/third_party/lodepng
)
    
add_executable(
    viewer
    ${viewer_source_files}
)

message(STATUS "VK_SDK_PATH = $ENV{VK_SDK_PATH}")

target_include_directories(
    viewer
    PUBLIC
    ${CMAKE_SOURCE_DIR}/assets/
    ${CMAKE_SOURCE_DIR}/assets/fonts/
    ${CMAKE_SOURCE_DIR}/src/shared
    ${CMAKE_SOURCE_DIR}/src/shared/platform
    ${CMAKE_SOURCE_DIR}/src/shared/sdl
    ${CMAKE_SOURCE_DIR}/src/shared/vk
    ${CMAKE_SOURCE_DIR}/src/shared/vk_ext
    ${CMAKE_SOURCE_DIR}/src/viewer
    ${CMAKE_SOURCE_DIR}/third_party/gli
    ${CMAKE_SOURCE_DIR}/third_party/gli/external
    ${CMAKE_SOURCE_DIR}/third_party/gli/external/glm
    ${CMAKE_SOURCE_DIR}/third_party/lodepng
    ${CMAKE_SOURCE_DIR}/third_party/nuklear
    ${CMAKE_SOURCE_DIR}/third_party/DirectXMath
    ${CMAKE_SOURCE_DIR}/third_party/DirectXMath/Inc
    ${CMAKE_SOURCE_DIR}/third_party/cxxopts/include
    ${CMAKE_SOURCE_DIR}/third_party/spdlog/include
    ${CMAKE_SOURCE_DIR}/third_party/flatbuffers/include
    ${CMAKE_SOURCE_DIR}/third_party/SDL2-2.0.7/include
    ${CMAKE_SOURCE_DIR}/third_party/EASTL/include
    ${CMAKE_SOURCE_DIR}/third_party/FbxPipeline/FbxPipeline/generated
    $ENV{VK_SDK_PATH}/include
    $ENV{VK_SDK_PATH}/shaderc/libshaderc/include
    $ENV{VK_SDK_PATH}/../source/shaderc/src/libshaderc/include
)

if (UNIX)
    set(
        third_party_libs
        $ENV{VK_SDK_PATH}/lib/libvulkan.so
        X11
        Xext
        Xcursor
        Xinerama
        Xrandr
        pthread
        stdc++fs
        ${CMAKE_SOURCE_DIR}/third_party/SDL2-2.0.7/build_linux_x86-64/libSDL2.a
        $ENV{VK_SDK_PATH}/../source/shaderc/src/build_linux_x86-64/libshaderc/libshaderc_combined.a
        ${CMAKE_SOURCE_DIR}/third_party/flatbuffers/build_linux_x86-64/libflatbuffers.a
        ${CMAKE_SOURCE_DIR}/third_party/EASTL/build_linux_x86-64/libEASTL.a
        ${CMAKE_SOURCE_DIR}/third_party/EASTL/build_linux_x86-64/test/packages/EAStdC/libEAStdC.a
        dl
    )
elseif(WIN32)
    set(
        third_party_libs

        $ENV{VK_SDK_PATH}/lib/vulkan-1.lib
        opengl32.lib
        winmm.lib
        imm32.lib
        version.lib
        
        debug ${CMAKE_SOURCE_DIR}/third_party/SDL2-2.0.7/build_windows_x86-64/$(Configuration)/SDL2-staticd.lib
        debug $ENV{VK_SDK_PATH}/shaderc/build_windows_x86-64/libshaderc/$(Configuration)/shaderc_combined.lib
        debug ${CMAKE_SOURCE_DIR}/third_party/flatbuffers/build_windows_x86-64/$(Configuration)/flatbuffers.lib
        debug ${CMAKE_SOURCE_DIR}/third_party/EASTL/build_windows_x86-64/$(Configuration)/EASTL.lib
        debug ${CMAKE_SOURCE_DIR}/third_party/EASTL/build_windows_x86-64/test/packages/EAStdC/$(Configuration)/EAStdC.lib
        
        optimized ${CMAKE_SOURCE_DIR}/third_party/SDL2-2.0.7/build_windows_x86-64/$(Configuration)/SDL2-static.lib
        optimized $ENV{VK_SDK_PATH}/shaderc/build_windows_x86-64/libshaderc/$(Configuration)/shaderc_combined.lib
        optimized ${CMAKE_SOURCE_DIR}/third_party/flatbuffers/build_windows_x86-64/$(Configuration)/flatbuffers.lib
        optimized ${CMAKE_SOURCE_DIR}/third_party/EASTL/build_windows_x86-64/$(Configuration)/EASTL.lib
        optimized ${CMAKE_SOURCE_DIR}/third_party/EASTL/build_windows_x86-64/test/packages/EAStdC/$(Configuration)/EAStdC.lib
    )
endif()

target_link_libraries(
    viewer
    lodepng
    ${third_party_libs}
)