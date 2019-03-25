cmake_minimum_required(VERSION 3.4.1)

include(ExternalProject)
project (ViewerSdl)

#
#
# platform decisions
#
#

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG" )
set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG" )

message(STATUS "CMAKE_SYSTEM_INFO_FILE = ${CMAKE_SYSTEM_INFO_FILE}")
message(STATUS "CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR = ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "CMAKE_SYSTEM = ${CMAKE_SYSTEM}")
message(STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR = ${CMAKE_BINARY_DIR}")

if (NOT BUILD_FOLDER_SUFFIX)
    set(BUILD_FOLDER_SUFFIX "build_${CMAKE_CXX_COMPILER_ID}_${CMAKE_GENERATOR}")
    string(TOLOWER  ${BUILD_FOLDER_SUFFIX} BUILD_FOLDER_SUFFIX)
    string(REPLACE " " "_" BUILD_FOLDER_SUFFIX ${BUILD_FOLDER_SUFFIX} )
endif()
message(STATUS "BUILD_FOLDER_SUFFIX = ${BUILD_FOLDER_SUFFIX}")

if (WIN32)
    set(
        default_cmake_args
        -G "Visual Studio 15 2017 Win64"
        -B${BUILD_FOLDER_SUFFIX} "-H."
    )
elseif(APPLE)
    set(
        default_cmake_args
        -G "Xcode"
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
# json
#
#

ExternalProject_Add(
    json
    GIT_REPOSITORY "git@github.com:nlohmann/json.git"
    GIT_TAG "master"
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/json"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(json SOURCE_DIR)
set(json_source_dir ${SOURCE_DIR})
message(STATUS "json_source_dir = ${json_source_dir}")

#
#
# draco
#
#

ExternalProject_Add(
    draco
    GIT_REPOSITORY "git@github.com:google/draco.git"
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/draco"
    GIT_TAG "1.3.4"
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    CMAKE_ARGS ${default_cmake_args}
    TEST_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(draco SOURCE_DIR)
ExternalProject_Get_Property(draco BINARY_DIR)
set(draco_source_dir ${SOURCE_DIR})
set(draco_binary_dir ${BINARY_DIR})
message(STATUS "draco_source_dir = ${draco_source_dir}")
message(STATUS "draco_binary_dir = ${draco_binary_dir}")

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
# taskflow
#
#

ExternalProject_Add(
    taskflow
    GIT_REPOSITORY "git@github.com:cpp-taskflow/cpp-taskflow.git"
    GIT_TAG "master"
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/cpp-taskflow"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(taskflow SOURCE_DIR)
set(taskflow_source_dir ${SOURCE_DIR})
message(STATUS "taskflow_source_dir = ${taskflow_source_dir}")

if (APPLE)
if (VIEWER_DOWNLOAD_AND_USE_VULKAN_SDK)

ExternalProject_Add(
    vulkansdk
    
    URL "https://sdk.lunarg.com/sdk/download/1.1.101.0/mac/vulkansdk-macos-1.1.101.0.tar.gz?u="
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/vulkansdk"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(vulkansdk SOURCE_DIR)
set(vulkansdk_source_dir ${SOURCE_DIR})
message(STATUS "vulkansdk_source_dir = ${vulkansdk_source_dir}")

set(VulkanSdk_DependencyProject vulkansdk)
set(VulkanSdk_IncludeDir ${vulkansdk_source_dir}/macOS/include ${vulkansdk_source_dir}/MoltenVK/include)
set(VulkanSdk_DebugLibrary ${vulkansdk_source_dir}/macOS/lib/libvulkan.1.1.101.dylib)
set(VulkanSdk_ReleaseLibrary ${vulkansdk_source_dir}/macOS/lib/libvulkan.1.1.101.dylib)
message(STATUS "VK_SDK_PATH = ${vulkansdk_source_dir}")

else()

ExternalProject_Add(
    MoltenVK
    GIT_REPOSITORY "git@github.com:KhronosGroup/MoltenVK.git"
    GIT_TAG "master"
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    TEST_COMMAND ""
    BUILD_IN_SOURCE 1
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/MoltenVK"
    BUILD_COMMAND   ./fetchDependencies &&
                    xcodebuild -project MoltenVKPackaging.xcodeproj -scheme "MoltenVK Package" build &&
                    xcodebuild -project MoltenVKPackaging.xcodeproj -scheme "MoltenVK Package (Debug)" build &&
                    xcodebuild -project MoltenVKPackaging.xcodeproj -scheme "MoltenVK Package (iOS only)" build &&
                    xcodebuild -project MoltenVKPackaging.xcodeproj -scheme "MoltenVK Package (macOS only)" build
                    # xcodebuild -project MoltenVKPackaging.xcodeproj -scheme "MoltenVK Package (Debug) (macOS only)" build &&
)

set(VulkanSdk_DependencyProject MoltenVK)
set(VulkanSdk_UseMoltenVK TRUE)
set(VulkanSdk_IncludeDir ${MOLTENVK_SDK_PATH}/MoltenVK/include)
set(VulkanSdk_DebugLibrary ${MOLTENVK_SDK_PATH}/Package/Debug/MoltenVK/macOS/dynamic/libMoltenVK.dylib)
set(VulkanSdk_ReleaseLibrary ${MOLTENVK_SDK_PATH}/Package/Release/MoltenVK/macOS/dynamic/libMoltenVK.dylib)
message(STATUS "MOLTENVK_SDK_PATH = ${MOLTENVK_SDK_PATH}")
 
endif()
endif()

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
# git@github.com:zeux/volk.git
#
#

ExternalProject_Add(
    volk
    GIT_REPOSITORY "git@github.com:zeux/volk.git"
    GIT_TAG "master"
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/volk"
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
# shaderc
#
#

# if (APPLE AND VIEWER_DOWNLOAD_AND_USE_VULKAN_SDK)

# set(Shaderc_IncludeDir)
# set(Shaderc_DebugCombinedLibrary)
# set(Shaderc_ReleaseCombinedLibrary)

# else()

ExternalProject_Add(
    shaderc
    GIT_REPOSITORY "git@github.com:google/shaderc.git"
    GIT_TAG "master"
    UPDATE_COMMAND ""
    PATCH_COMMAND python "${CMAKE_SOURCE_DIR}/dependencies/shaderc/utils/git-sync-deps"
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/shaderc"
    CMAKE_ARGS ${default_cmake_args} -DSHADERC_SKIP_TESTS:BOOL=TRUE -DSHADERC_SKIP_INSTALL:BOOL=TRUE -DSHADERC_ENABLE_SHARED_CRT:BOOL=TRUE
    TEST_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(shaderc SOURCE_DIR)
ExternalProject_Get_Property(shaderc BINARY_DIR)
set(shaderc_source_dir ${SOURCE_DIR})
set(shaderc_binary_dir ${BINARY_DIR})
message(STATUS "shaderc_source_dir = ${shaderc_source_dir}")
message(STATUS "shaderc_binary_dir = ${shaderc_binary_dir}")

set(Shaderc_IncludeDir ${shaderc_source_dir}/libshaderc/include)
set(Shaderc_DebugCombinedLibrary ${shaderc_binary_dir}/libshaderc/Debug/libshaderc_combined.a)
set(Shaderc_ReleaseCombinedLibrary ${shaderc_binary_dir}/libshaderc/Release/libshaderc_combined.a)

# endif()

if (UNIX)
add_definitions("-rdynamic")
endif()

if (UNIX AND APPLE)
add_definitions("-Wno-invalid-offsetof -Wno-deprecated-declarations -fno-omit-frame-pointer")
endif()

# add_definitions("-DAPEMODE_NO_GLOBAL_NEW_DELETE_OP_OVERRIDES")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

    # using Clang
    # add_definitions("-stdlib=libc++")
    add_definitions("-std=c++17")
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
    add_definitions("-std=c++17")
    add_definitions( "-DHAVE_PTHREAD" )
    add_definitions( "-D_XOPEN_SOURCE=600" )
    add_definitions( "-D_DARWIN_C_SOURCE=600" )
    set( CONFIGURATION_SUFFIX "/$CONFIGURATION" )

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
# ApemodeVulkan
#
#

set(ApemodeVulkan_vk_source_files
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

set(ApemodeVulkan_vk_ext_source_files
    ${CMAKE_SOURCE_DIR}/src/apemode/vk_ext/AppSurface.Vulkan.h
    ${CMAKE_SOURCE_DIR}/src/apemode/vk_ext/AppSurface.Vulkan.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/vk_ext/ImageUploader.Vulkan.h
    ${CMAKE_SOURCE_DIR}/src/apemode/vk_ext/ImageUploader.Vulkan.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/vk_ext/SamplerManager.Vulkan.h
    ${CMAKE_SOURCE_DIR}/src/apemode/vk_ext/SamplerManager.Vulkan.cpp
)

if (APPLE)

    set( Vulkan_include_dirs ${VulkanSdk_IncludeDir} )
    set( Vulkan_third_party_libs
         debug ${VulkanSdk_DebugLibrary}
         optimized ${VulkanSdk_ReleaseLibrary}
         debug ${eastl_binary_dir}/Debug/libEASTL.a
         debug ${eastl_binary_dir}/test/packages/EAStdC/Debug/libEAStdC.a
         optimized ${eastl_binary_dir}/Release/libEASTL.a
         optimized ${eastl_binary_dir}/test/packages/EAStdC/Release/libEAStdC.a )

elseif(WIN32)

    set( Vulkan_include_dirs $ENV{VK_SDK_PATH}/include )
    set( Vulkan_third_party_libs
         $ENV{VK_SDK_PATH}/lib/vulkan-1.lib
         ${eastl_binary_dir}${CONFIGURATION_SUFFIX}/EASTL.lib
         ${eastl_binary_dir}/test/packages/EAStdC${CONFIGURATION_SUFFIX}/EAStdC.lib )

elseif (UNIX)

    set( Vulkan_include_dirs $ENV{VK_SDK_PATH}/include )
    set( Vulkan_third_party_libs
         $ENV{VK_SDK_PATH}/lib/libvulkan.so
         ${eastl_binary_dir}${CONFIGURATION_SUFFIX}/libEASTL.a
         ${eastl_binary_dir}/test/packages/EAStdC${CONFIGURATION_SUFFIX}/libEAStdC.a )

endif()

#
#
# ApemodePlatform
#
#

if (APPLE)

    set(ApemodePlatform_third_party_libs
        debug ${eastl_binary_dir}/Debug/libEASTL.a
        debug ${eastl_binary_dir}/test/packages/EAStdC/Debug/libEAStdC.a
        optimized ${eastl_binary_dir}/Release/libEASTL.a
        optimized ${eastl_binary_dir}/test/packages/EAStdC/Release/libEAStdC.a
        pthread
        )

elseif(WIN32)

    set( ApemodePlatform_third_party_libs
         ${eastl_binary_dir}${CONFIGURATION_SUFFIX}/EASTL.lib
         ${eastl_binary_dir}/test/packages/EAStdC${CONFIGURATION_SUFFIX}/EAStdC.lib
         )

elseif (UNIX)

    set( ApemodePlatform_third_party_libs
         ${eastl_binary_dir}${CONFIGURATION_SUFFIX}/libEASTL.a
         ${eastl_binary_dir}/test/packages/EAStdC${CONFIGURATION_SUFFIX}/libEAStdC.a
         )

endif()

set(
    ApemodePlatform_source_files
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/memory/MemoryManager.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/memory/MemoryManager.h
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/shared/AssetManager.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/shared/AssetManager.h
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/AppState.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/AppState.h
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/AppInput.h
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/AppSurface.h
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/IAppShell.h
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/IAssetManager.h
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/ArrayUtils.h
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/CityHash.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/CityHash.h
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/MathInc.h
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/Stopwatch.h
)

#
#
# ViewerSdl
#
#

set(
    Viewer_source_files
    ${CMAKE_SOURCE_DIR}/src/viewer/ScenePrintPretty.cpp
    ${CMAKE_SOURCE_DIR}/src/viewer/Scene.cpp
    ${CMAKE_SOURCE_DIR}/src/viewer/Scene.h
    ${CMAKE_SOURCE_DIR}/src/viewer/NuklearRendererBase.cpp
    ${CMAKE_SOURCE_DIR}/src/viewer/NuklearRendererBase.h
    ${CMAKE_SOURCE_DIR}/src/viewer/ViewerAppShellFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/viewer/ViewerAppShellFactory.h
    ${CMAKE_SOURCE_DIR}/src/viewer/vk/ViewerShellVk.cpp
    ${CMAKE_SOURCE_DIR}/src/viewer/vk/ViewerShellVk.h
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

#
#
# Shaderc
#
#

add_executable(
    ShaderPipeline
    ${CMAKE_SOURCE_DIR}/src/shaderc/ShaderCompiler.Vulkan.h
    ${CMAKE_SOURCE_DIR}/src/shaderc/ShaderCompiler.Vulkan.cpp
    ${CMAKE_SOURCE_DIR}/src/shaderc/main.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/memory/MemoryManager.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/memory/MemoryManager.h
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/shared/AssetManager.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/shared/AssetManager.h
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/AppState.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/AppState.h
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/IAssetManager.h
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/CityHash.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/CityHash.h
    )

target_include_directories(
    ShaderPipeline
    PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/generated
    ${CMAKE_SOURCE_DIR}/src/shaderc
    ${CMAKE_SOURCE_DIR}/dependencies/argh
    ${CMAKE_SOURCE_DIR}/dependencies/spdlog/include
    ${CMAKE_SOURCE_DIR}/dependencies/cpp-taskflow
    ${CMAKE_SOURCE_DIR}/dependencies/json/include
    ${CMAKE_SOURCE_DIR}/dependencies/flatbuffers/include
    ${CMAKE_SOURCE_DIR}/dependencies/eastl/include
    ${CMAKE_SOURCE_DIR}/dependencies/eastl/test/packages/EAAssert/include
    ${CMAKE_SOURCE_DIR}/dependencies/eastl/test/packages/EABase/include
    ${CMAKE_SOURCE_DIR}/dependencies/eastl/test/packages/EABase/include/Common
    ${CMAKE_SOURCE_DIR}/dependencies/eastl/test/packages/EAMain/include
    ${CMAKE_SOURCE_DIR}/dependencies/eastl/test/packages/EAStdC/include
    ${Shaderc_IncludeDir}
    # ${CMAKE_SOURCE_DIR}/dependencies/shaderc/libshaderc/include
    # ${CMAKE_SOURCE_DIR}/dependencies/shaderc/third_party/spirv-cross
    # ${CMAKE_SOURCE_DIR}/dependencies/shaderc/third_party/spirv-cross/include
    )

add_dependencies(
    ShaderPipeline
    shaderc
    json
    flatbuffers
    DirectXMath
    eastl
    gli
    argh
    spdlog
    taskflow
)

if (APPLE )
    target_link_libraries(
        ShaderPipeline

        debug ${eastl_binary_dir}/Debug/libEASTL.a
        debug ${eastl_binary_dir}/test/packages/EAStdC/Debug/libEAStdC.a
        debug ${flatbuffers_binary_dir}/Debug/libflatbuffers.a
        debug ${Shaderc_DebugCombinedLibrary}
        # debug ${shaderc_binary_dir}/libshaderc/Debug/libshaderc_combined.a

        optimized ${eastl_binary_dir}/Release/libEASTL.a
        optimized ${eastl_binary_dir}/test/packages/EAStdC/Release/libEAStdC.a
        optimized ${flatbuffers_binary_dir}/Release/libflatbuffers.a
        optimized ${Shaderc_ReleaseCombinedLibrary}
        # optimized ${shaderc_binary_dir}/libshaderc/Release/libshaderc_combined.a
        
        dl
        stdc++
        objc
        iconv
        )
elseif (WIN32)
    target_link_libraries(
        ShaderPipeline
        ${eastl_binary_dir}${CONFIGURATION_SUFFIX}/EASTL.lib
        ${eastl_binary_dir}/test/packages/EAStdC${CONFIGURATION_SUFFIX}/EAStdC.lib
        ${shaderc_binary_dir}/libshaderc${CONFIGURATION_SUFFIX}/shaderc_combined.lib
        ${flatbuffers_binary_dir}${CONFIGURATION_SUFFIX}/flatbuffers.lib
        )
elseif (UNIX)
    target_link_libraries(
        ShaderPipeline

        ${eastl_binary_dir}${CONFIGURATION_SUFFIX}/libEASTL.a
        ${eastl_binary_dir}/test/packages/EAStdC${CONFIGURATION_SUFFIX}/libEAStdC.a
        ${shaderc_binary_dir}/libshaderc${CONFIGURATION_SUFFIX}/libshaderc_combined.a
        ${flatbuffers_binary_dir}${CONFIGURATION_SUFFIX}/libflatbuffers.a
       
        pthread
        stdc++fs
        dl
        )
endif ()

#
#
# ViewerSdl
#
#

set(
    ViewerSdl_source_files
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/sdl2/AppSurfaceSdl.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/sdl2/AppSurfaceSdl.h
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/sdl2/AppSurfaceSdl.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/sdl2/InputManagerSdl.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/sdl2/InputManagerSdl.h
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/sdl2/AppMainSdl.cpp
)

if (APPLE)
    set ( ViewerSdl_source_files
          ${ViewerSdl_source_files}
          ${CMAKE_SOURCE_DIR}/src/apemode/platform/sdl2/AppSurfaceCocoaSdl.mm
        )
endif()

add_executable( ViewerSdl
                ${ApemodePlatform_source_files}
                ${Viewer_source_files}
                ${ViewerSdl_source_files} 
                ${ApemodeVulkan_vk_source_files}
                ${ApemodeVulkan_vk_ext_source_files}
                )

target_include_directories(
    ViewerSdl
    PUBLIC
    ${CMAKE_SOURCE_DIR}/dependencies/DirectXMath
    ${CMAKE_SOURCE_DIR}/dependencies/DirectXMath/Inc
    ${CMAKE_SOURCE_DIR}/dependencies/FbxPipeline/FbxPipeline/generated
    ${CMAKE_SOURCE_DIR}/dependencies/FbxPipeline/FbxPipeline/include/utils
    ${CMAKE_SOURCE_DIR}/dependencies/Math
    ${CMAKE_SOURCE_DIR}/dependencies/volk
    ${CMAKE_SOURCE_DIR}/dependencies/VulkanMemoryAllocator/src
    ${CMAKE_SOURCE_DIR}/dependencies/argh
    ${CMAKE_SOURCE_DIR}/dependencies/cpp-taskflow
    ${CMAKE_SOURCE_DIR}/dependencies/draco/src
    ${CMAKE_SOURCE_DIR}/dependencies/eastl/include
    ${CMAKE_SOURCE_DIR}/dependencies/eastl/test/packages/EAAssert/include
    ${CMAKE_SOURCE_DIR}/dependencies/eastl/test/packages/EABase/include
    ${CMAKE_SOURCE_DIR}/dependencies/eastl/test/packages/EABase/include/Common
    ${CMAKE_SOURCE_DIR}/dependencies/eastl/test/packages/EAMain/include
    ${CMAKE_SOURCE_DIR}/dependencies/eastl/test/packages/EAMain/include
    ${CMAKE_SOURCE_DIR}/dependencies/eastl/test/packages/EAStdC/include
    ${CMAKE_SOURCE_DIR}/dependencies/flatbuffers/include
    ${CMAKE_SOURCE_DIR}/dependencies/gli
    ${CMAKE_SOURCE_DIR}/dependencies/gli/external
    ${CMAKE_SOURCE_DIR}/dependencies/gli/external/glm
    ${CMAKE_SOURCE_DIR}/dependencies/nuklear
    ${CMAKE_SOURCE_DIR}/dependencies/sdl2/include
    ${CMAKE_SOURCE_DIR}/dependencies/spdlog/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/apemode/vk
    ${CMAKE_SOURCE_DIR}/src/apemode/vk_ext
    ${CMAKE_SOURCE_DIR}/src/viewer
    ${CMAKE_SOURCE_DIR}/src/viewer/vk
    ${Vulkan_include_dirs}
    ${Vulkan_include_dirs}
    ${draco_binary_dir}
)

set(ViewerSdl_AdditionalDependencies)

if (APPLE)
    set(ViewerSdl_AdditionalDependencies ${VulkanSdk_DependencyProject})
    target_link_libraries(
        ViewerSdl
        ${sdl2_binary_dir}${CONFIGURATION_SUFFIX}/libSDL2.a
        ${draco_binary_dir}${CONFIGURATION_SUFFIX}/libdraco.a
        ${flatbuffers_binary_dir}${CONFIGURATION_SUFFIX}/libflatbuffers.a
        ${sdl2_binary_dir}${CONFIGURATION_SUFFIX}/libSDL2.a
        ${draco_binary_dir}${CONFIGURATION_SUFFIX}/libdraco.a
        ${flatbuffers_binary_dir}${CONFIGURATION_SUFFIX}/libflatbuffers.a
        ${Vulkan_third_party_libs}
        ${ApemodePlatform_third_party_libs}
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
elseif (WIN32)
    target_link_libraries(
        ViewerSdl
        ${sdl2_binary_dir}${CONFIGURATION_SUFFIX}/SDL2.lib
        ${sdl2_binary_dir}${CONFIGURATION_SUFFIX}/SDL2main.lib
        ${draco_binary_dir}${CONFIGURATION_SUFFIX}/draco.lib
        ${flatbuffers_binary_dir}${CONFIGURATION_SUFFIX}/flatbuffers.lib
        ${ApemodePlatform_third_party_libs}
        ${Vulkan_third_party_libs}
        )
elseif (UNIX)
    target_link_libraries(
        ViewerSdl
        ${sdl2_binary_dir}${CONFIGURATION_SUFFIX}/libSDL2.a
        ${draco_binary_dir}${CONFIGURATION_SUFFIX}/libdraco.a
        ${flatbuffers_binary_dir}${CONFIGURATION_SUFFIX}/libflatbuffers.a
        ${Vulkan_third_party_libs}
        ${ApemodePlatform_third_party_libs}
        X11
        Xext
        Xcursor
        Xinerama
        Xrandr
        pthread
        stdc++fs
        dl
        )
endif ()

add_dependencies(
    ViewerSdl
    sdl2
    nuklear
    draco
    flatbuffers
    DirectXMath
    FbxPipeline
    eastl
    gli
    VulkanMemoryAllocator
    volk
    argh
    spdlog
    taskflow
    ${ViewerSdl_AdditionalDependencies}
)

message(STATUS "VK_SDK_PATH = $ENV{VK_SDK_PATH}")

if (APPLE)
    add_custom_command( TARGET ViewerSdl PRE_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/Debug")
    add_custom_command( TARGET ViewerSdl PRE_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/Release")

    add_custom_command(
        TARGET ViewerSdl POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${VulkanSdk_DebugLibrary}
        ${CMAKE_BINARY_DIR}/Debug
    )

    add_custom_command(
        TARGET ViewerSdl POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${VulkanSdk_ReleaseLibrary}
        ${CMAKE_BINARY_DIR}/Release
    )
endif()

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
        ViewerSdl
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${viewer_dependencies_dll_pdb}
        ${CMAKE_BINARY_DIR}${CONFIGURATION_SUFFIX}
    )
endforeach()

set_target_properties(
    ViewerSdl
    PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY
    "$(OutDir)"
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(PREDEFINED_TARGETS_FOLDER "CustomTargets")
