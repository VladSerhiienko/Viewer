cmake_minimum_required(VERSION 3.4.1)

include(ExternalProject)
project (Viewer)

message(STATUS "CMAKE_SYSTEM_INFO_FILE = ${CMAKE_SYSTEM_INFO_FILE}")
message(STATUS "CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR = ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "CMAKE_SYSTEM = ${CMAKE_SYSTEM}")
message(STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR = ${CMAKE_BINARY_DIR}")

set(BUILD_FOLDER_SUFFIX "build_${CMAKE_SYSTEM_NAME}_${CMAKE_SYSTEM_PROCESSOR}")
string(TOLOWER  ${BUILD_FOLDER_SUFFIX} BUILD_FOLDER_SUFFIX)
message(STATUS "BUILD_FOLDER_SUFFIX = ${BUILD_FOLDER_SUFFIX}")

if (WIN32)
    set(
        default_cmake_args
        -G
        "Visual Studio 15 2017 Win64"
        -B${BUILD_FOLDER_SUFFIX} "-H."
    )
else()
    set(
        default_cmake_args
        -B${BUILD_FOLDER_SUFFIX} -H.
    )
endif()

#
#
# cxxopts
#
#

ExternalProject_Add(
    cxxopts
    GIT_REPOSITORY "git@github.com:jarro2783/cxxopts.git"
    GIT_TAG "1_0"
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/cxxopts"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(cxxopts SOURCE_DIR)
set(cxxopts_source_dir ${SOURCE_DIR})
message(STATUS "cxxopts_source_dir = ${cxxopts_source_dir}")

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
# lodepng
#
#

ExternalProject_Add(
    lodepng
    GIT_REPOSITORY "git@github.com:lvandeve/lodepng.git"
    GIT_TAG "master"
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/lodepng"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(lodepng SOURCE_DIR)
set(lodepng_source_dir ${SOURCE_DIR})
message(STATUS "lodepng_source_dir = ${lodepng_source_dir}")

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
    GIT_TAG "master"
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
    CMAKE_ARGS -B${BUILD_FOLDER_SUFFIX} -H. -DCMAKE_DEBUG_POSTFIX:STRING=
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
# spirv_headers
#
#

ExternalProject_Add(
    spirv_headers
    GIT_REPOSITORY "git@github.com:KhronosGroup/SPIRV-Headers.git"
    GIT_TAG "master"
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/SPIRV-Headers"
    CMAKE_ARGS -B${BUILD_FOLDER_SUFFIX} -H.
    TEST_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(spirv_headers SOURCE_DIR)
ExternalProject_Get_Property(spirv_headers BINARY_DIR)
set(spirv_headers_source_dir ${SOURCE_DIR})
set(spirv_headers_binary_dir ${BINARY_DIR})
message(STATUS "spirv_headers_source_dir = ${spirv_headers_source_dir}")
message(STATUS "spirv_headers_binary_dir = ${spirv_headers_binary_dir}")

#
#
# glslang
#
#

ExternalProject_Add(
    glslang
    GIT_REPOSITORY "git@github.com:KhronosGroup/glslang.git"
    GIT_TAG "master"
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/glslang"
    CMAKE_ARGS -B${BUILD_FOLDER_SUFFIX} -H.
    TEST_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(glslang SOURCE_DIR)
ExternalProject_Get_Property(glslang BINARY_DIR)
set(glslang_source_dir ${SOURCE_DIR})
set(glslang_binary_dir ${BINARY_DIR})
message(STATUS "glslang_source_dir = ${glslang_source_dir}")
message(STATUS "glslang_binary_dir = ${glslang_binary_dir}")

#
#
# spirv_tools
#
#

ExternalProject_Add(
    spirv_tools
    DEPENDS spirv_headers glslang
    GIT_REPOSITORY "git@github.com:KhronosGroup/SPIRV-Tools.git"
    GIT_TAG "master"
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/SPIRV-Tools"
    CMAKE_ARGS -B${BUILD_FOLDER_SUFFIX} -H. -DSPIRV-Headers_SOURCE_DIR:PATH=${spirv_headers_source_dir}
    TEST_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(spirv_tools SOURCE_DIR)
ExternalProject_Get_Property(spirv_tools BINARY_DIR)
set(spirv_tools_source_dir ${SOURCE_DIR})
set(spirv_tools_binary_dir ${BINARY_DIR})
message(STATUS "spirv_tools_source_dir = ${spirv_tools_source_dir}")
message(STATUS "spirv_tools_binary_dir = ${spirv_tools_binary_dir}")

#
#
# shaderc
#
#

ExternalProject_Add(
    shaderc
    DEPENDS spirv_tools spirv_headers glslang
    GIT_REPOSITORY "git@github.com:google/shaderc.git"
    GIT_TAG "master"
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/shaderc"
    CMAKE_ARGS -B${BUILD_FOLDER_SUFFIX} -H. -DSHADERC_ENABLE_SHARED_CRT:BOOL=TRUE # MD
                                            -DSHADERC_SKIP_TESTS:BOOL=TRUE
                                            -DSHADERC_SPIRV_TOOLS_DIR:PATH=${spirv_tools_source_dir}
                                            -DSHADERC_SPIRV_HEADERS_DIR:PATH=${spirv_headers_source_dir}
                                            -DSHADERC_GLSLANG_DIR:PATH=${glslang_source_dir}
    TEST_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(shaderc SOURCE_DIR)
ExternalProject_Get_Property(shaderc BINARY_DIR)
set(shaderc_source_dir ${SOURCE_DIR})
set(shaderc_binary_dir ${BINARY_DIR})
message(STATUS "shaderc_source_dir = ${shaderc_source_dir}")
message(STATUS "shaderc_binary_dir = ${shaderc_binary_dir}")

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
    CMAKE_ARGS -B${BUILD_FOLDER_SUFFIX} -H.
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

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")

if (WIN32)
    set(CONFIGURATION_SUFFIX "/$(Configuration)")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_CRT_SECURE_NO_WARNINGS")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -D_CRT_SECURE_NO_WARNINGS")
endif()

if (UNIX)
    add_definitions("-g")
    
    if (CMAKE_VERSION VERSION_LESS "3.1")
      if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
      endif ()
    else ()
      set (CMAKE_CXX_STANDARD 11)
    endif ()

    add_definitions("-DHAVE_PTHREAD")
    set(CONFIGURATION_SUFFIX "")
endif()

#
#
# shared sources
#
#

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
    ${CMAKE_SOURCE_DIR}/src/shared/platform/AssetManager.cpp
    ${CMAKE_SOURCE_DIR}/src/shared/platform/FileManager.cpp
    ${CMAKE_SOURCE_DIR}/src/shared/platform/memory/MemoryManager.cpp
    ${CMAKE_SOURCE_DIR}/src/shared/platform/memory/MemoryManager.h
)

set(
    shared_source_files
    ${CMAKE_SOURCE_DIR}/src/shared/CityHash.cpp      
    ${CMAKE_SOURCE_DIR}/src/shared/AppBase.cpp 
    ${CMAKE_SOURCE_DIR}/src/shared/DebugRendererVk.cpp 
    ${CMAKE_SOURCE_DIR}/src/shared/AppState.cpp 
    ${CMAKE_SOURCE_DIR}/src/shared/AppSurfaceSdlVk.cpp  
    ${CMAKE_SOURCE_DIR}/src/shared/NuklearSdlVk.cpp
)

# Sources from external projects
add_custom_command(
    OUTPUT
    ${CMAKE_SOURCE_DIR}/dependencies/lodepng/lodepng.cpp
    DEPENDS Viewer
    COMMAND ""
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
    ${CMAKE_SOURCE_DIR}/dependencies/lodepng/lodepng.cpp
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
    cxxopts
    spdlog
    zlib
    flatbuffers
    nuklear
    shaderc
    DirectXMath
    FbxPipeline
    eastl
    lodepng
    gli
    sdl2
    VulkanMemoryAllocator
)

message(STATUS "VK_SDK_PATH = $ENV{VK_SDK_PATH}")

target_include_directories(
    Viewer
    PUBLIC
    ${CMAKE_SOURCE_DIR}/assets/
    ${CMAKE_SOURCE_DIR}/assets/fonts/
    ${CMAKE_SOURCE_DIR}/src/shared
    ${CMAKE_SOURCE_DIR}/src/shared/platform
    ${CMAKE_SOURCE_DIR}/src/shared/sdl
    ${CMAKE_SOURCE_DIR}/src/shared/vk
    ${CMAKE_SOURCE_DIR}/src/shared/vk_ext
    ${CMAKE_SOURCE_DIR}/src/shared/platform/memory
    ${CMAKE_SOURCE_DIR}/src/viewer
    ${CMAKE_SOURCE_DIR}/dependencies/gli
    ${CMAKE_SOURCE_DIR}/dependencies/gli/external
    ${CMAKE_SOURCE_DIR}/dependencies/gli/external/glm
    ${CMAKE_SOURCE_DIR}/dependencies/lodepng
    ${CMAKE_SOURCE_DIR}/dependencies/nuklear
    ${CMAKE_SOURCE_DIR}/dependencies/VulkanMemoryAllocator/src
    ${CMAKE_SOURCE_DIR}/dependencies/DirectXMath
    ${CMAKE_SOURCE_DIR}/dependencies/DirectXMath/Inc
    ${CMAKE_SOURCE_DIR}/dependencies/cxxopts/include
    ${CMAKE_SOURCE_DIR}/dependencies/spdlog/include
    ${CMAKE_SOURCE_DIR}/dependencies/flatbuffers/include
    ${CMAKE_SOURCE_DIR}/dependencies/sdl2/include
    ${CMAKE_SOURCE_DIR}/dependencies/EASTL/include
    ${CMAKE_SOURCE_DIR}/dependencies/FbxPipeline/FbxPipeline/generated
    ${shaderc_source_dir}/libshaderc/include
    $ENV{VK_SDK_PATH}/include
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
        ${sdl2_binary_dir}/libSDL2.a
        ${shaderc_binary_dir}/libshaderc/libshaderc_combined.a
        ${flatbuffers_binary_dir}/libflatbuffers.a
        ${eastl_binary_dir}/libEASTL.a
        ${eastl_binary_dir}/test/packages/EAStdC/libEAStdC.a
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
        
        ${sdl2_binary_dir}${CONFIGURATION_SUFFIX}/SDL2.lib
        ${sdl2_binary_dir}${CONFIGURATION_SUFFIX}/SDL2main.lib
        ${eastl_binary_dir}${CONFIGURATION_SUFFIX}/EASTL.lib
        ${flatbuffers_binary_dir}${CONFIGURATION_SUFFIX}/flatbuffers.lib
        ${eastl_binary_dir}/test/packages/EAStdC${CONFIGURATION_SUFFIX}/EAStdC.lib
        ${shaderc_binary_dir}/libshaderc${CONFIGURATION_SUFFIX}/shaderc_combined.lib
    )
endif()

make_directory(${CMAKE_BINARY_DIR})
make_directory(${CMAKE_BINARY_DIR}/shaders)
make_directory(${CMAKE_BINARY_DIR}/Debug/shaders)
make_directory(${CMAKE_BINARY_DIR}/Release/shaders)

foreach( Viewer_shader_source_file ${Viewer_shader_source_files} )
    if(EXISTS ${Viewer_shader_source_file})
        add_custom_command(
            TARGET
            Viewer
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${Viewer_shader_source_file}
            ${CMAKE_BINARY_DIR}${CONFIGURATION_SUFFIX}/shaders/
        )
    endif()
endforeach()

if(WIN32)
    set( viewer_dependencies_dlls_pdbs
        ${sdl2_binary_dir}${CONFIGURATION_SUFFIX}/SDL2.dll
        ${sdl2_binary_dir}${CONFIGURATION_SUFFIX}/SDL2.pdb
        )
else()
    set( viewer_dependencies_dlls_pdbs
        ${sdl2_binary_dir}${CONFIGURATION_SUFFIX}/libSDL2-2.0.so
        )
endif()

foreach( viewer_dependencies_dll_pdb ${viewer_dependencies_dlls_pdbs} )
    add_custom_command(
        TARGET
        Viewer
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${viewer_dependencies_dll_pdb}
        ${CMAKE_BINARY_DIR}${CONFIGURATION_SUFFIX}
    )
endforeach()

target_link_libraries(
    Viewer
    ${third_party_libs}
)

set_target_properties(
    Viewer
    PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY
    "$(OutDir)"
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(PREDEFINED_TARGETS_FOLDER "CustomTargets")