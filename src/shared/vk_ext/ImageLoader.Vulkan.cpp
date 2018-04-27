#include <ImageLoader.Vulkan.h>
#include <MemoryManager.h>

#include <BufferPools.Vulkan.h>
#include <QueuePools.Vulkan.h>

#ifndef LODEPNG_NO_COMPILE_ALLOCATORS
#define LODEPNG_NO_COMPILE_ALLOCATORS
#endif

void* lodepng_malloc( size_t size ) {
    return apemode_malloc( size, APEMODE_DEFAULT_ALIGNMENT );
}

void* lodepng_realloc( void* p, size_t size ) {
    return apemode_realloc( p, size, APEMODE_DEFAULT_ALIGNMENT );
}

void lodepng_free( void* p ) {
    apemode_free( p );
}

#include <lodepng.h>
#include <lodepng_util.h>

#pragma warning(disable:4309)
#include <gli/gli.hpp>
#pragma warning(default:4309)

/**
 * Exactly the same, no need for explicit mapping.
 * But still decided to leave it as a function in case GLI stops maintain such compatibility.
 */
VkFormat ToImgFormat( gli::format textureFormat ) {
    return static_cast< VkFormat >( textureFormat );
}

VkImageType ToImgType( gli::target textureTarget ) {
    switch ( textureTarget ) {
        case gli::TARGET_1D:            return VK_IMAGE_TYPE_1D;
        case gli::TARGET_1D_ARRAY:      return VK_IMAGE_TYPE_1D;
        case gli::TARGET_2D:            return VK_IMAGE_TYPE_2D;
        case gli::TARGET_2D_ARRAY:      return VK_IMAGE_TYPE_2D;
        case gli::TARGET_3D:            return VK_IMAGE_TYPE_3D;
        case gli::TARGET_RECT:          return VK_IMAGE_TYPE_2D;
        case gli::TARGET_RECT_ARRAY:    return VK_IMAGE_TYPE_2D;
        case gli::TARGET_CUBE:          return VK_IMAGE_TYPE_2D;
        case gli::TARGET_CUBE_ARRAY:    return VK_IMAGE_TYPE_2D;
        default:                        return VK_IMAGE_TYPE_MAX_ENUM;
    }
}

VkImageViewType ToImgViewType( gli::target textureTarget ) {
    switch ( textureTarget ) {
        case gli::TARGET_1D:            return VK_IMAGE_VIEW_TYPE_1D;
        case gli::TARGET_1D_ARRAY:      return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
        case gli::TARGET_2D:            return VK_IMAGE_VIEW_TYPE_2D;
        case gli::TARGET_2D_ARRAY:      return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        case gli::TARGET_3D:            return VK_IMAGE_VIEW_TYPE_3D;
        case gli::TARGET_RECT:          return VK_IMAGE_VIEW_TYPE_2D;
        case gli::TARGET_RECT_ARRAY:    return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        case gli::TARGET_CUBE:          return VK_IMAGE_VIEW_TYPE_CUBE;
        case gli::TARGET_CUBE_ARRAY:    return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
        default:                        return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    }
}

bool apemodevk::ImageLoader::Recreate( GraphicsDevice* pInNode, HostBufferPool* pInHostBufferPool ) {
    pNode = pInNode;

    if ( nullptr == pInHostBufferPool ) {
        pHostBufferPool = new HostBufferPool( );
        pHostBufferPool->Recreate( pNode, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, false );
    } else {
        pHostBufferPool = pInHostBufferPool;
    }

    return true;
}

std::unique_ptr< apemodevk::LoadedImage > apemodevk::ImageLoader::LoadImageFromData(
    const std::vector< uint8_t >& InFileContent, EImageFileFormat eFileFormat, bool bImgView, bool bAwaitLoading ) {

    auto loadedImage = apemodevk::make_unique< LoadedImage >( );

    std::vector< VkBufferImageCopy > bufferImageCopies;
    VkImageMemoryBarrier             writeImageMemoryBarrier;
    VkImageMemoryBarrier             readImgMemoryBarrier;
    HostBufferPool::SuballocResult   imageBufferSuballocResult;

    InitializeStruct( writeImageMemoryBarrier );
    InitializeStruct( readImgMemoryBarrier );
    InitializeStruct( loadedImage->ImageCreateInfo );
    InitializeStruct( loadedImage->ImgViewCreateInfo );

    pHostBufferPool->Reset( );

    /**
     * @note All the data will be uploaded in switch cases,
     *       all the structures that depend on image type and dimensions
     *       will be filled in switch cases.
     **/
    switch ( eFileFormat ) {
        case apemodevk::ImageLoader::eImageFileFormat_DDS:
        case apemodevk::ImageLoader::eImageFileFormat_KTX: {
            auto texture = gli::load( (const char*) InFileContent.data( ), InFileContent.size( ) );

            if ( false == texture.empty( ) ) {

                loadedImage->ImageCreateInfo.format        = ToImgFormat( texture.format( ) );
                loadedImage->ImageCreateInfo.imageType     = ToImgType( texture.target( ) );
                loadedImage->ImageCreateInfo.extent.width  = (uint32_t) texture.extent( ).x;
                loadedImage->ImageCreateInfo.extent.height = (uint32_t) texture.extent( ).y;
                loadedImage->ImageCreateInfo.extent.depth  = (uint32_t) texture.extent( ).z;
                loadedImage->ImageCreateInfo.mipLevels     = (uint32_t) texture.levels( );
                loadedImage->ImageCreateInfo.arrayLayers   = (uint32_t) texture.faces( );
                loadedImage->ImageCreateInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
                loadedImage->ImageCreateInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
                loadedImage->ImageCreateInfo.usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                loadedImage->ImageCreateInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
                loadedImage->ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

                loadedImage->ImgViewCreateInfo.flags                           = 0;
                loadedImage->ImgViewCreateInfo.format                          = ToImgFormat( texture.format( ) );
                loadedImage->ImgViewCreateInfo.viewType                        = ToImgViewType( texture.target( ) );
                loadedImage->ImgViewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                loadedImage->ImgViewCreateInfo.subresourceRange.levelCount     = (uint32_t) texture.levels( );
                loadedImage->ImgViewCreateInfo.subresourceRange.layerCount     = (uint32_t) texture.faces( );
                loadedImage->ImgViewCreateInfo.subresourceRange.baseMipLevel   = 0;
                loadedImage->ImgViewCreateInfo.subresourceRange.baseArrayLayer = 0;

                switch ( loadedImage->ImgViewCreateInfo.viewType ) {
                    case VK_IMAGE_VIEW_TYPE_CUBE:
                    case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY: {
                        loadedImage->ImageCreateInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
                    } break;
                }

                imageBufferSuballocResult = pHostBufferPool->Suballocate( texture.data( ), (uint32_t) texture.size( ) );

                uint32_t bufferImageCopyIndex = 0;
                bufferImageCopies.resize( ( uint32_t )( texture.levels( ) * texture.faces( ) ) );

                for ( size_t mipLevel = 0; mipLevel < texture.levels( ); ++mipLevel ) {
                    for ( size_t face = 0; face < texture.faces( ); ++face ) {
                        auto& bufferImageCopy = bufferImageCopies[ bufferImageCopyIndex++ ];
                        InitializeStruct( bufferImageCopy );

                        /**
                         * Note, that the texture will be destructed at the end of the 'case' scope.
                         * All the memory was copied to the pHostBufferPool.
                         **/

                        uintptr_t    imgAddress    = (uintptr_t) texture.data( );
                        uintptr_t    subImgAddress = (uintptr_t) texture.data( 0, face, mipLevel );
                        VkDeviceSize imageOffset   = subImgAddress - imgAddress;
                        VkDeviceSize bufferOffset  = imageBufferSuballocResult.DynamicOffset + imageOffset;

                        bufferImageCopy.imageSubresource.baseArrayLayer = 0;
                        bufferImageCopy.imageSubresource.mipLevel       = (uint32_t) mipLevel;
                        bufferImageCopy.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                        bufferImageCopy.imageSubresource.baseArrayLayer = (uint32_t) face;
                        bufferImageCopy.imageSubresource.layerCount     = 1;
                        bufferImageCopy.imageExtent.width               = (uint32_t) texture.extent( mipLevel ).x;
                        bufferImageCopy.imageExtent.height              = (uint32_t) texture.extent( mipLevel ).y;
                        bufferImageCopy.imageExtent.depth               = (uint32_t) texture.extent( mipLevel ).z;
                        bufferImageCopy.bufferOffset                    = bufferOffset;
                        bufferImageCopy.bufferImageHeight               = 0; /* Tightly packed according to the imageExtent */
                        bufferImageCopy.bufferRowLength                 = 0; /* Tightly packed according to the imageExtent */
                    }
                }

                writeImageMemoryBarrier.dstAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;
                writeImageMemoryBarrier.oldLayout                   = VK_IMAGE_LAYOUT_UNDEFINED;
                writeImageMemoryBarrier.newLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                writeImageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                writeImageMemoryBarrier.subresourceRange.levelCount = (uint32_t) texture.levels( );
                writeImageMemoryBarrier.subresourceRange.layerCount = (uint32_t) texture.faces( );

                readImgMemoryBarrier.srcAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;
                readImgMemoryBarrier.dstAccessMask               = VK_ACCESS_SHADER_READ_BIT;
                readImgMemoryBarrier.oldLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                readImgMemoryBarrier.newLayout                   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                readImgMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                readImgMemoryBarrier.subresourceRange.levelCount = (uint32_t) texture.levels( );
                readImgMemoryBarrier.subresourceRange.layerCount = (uint32_t) texture.faces( );

                loadedImage->eImgLayout = readImgMemoryBarrier.newLayout;
            }
        } break;
        case apemodevk::ImageLoader::eImageFileFormat_PNG: {

            /* Ensure no leaks */
            struct LodePNGStateWrapper {
                LodePNGState state;
                LodePNGStateWrapper( ) { lodepng_state_init( &state ); }
                ~LodePNGStateWrapper( ) { lodepng_state_cleanup( &state ); }
            } stateWrapper;

            /* Load png file here from memory buffer */
            uint8_t* pImageBytes = nullptr;
            uint32_t imageHeight = 0;
            uint32_t imageWidth  = 0;

            if ( 0 == lodepng_decode( &pImageBytes,
                                      &imageWidth,
                                      &imageHeight,
                                      &stateWrapper.state,
                                      InFileContent.data( ),
                                      InFileContent.size( ) ) ) {

                loadedImage->ImageCreateInfo.imageType     = VK_IMAGE_TYPE_2D;
                loadedImage->ImageCreateInfo.format        = VK_FORMAT_R8G8B8A8_UNORM;
                loadedImage->ImageCreateInfo.extent.width  = imageWidth;
                loadedImage->ImageCreateInfo.extent.height = imageHeight;
                loadedImage->ImageCreateInfo.extent.depth  = 1;
                loadedImage->ImageCreateInfo.mipLevels     = 1;
                loadedImage->ImageCreateInfo.arrayLayers   = 1;
                loadedImage->ImageCreateInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
                loadedImage->ImageCreateInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
                loadedImage->ImageCreateInfo.usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                loadedImage->ImageCreateInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
                loadedImage->ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

                loadedImage->ImgViewCreateInfo.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
                loadedImage->ImgViewCreateInfo.format                      = VK_FORMAT_R8G8B8A8_UNORM;
                loadedImage->ImgViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                loadedImage->ImgViewCreateInfo.subresourceRange.levelCount = 1;
                loadedImage->ImgViewCreateInfo.subresourceRange.layerCount = 1;

                imageBufferSuballocResult = pHostBufferPool->Suballocate( pImageBytes, imageWidth * imageHeight * 4 );
                lodepng_free( pImageBytes ); /* Free decoded PNG since it is no longer needed */

                bufferImageCopies.resize(1);
                auto& bufferImageCopy = bufferImageCopies.back();
                InitializeStruct(bufferImageCopy);

                bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                bufferImageCopy.imageSubresource.layerCount = 1;
                bufferImageCopy.imageExtent.width = imageWidth;
                bufferImageCopy.imageExtent.height = imageHeight;
                bufferImageCopy.imageExtent.depth = 1;
                bufferImageCopy.bufferOffset = imageBufferSuballocResult.DynamicOffset;
                bufferImageCopy.bufferImageHeight = 0; /* Tightly packed according to the imageExtent */
                bufferImageCopy.bufferRowLength = 0; /* Tightly packed according to the imageExtent */

                writeImageMemoryBarrier.dstAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;
                writeImageMemoryBarrier.oldLayout                   = VK_IMAGE_LAYOUT_UNDEFINED;
                writeImageMemoryBarrier.newLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                writeImageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                writeImageMemoryBarrier.subresourceRange.levelCount = 1;
                writeImageMemoryBarrier.subresourceRange.layerCount = 1;

                readImgMemoryBarrier.srcAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;
                readImgMemoryBarrier.dstAccessMask               = VK_ACCESS_SHADER_READ_BIT;
                readImgMemoryBarrier.oldLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                readImgMemoryBarrier.newLayout                   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                readImgMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                readImgMemoryBarrier.subresourceRange.levelCount = 1;
                readImgMemoryBarrier.subresourceRange.layerCount = 1;

                loadedImage->eImgLayout = readImgMemoryBarrier.newLayout;
            }
        } break;
    }

    pHostBufferPool->Flush( ); /* Unmap buffers and flush all memory ranges */

    VmaAllocationCreateInfo allocationCreateInfo = {};
    allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocationCreateInfo.flags = 0;

    if ( false == loadedImage->hImg.Recreate( pNode->hAllocator, loadedImage->ImageCreateInfo, allocationCreateInfo ) ) {
        return nullptr;
    }

    if ( bImgView ) {
        loadedImage->ImgViewCreateInfo.image = loadedImage->hImg.Handle.pImg;
        if ( false == loadedImage->hImgView.Recreate( *pNode, loadedImage->ImgViewCreateInfo ) ) {
            return nullptr;
        }
    }

    auto acquiredQueue = pNode->GetQueuePool( )->Acquire( false, VK_QUEUE_TRANSFER_BIT | VK_QUEUE_GRAPHICS_BIT, true );
    if ( nullptr == acquiredQueue.pQueue ) {
        while ( nullptr == acquiredQueue.pQueue ) {
            acquiredQueue = pNode->GetQueuePool( )->Acquire( false, VK_QUEUE_TRANSFER_BIT | VK_QUEUE_GRAPHICS_BIT, false );
        }
    }

    auto acquiredCmdBuffer = pNode->GetCommandBufferPool( )->Acquire( false, acquiredQueue.QueueFamilyId );
    assert( acquiredCmdBuffer.pCmdBuffer != nullptr );

    if ( VK_SUCCESS != CheckedCall( vkResetCommandPool( *pNode, acquiredCmdBuffer.pCmdPool, 0 ) ) ) {
        return nullptr;
    }

    VkCommandBufferBeginInfo cmdBufferBeginInfo;
    InitializeStruct( cmdBufferBeginInfo );
    cmdBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if ( VK_SUCCESS != CheckedCall( vkBeginCommandBuffer( acquiredCmdBuffer.pCmdBuffer, &cmdBufferBeginInfo ) ) ) {
        return nullptr;
    }

    writeImageMemoryBarrier.image               = loadedImage->hImg.Handle.pImg;
    writeImageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    writeImageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    vkCmdPipelineBarrier( acquiredCmdBuffer.pCmdBuffer,
                          VK_PIPELINE_STAGE_HOST_BIT,
                          VK_PIPELINE_STAGE_TRANSFER_BIT,
                          0,
                          0,
                          NULL,
                          0,
                          NULL,
                          1,
                          &writeImageMemoryBarrier);

    vkCmdCopyBufferToImage( acquiredCmdBuffer.pCmdBuffer,
                            imageBufferSuballocResult.DescriptorBufferInfo.buffer,
                            loadedImage->hImg.Handle.pImg,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            (uint32_t) bufferImageCopies.size( ),
                            bufferImageCopies.data( ) );

    readImgMemoryBarrier.image               = loadedImage->hImg.Handle.pImg;
    readImgMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    readImgMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    vkCmdPipelineBarrier( acquiredCmdBuffer.pCmdBuffer,
                          VK_PIPELINE_STAGE_TRANSFER_BIT,
                          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                          0,
                          0,
                          NULL,
                          0,
                          NULL,
                          1,
                          &readImgMemoryBarrier);

    vkEndCommandBuffer( acquiredCmdBuffer.pCmdBuffer );

    VkSubmitInfo submitInfo;
    InitializeStruct( submitInfo );
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &acquiredCmdBuffer.pCmdBuffer;

    /* Reset signaled fence */
    if ( VK_SUCCESS == vkGetFenceStatus( *pNode, acquiredQueue.pFence ) )
        if ( VK_SUCCESS != CheckedCall( vkResetFences( *pNode, 1, &acquiredQueue.pFence ) ) ) {
            return nullptr;
        }

    if ( VK_SUCCESS != CheckedCall( vkQueueSubmit( acquiredQueue.pQueue, 1, &submitInfo, acquiredQueue.pFence ) ) ) {
        return nullptr;
    }

    if ( bAwaitLoading ) {
        /* No need to pass fence to command buffer pool */
        acquiredCmdBuffer.pFence = nullptr;

        /* No need to pass queue info with the texture result */
        loadedImage->QueueId = -1;
        loadedImage->QueueFamilyId = -1;

        /* Ensure the image can be used right away */
        CheckedCall( vkWaitForFences( *pNode, 1, &acquiredQueue.pFence, true, UINT64_MAX ) );
    } else {
        /* Ensure the command buffer is synchronized */
        acquiredCmdBuffer.pFence = acquiredQueue.pFence;

        /* Ensure the image memory transfer can be synchronized */
        loadedImage->QueueId = acquiredQueue.QueueId;
        loadedImage->QueueFamilyId = acquiredQueue.QueueFamilyId;
    }

    pNode->GetCommandBufferPool( )->Release( acquiredCmdBuffer );
    pNode->GetQueuePool( )->Release( acquiredQueue );

    return std::move( loadedImage );
}

#undef lodepng_malloc
#undef lodepng_realloc
#undef lodepng_free

#pragma warning( push )
// '<<': result of 32-bit shift implicitly converted to 64 bits
#pragma warning( disable : 4334 )
#include <lodepng.cpp>
#pragma warning( pop )
