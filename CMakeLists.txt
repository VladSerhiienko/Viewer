cmake_minimum_required(VERSION 3.4.1)

include(ExternalProject)
project (Viewer)

message(STATUS "CMAKE_SYSTEM_INFO_FILE = ${CMAKE_SYSTEM_INFO_FILE}")
message(STATUS "CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR = ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "CMAKE_SYSTEM = ${CMAKE_SYSTEM}")
message(STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR = ${CMAKE_BINARY_DIR}")

set(BUILD_FOLDER_SUFFIX "build_${CMAKE_SYSTEM_NAME}_${CMAKE_SYSTEM_PROCESSOR}_${CMAKE_CXX_COMPILER_ID}")
string(TOLOWER  ${BUILD_FOLDER_SUFFIX} BUILD_FOLDER_SUFFIX)
message(STATUS "BUILD_FOLDER_SUFFIX = ${BUILD_FOLDER_SUFFIX}")

if (WIN32)
    set(
        default_cmake_args
        -G
        "Visual Studio 15 2017 Win64"
        -B${BUILD_FOLDER_SUFFIX} "-H."
    )
elseif(APPLE)
    set(
        default_cmake_args
        -B${BUILD_FOLDER_SUFFIX} -H.
    )
else()
    set(
        default_cmake_args
        -B${BUILD_FOLDER_SUFFIX} -H.
    )
endif()

#
#
# argh
#
#

ExternalProject_Add(
    argh
    GIT_REPOSITORY "git@github.com:adishavit/argh.git"
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/argh"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(argh SOURCE_DIR)
set(argh_source_dir ${SOURCE_DIR})
message(STATUS "argh_source_dir = ${argh_source_dir}")

#
#
# spdlog
#
#

ExternalProject_Add(
    spdlog
    GIT_REPOSITORY "git@github.com:gabime/spdlog.git"
    GIT_TAG "master"
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/spdlog"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(spdlog SOURCE_DIR)
set(spdlog_source_dir ${SOURCE_DIR})
message(STATUS "spdlog_source_dir = ${spdlog_source_dir}")

#
#
# nuklear
#
#

ExternalProject_Add(
    nuklear
    GIT_REPOSITORY "git@github.com:vurtun/nuklear.git"
    GIT_TAG "master"
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/nuklear"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(nuklear SOURCE_DIR)
set(nuklear_source_dir ${SOURCE_DIR})
message(STATUS "nuklear_source_dir = ${nuklear_source_dir}")

#
#
# eastl
#
#

ExternalProject_Add(
    eastl
    GIT_REPOSITORY "git@github.com:electronicarts/EASTL.git"
    GIT_TAG "master"
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/eastl"
    CMAKE_ARGS ${default_cmake_args} -DEASTL_BUILD_TESTS:BOOL=TRUE
                                     -DEASTL_BUILD_BENCHMARK:BOOL=TRUE
    TEST_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(eastl SOURCE_DIR)
ExternalProject_Get_Property(eastl BINARY_DIR)
set(eastl_source_dir ${SOURCE_DIR})
set(eastl_binary_dir ${BINARY_DIR})
message(STATUS "eastl_source_dir = ${eastl_source_dir}")
message(STATUS "eastl_binary_dir = ${eastl_binary_dir}")

#
#
# FbxPipeline
#
#

ExternalProject_Add(
    FbxPipeline
    GIT_REPOSITORY "git@github.com:VladSerhiienko/FbxPipeline.git"
    GIT_TAG "master"
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/FbxPipeline"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(FbxPipeline SOURCE_DIR)
set(FbxPipeline_source_dir ${SOURCE_DIR})
message(STATUS "FbxPipeline_source_dir = ${FbxPipeline_source_dir}")

#
#
# gli
#
#

ExternalProject_Add(
    gli
    GIT_REPOSITORY "git@github.com:g-truc/gli.git"
    GIT_TAG "master"
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/gli"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(gli SOURCE_DIR)
set(gli_source_dir ${SOURCE_DIR})
message(STATUS "gli_source_dir = ${gli_source_dir}")

#
#
# DirectXMath
#
#

ExternalProject_Add(
    DirectXMath
    GIT_REPOSITORY "git@github.com:VladSerhiienko/DirectXMath.git"
    GIT_TAG "master"
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/DirectXMath"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(DirectXMath SOURCE_DIR)
set(DirectXMath_source_dir ${SOURCE_DIR})
message(STATUS "DirectXMath_source_dir = ${DirectXMath_source_dir}")

#
#
# flatbuffers
#
#

ExternalProject_Add(
    flatbuffers
    GIT_REPOSITORY "git@github.com:google/flatbuffers.git"
    GIT_TAG "v1.8.0"
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/flatbuffers"
    CMAKE_ARGS ${default_cmake_args}
    TEST_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(flatbuffers SOURCE_DIR)
ExternalProject_Get_Property(flatbuffers BINARY_DIR)
set(flatbuffers_source_dir ${SOURCE_DIR})
set(flatbuffers_binary_dir ${BINARY_DIR})
message(STATUS "flatbuffers_source_dir = ${flatbuffers_source_dir}")
message(STATUS "flatbuffers_binary_dir = ${flatbuffers_binary_dir}")

#
#
# sdl2
#
#

ExternalProject_Add(
    sdl2
    URL http://www.libsdl.org/release/SDL2-2.0.7.tar.gz
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/sdl2"
    CMAKE_ARGS ${default_cmake_args} -DCMAKE_DEBUG_POSTFIX:STRING=
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    TEST_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(sdl2 SOURCE_DIR)
ExternalProject_Get_Property(sdl2 BINARY_DIR)
set(sdl2_source_dir ${SOURCE_DIR})
set(sdl2_binary_dir ${BINARY_DIR})
message(STATUS "sdl2_source_dir = ${sdl2_source_dir}")
message(STATUS "sdl2_binary_dir = ${sdl2_binary_dir}")

#
#
# TaskScheduler
#
#

ExternalProject_Add(
    TaskScheduler
    GIT_REPOSITORY "git@github.com:VladSerhiienko/TaskScheduler.git"
    GIT_TAG "master"
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/TaskScheduler"
    CMAKE_ARGS ${default_cmake_args}
    TEST_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(TaskScheduler SOURCE_DIR)
ExternalProject_Get_Property(TaskScheduler BINARY_DIR)
set(TaskScheduler_source_dir ${SOURCE_DIR})
set(TaskScheduler_binary_dir ${BINARY_DIR})
message(STATUS "TaskScheduler_source_dir = ${TaskScheduler_source_dir}")
message(STATUS "TaskScheduler_binary_dir = ${TaskScheduler_binary_dir}")

#
#
# zlib
#
#

ExternalProject_Add(
    zlib
    GIT_REPOSITORY "git@github.com:madler/zlib.git"
    GIT_TAG "master"
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/zlib"
    CMAKE_ARGS ${default_cmake_args}
    TEST_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(zlib SOURCE_DIR)
ExternalProject_Get_Property(zlib BINARY_DIR)
set(zlib_source_dir ${SOURCE_DIR})
set(zlib_binary_dir ${BINARY_DIR})
message(STATUS "zlib_source_dir = ${zlib_source_dir}")
message(STATUS "zlib_binary_dir = ${zlib_binary_dir}")

#
#
# VulkanMemoryAllocator
#
#

ExternalProject_Add(
    VulkanMemoryAllocator
    GIT_REPOSITORY "git@github.com:GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git"
    GIT_TAG "master"
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/VulkanMemoryAllocator"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(VulkanMemoryAllocator SOURCE_DIR)
set(VulkanMemoryAllocator_source_dir ${SOURCE_DIR})
message(STATUS "VulkanMemoryAllocator_source_dir = ${VulkanMemoryAllocator_source_dir}")

#
#
# platform decisions
#
#

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG" )
set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG" )
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DMT_INSTRUMENTED_BUILD")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DMT_INSTRUMENTED_BUILD")

# add_definitions("-DAPEMODE_NO_GLOBAL_NEW_DELETE_OP_OVERRIDES")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

    # using Clang
    add_definitions("-g")
    add_definitions( "-DHAVE_PTHREAD" )
    set( CONFIGURATION_SUFFIX "" )

    if (UNIX AND NOT APPLE)
        set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DVK_USE_PLATFORM_XLIB_KHR" )
        set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DVK_USE_PLATFORM_XLIB_KHR" )
    elseif (WIN32)
        set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DVK_USE_PLATFORM_WIN32_KHR" )
        set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DVK_USE_PLATFORM_WIN32_KHR" )
    else()
        message( FATAL_ERROR "Unsupported configuration for ${CMAKE_CXX_COMPILER_ID} compiler." )
    endif()

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")

    # using AppleClang
    add_definitions( "-g" )
    add_definitions( "-DHAVE_PTHREAD" )
    set( CONFIGURATION_SUFFIX "" )

    if (UNIX AND APPLE)
        set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DVK_USE_PLATFORM_MACOS_MVK" )
        set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DVK_USE_PLATFORM_MACOS_MVK" )
    elseif( DEFINED IOS_PLATFORM )
        set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DVK_USE_PLATFORM_IOS_MVK" )
        set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DVK_USE_PLATFORM_IOS_MVK" )
    else()
        message( FATAL_ERROR "Unsupported configuration for ${CMAKE_CXX_COMPILER_ID} compiler." )
    endif()

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

    # using GCC
    add_definitions( "-g" )
    add_definitions( "-DHAVE_PTHREAD" )
    set( CONFIGURATION_SUFFIX "" )

    if (UNIX)
        set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DVK_USE_PLATFORM_XLIB_KHR" )
        set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DVK_USE_PLATFORM_XLIB_KHR" )
    else()
        message( FATAL_ERROR "Unsupported configuration for ${CMAKE_CXX_COMPILER_ID} compiler." )
    endif()

# elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

    # using Microsoft Visual C++
    set( CONFIGURATION_SUFFIX "/$(Configuration)" )
    set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_CRT_SECURE_NO_WARNINGS" )
    set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -D_CRT_SECURE_NO_WARNINGS" )

    if (WIN32)
        set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DVK_USE_PLATFORM_WIN32_KHR" )
        set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DVK_USE_PLATFORM_WIN32_KHR" )
    else()
        message( FATAL_ERROR "Unsupported configuration for ${CMAKE_CXX_COMPILER_ID} compiler." )
    endif()

else()
    message( FATAL_ERROR "Unsupported configuration for ${CMAKE_CXX_COMPILER_ID} compiler." )
endif()

#
#
# shared sources
#
#

set(
    shared_vk_source_files
    ${CMAKE_SOURCE_DIR}/src/apemode/vk/CityHash.Vulkan.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/vk/CityHash.Vulkan.h
    ${CMAKE_SOURCE_DIR}/src/apemode/vk/Image.Vulkan.h
    ${CMAKE_SOURCE_DIR}/src/apemode/vk/Buffer.Vulkan.h
    ${CMAKE_SOURCE_DIR}/src/apemode/vk/NativeHandles.Vulkan.h
    ${CMAKE_SOURCE_DIR}/src/apemode/vk/ResultHandle.Vulkan.h
    ${CMAKE_SOURCE_DIR}/src/apemode/vk/THandle.Vulkan.h
    ${CMAKE_SOURCE_DIR}/src/apemode/vk/TInfoStruct.Vulkan.h
    ${CMAKE_SOURCE_DIR}/src/apemode/vk/BufferPools.Vulkan.h
    ${CMAKE_SOURCE_DIR}/src/apemode/vk/BufferPools.Vulkan.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/vk/DescriptorPool.Vulkan.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/vk/DescriptorPool.Vulkan.h
    ${CMAKE_SOURCE_DIR}/src/apemode/vk/GraphicsDevice.Vulkan.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/vk/GraphicsDevice.Vulkan.h
    ${CMAKE_SOURCE_DIR}/src/apemode/vk/GraphicsManager.Vulkan.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/vk/GraphicsManager.Vulkan.h
    ${CMAKE_SOURCE_DIR}/src/apemode/vk/QueuePools.Vulkan.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/vk/QueuePools.Vulkan.h
    ${CMAKE_SOURCE_DIR}/src/apemode/vk/Swapchain.Vulkan.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/vk/Swapchain.Vulkan.h
)

set(
    shared_vk_ext_source_files
    ${CMAKE_SOURCE_DIR}/src/apemode/vk_ext/ImageUploader.Vulkan.h
    ${CMAKE_SOURCE_DIR}/src/apemode/vk_ext/ImageUploader.Vulkan.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/vk_ext/SamplerManager.Vulkan.h
    ${CMAKE_SOURCE_DIR}/src/apemode/vk_ext/SamplerManager.Vulkan.cpp
)

set(
    shared_sdl_source_files
    ${CMAKE_SOURCE_DIR}/src/apemode/sdl/AppMainSdl.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/sdl/AppSurfaceSdlBase.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/sdl/AppSurfaceSdlBase.h
    ${CMAKE_SOURCE_DIR}/src/apemode/sdl/InputSdl.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/sdl/Input.h
    ${CMAKE_SOURCE_DIR}/src/apemode/sdl/NuklearSdlBase.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/sdl/NuklearSdlBase.h
    ${CMAKE_SOURCE_DIR}/src/apemode/sdl/StopwatchSdl.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/sdl/Stopwatch.h
)

if (APPLE)
    set ( shared_sdl_source_files
          ${shared_sdl_source_files}
          ${CMAKE_SOURCE_DIR}/src/apemode/sdl/AppSurfaceSdlBaseCocoa.mm
          )
endif()

set(
    shared_platform_source_files
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/IAssetManager.h
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/IAssetManager.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/AssetManager.h
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/AssetManager.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/memory/MemoryManager.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/memory/MemoryManager.h
)

set(
    shared_source_files
    ${CMAKE_SOURCE_DIR}/src/apemode/CityHash.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/CityHash.h
    ${CMAKE_SOURCE_DIR}/src/apemode/AppBase.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/AppBase.h
    ${CMAKE_SOURCE_DIR}/src/apemode/AppState.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/AppState.h
    ${CMAKE_SOURCE_DIR}/src/apemode/AppSurfaceSdlVk.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/AppSurfaceSdlVk.h
)

set(
    viewer_source_files
    ${shared_platform_source_files}
    ${shared_sdl_source_files}
    ${shared_vk_source_files}
    ${shared_vk_ext_source_files}
    ${shared_source_files}
    ${CMAKE_SOURCE_DIR}/src/viewer/Scene.cpp
    ${CMAKE_SOURCE_DIR}/src/viewer/Scene.h
    ${CMAKE_SOURCE_DIR}/src/viewer/ViewerApp.cpp
    ${CMAKE_SOURCE_DIR}/src/viewer/ViewerApp.h
    ${CMAKE_SOURCE_DIR}/src/viewer/vk/SceneUploaderVk.cpp
    ${CMAKE_SOURCE_DIR}/src/viewer/vk/SceneUploaderVk.h
    ${CMAKE_SOURCE_DIR}/src/viewer/vk/SceneRendererVk.cpp
    ${CMAKE_SOURCE_DIR}/src/viewer/vk/SceneRendererVk.h
    ${CMAKE_SOURCE_DIR}/src/viewer/vk/SkyboxRendererVk.cpp
    ${CMAKE_SOURCE_DIR}/src/viewer/vk/SkyboxRendererVk.h
    ${CMAKE_SOURCE_DIR}/src/viewer/vk/DebugRendererVk.cpp
    ${CMAKE_SOURCE_DIR}/src/viewer/vk/DebugRendererVk.h
    ${CMAKE_SOURCE_DIR}/src/viewer/vk/NuklearRendererVk.cpp
    ${CMAKE_SOURCE_DIR}/src/viewer/vk/NuklearRendererVk.h
)


# vert|frag|comp|geom|tesc|tese|h|inl|inc|fx
file(
    GLOB
    Viewer_shader_source_files
    # Headers
    "${CMAKE_SOURCE_DIR}/assets/shaders/*.h"
    "${CMAKE_SOURCE_DIR}/assets/shaders/*.inc"
    "${CMAKE_SOURCE_DIR}/assets/shaders/*.inl"
    "${CMAKE_SOURCE_DIR}/assets/shaders/*.fx"
    # Sources
    "${CMAKE_SOURCE_DIR}/assets/shaders/*.vert"
    "${CMAKE_SOURCE_DIR}/assets/shaders/*.frag"
    "${CMAKE_SOURCE_DIR}/assets/shaders/*.tesc"
    "${CMAKE_SOURCE_DIR}/assets/shaders/*.tese"
    "${CMAKE_SOURCE_DIR}/assets/shaders/*.comp"
    "${CMAKE_SOURCE_DIR}/assets/shaders/*.geom"
)

add_executable(
    Viewer
    ${viewer_source_files}
    ${Viewer_shader_source_files}
)

source_group("Shader Sources" FILES ${Viewer_shader_source_files})
source_group("Shared Sources" FILES ${shared_source_files})
source_group("Shared Platform Sources" FILES ${shared_platform_source_files})
source_group("Shared SDL2 Sources" FILES ${shared_sdl_source_files})
source_group("Shared Vulkan Extention Sources" FILES ${shared_vk_ext_source_files})
source_group("Shared Vulkan Sources" FILES ${shared_vk_source_files})

add_dependencies(
    Viewer
    argh
    spdlog
    zlib
    flatbuffers
    nuklear
    DirectXMath
    FbxPipeline
    eastl
    gli
    sdl2
    VulkanMemoryAllocator
    TaskScheduler
)

message(STATUS "VK_SDK_PATH = $ENV{VK_SDK_PATH}")
message(STATUS "MOLTENVK_SDK_PATH = ${MOLTENVK_SDK_PATH}")

set(
    Viewer_include_dirs
    ${CMAKE_SOURCE_DIR}/assets/
    ${CMAKE_SOURCE_DIR}/assets/fonts/
    ${CMAKE_SOURCE_DIR}/src/apemode
    ${CMAKE_SOURCE_DIR}/src/apemode/platform
    ${CMAKE_SOURCE_DIR}/src/apemode/sdl
    ${CMAKE_SOURCE_DIR}/src/apemode/vk
    ${CMAKE_SOURCE_DIR}/src/apemode/vk_ext
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/memory
    ${CMAKE_SOURCE_DIR}/src/viewer
    ${CMAKE_SOURCE_DIR}/src/viewer/vk
    ${CMAKE_SOURCE_DIR}/dependencies/Math
    ${CMAKE_SOURCE_DIR}/dependencies/gli
    ${CMAKE_SOURCE_DIR}/dependencies/gli/external
    ${CMAKE_SOURCE_DIR}/dependencies/gli/external/glm
    ${CMAKE_SOURCE_DIR}/dependencies/nuklear
    ${CMAKE_SOURCE_DIR}/dependencies/VulkanMemoryAllocator/src
    ${CMAKE_SOURCE_DIR}/dependencies/DirectXMath
    ${CMAKE_SOURCE_DIR}/dependencies/DirectXMath/Inc
    ${CMAKE_SOURCE_DIR}/dependencies/argh
    ${CMAKE_SOURCE_DIR}/dependencies/spdlog/include
    ${CMAKE_SOURCE_DIR}/dependencies/flatbuffers/include
    ${CMAKE_SOURCE_DIR}/dependencies/sdl2/include
    ${CMAKE_SOURCE_DIR}/dependencies/eastl/include
    ${CMAKE_SOURCE_DIR}/dependencies/eastl/test/packages/EAAssert/include
    ${CMAKE_SOURCE_DIR}/dependencies/eastl/test/packages/EABase/include
    ${CMAKE_SOURCE_DIR}/dependencies/eastl/test/packages/EABase/include/Common
    ${CMAKE_SOURCE_DIR}/dependencies/eastl/test/packages/EAMain/include
    ${CMAKE_SOURCE_DIR}/dependencies/eastl/test/packages/EAStdC/include
    ${CMAKE_SOURCE_DIR}/dependencies/FbxPipeline/FbxPipeline/include/utils
    ${CMAKE_SOURCE_DIR}/dependencies/FbxPipeline/FbxPipeline/generated
    ${CMAKE_SOURCE_DIR}/dependencies/TaskScheduler/Scheduler/Include
)

if (APPLE)
    set(
        Viewer_include_dirs
        ${Viewer_include_dirs}
        ${MOLTENVK_SDK_PATH}/MoltenVK/include
    )

    set(
        Viewer_third_party_libs
        debug ${MOLTENVK_SDK_PATH}/Package/Debug/MoltenVK/macOS/libMoltenVK.dylib
        optimized ${MOLTENVK_SDK_PATH}/Package/Release/MoltenVK/macOS/libMoltenVK.dylib
        ${flatbuffers_binary_dir}/libflatbuffers.a
        ${eastl_binary_dir}/libEASTL.a
        ${eastl_binary_dir}/test/packages/EAStdC/libEAStdC.a
        ${sdl2_binary_dir}/libSDL2.a
        ${TaskScheduler_binary_dir}/libTaskScheduler.a
        pthread
        dl
        stdc++
        objc
        iconv
        "-framework Foundation"
        "-framework CoreFoundation"
        "-framework CoreAudio"
        "-framework CoreVideo"
        "-framework IOKit"
        "-framework Cocoa"
        "-framework AudioToolbox"
        "-framework ForceFeedback"
        "-framework Carbon"
        "-framework QuartzCore"
    )
elseif (UNIX)
    set(
        Viewer_include_dirs
        ${Viewer_include_dirs}
        $ENV{VK_SDK_PATH}/include
    )
    set(
        Viewer_third_party_libs
        $ENV{VK_SDK_PATH}/lib/libvulkan.so
        X11
        Xext
        Xcursor
        Xinerama
        Xrandr
        pthread
        stdc++fs
        ${sdl2_binary_dir}/libSDL2.a
        ${flatbuffers_binary_dir}/libflatbuffers.a
        ${eastl_binary_dir}/libEASTL.a
        ${eastl_binary_dir}/test/packages/EAStdC/libEAStdC.a
        ${TaskScheduler_binary_dir}/libTaskScheduler.a
        dl
    )
elseif(WIN32)
    set(
        Viewer_include_dirs
        ${Viewer_include_dirs}
        $ENV{VK_SDK_PATH}/include
    )
    set(
        Viewer_third_party_libs

        $ENV{VK_SDK_PATH}/lib/vulkan-1.lib
        opengl32.lib
        winmm.lib
        imm32.lib
        version.lib

        ${sdl2_binary_dir}${CONFIGURATION_SUFFIX}/SDL2.lib
        ${sdl2_binary_dir}${CONFIGURATION_SUFFIX}/SDL2main.lib
        ${eastl_binary_dir}${CONFIGURATION_SUFFIX}/EASTL.lib
        ${flatbuffers_binary_dir}${CONFIGURATION_SUFFIX}/flatbuffers.lib
        ${eastl_binary_dir}/test/packages/EAStdC${CONFIGURATION_SUFFIX}/EAStdC.lib
        ${TaskScheduler_binary_dir}${CONFIGURATION_SUFFIX}/TaskScheduler.lib
    )
endif()

target_include_directories(
    Viewer
    PUBLIC
    ${Viewer_include_dirs}
)

target_link_libraries(
    Viewer
    ${Viewer_third_party_libs}
)

if(WIN32)
    set( viewer_dependencies_dlls_pdbs
        ${sdl2_binary_dir}${CONFIGURATION_SUFFIX}/SDL2.dll
        # ${sdl2_binary_dir}${CONFIGURATION_SUFFIX}/SDL2.pdb
        )
elseif(UNIX AND NOT APPLE)
    set( viewer_dependencies_dlls_pdbs
        ${sdl2_binary_dir}${CONFIGURATION_SUFFIX}/libSDL2-2.0.so
        )
endif()

foreach( viewer_dependencies_dll_pdb ${viewer_dependencies_dlls_pdbs} )
    add_custom_command(
        TARGET
        Viewer
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${viewer_dependencies_dll_pdb}
        ${CMAKE_BINARY_DIR}${CONFIGURATION_SUFFIX}
    )
endforeach()

set_target_properties(
    Viewer
    PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY
    "$(OutDir)"
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(PREDEFINED_TARGETS_FOLDER "CustomTargets")

#
# Shaders
#

# Shader source files
# Source directory for the shaders
set(
    shader_directory
    ${CMAKE_SOURCE_DIR}/assets/shaders
)
set(
    shader_files
    ${shader_directory}/Debug.vert
    ${shader_directory}/Debug.frag
    ${shader_directory}/NuklearUI.vert
    ${shader_directory}/NuklearUI.frag
    ${shader_directory}/Skybox.vert
    ${shader_directory}/Skybox.frag
    ${shader_directory}/Scene.vert
    ${shader_directory}/Scene.frag
)

if (APPLE)
    set( glslangValidatorAbsolute "${MOLTENVK_SDK_PATH}/External/glslang/build/StandAlone/glslangValidator")
else()
    set( glslangValidatorAbsolute "$ENV{VK_SDK_PATH}/bin/glslangValidator")
endif()

message(STATUS "glslangValidator = ${glslangValidatorAbsolute}")

# Shaders
set( spirv_directory ${CMAKE_SOURCE_DIR}/assets/shaders/.spv )
add_custom_command( TARGET Viewer PRE_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "${spirv_directory}")
add_custom_command( TARGET Viewer PRE_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "${spirv_directory}/.preprocessed")
add_custom_command( TARGET Viewer PRE_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "${spirv_directory}/.reflection")

# Shader compilation routine
foreach(shader_file ${shader_files})

    get_filename_component( shader_file_name ${shader_file} NAME )

    string(LENGTH ${shader_file_name} shader_file_name_length)
    math(EXPR shader_file_name_ext_begin "${shader_file_name_length} - 4")
    string(SUBSTRING ${shader_file_name} ${shader_file_name_ext_begin} 4 shader_file_name_ext)

    set( shader_file_preprocessed "${spirv_directory}/.preprocessed/${shader_file_name}.txt" )
    set( shader_file_reflection "${spirv_directory}/.reflection/${shader_file_name}.txt" )
    set( shader_file_spv "${spirv_directory}/${shader_file_name}.spv" )

    message(STATUS "${shader_file} > ${shader_file_spv}")
    message(STATUS "${shader_file} + ${shader_file_preprocessed}")
    message(STATUS "${shader_file} + ${shader_file_reflection}")

    add_custom_command(
        TARGET Viewer PRE_BUILD
        COMMAND ${glslangValidatorAbsolute} -S ${shader_file_name_ext} -E ${shader_file} > ${shader_file_preprocessed}
        COMMAND ${glslangValidatorAbsolute} -S ${shader_file_name_ext} -V -q ${shader_file} -o ${shader_file_spv} > ${shader_file_reflection}
    )

endforeach()

if (APPLE)

    set( MoltenVKShaderConverterAbsolute "${MOLTENVK_SDK_PATH}/Package/Debug/MoltenVKShaderConverter/Tools/MoltenVKShaderConverter")

    # Output directory for the shaders
    set( msl_directory ${CMAKE_SOURCE_DIR}/assets/shaders/.msl )
    add_custom_command( TARGET Viewer PRE_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "${msl_directory}")
    add_custom_command( TARGET Viewer PRE_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "${msl_directory}/.logs")

    # Shader compilation routine
    foreach(shader_file ${shader_files})
        get_filename_component( shader_file_name ${shader_file} NAME )
        set( shader_file_spv "${spirv_directory}/${shader_file_name}.spv" )

        string(LENGTH ${shader_file_name} shader_file_name_length)
        math(EXPR shader_file_name_ext_begin "${shader_file_name_length} - 4")
        string(SUBSTRING ${shader_file_name} ${shader_file_name_ext_begin} 1 shader_file_name_ext)

        set( shader_file_msl "${msl_directory}/${shader_file_name}.metal" )
        message(STATUS "${shader_file} > ${shader_file_msl}")

        add_custom_command(
            TARGET Viewer PRE_BUILD
            COMMAND ${MoltenVKShaderConverterAbsolute} -t ${shader_file_name_ext} -si ${shader_file_spv} -mo ${shader_file_msl} -l > "${msl_directory}/.logs/${shader_file_name}.txt"
        )
    endforeach()

endif()
