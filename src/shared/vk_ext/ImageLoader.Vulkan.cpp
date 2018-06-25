#include <BufferPools.Vulkan.h>
#include <ImageLoader.Vulkan.h>
#include <MemoryManager.h>
#include <QueuePools.Vulkan.h>
#include <TOneTimeCmdBufferSubmit.Vulkan.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#pragma warning(disable:4309)
#include <gli/gli.hpp>
#include <gli/generate_mipmaps.hpp>
#pragma warning(default:4309)

apemodevk::LoadedImage::~LoadedImage( ) {
    Destroy( );
}

void apemodevk::LoadedImage::Destroy( ) {
    apemodevk_memory_allocation_scope;

    hImgView.Destroy( );
    hImg.Destroy( );

    eImgLayout    = VK_IMAGE_LAYOUT_UNDEFINED;
    QueueId       = kInvalidIndex;
    QueueFamilyId = kInvalidIndex;
}

/**
 * Exactly the same, no need for explicit mapping.
 * But still decided to leave it as a function in case GLI stops maintain such compatibility.
 */
VkFormat ToImgFormat( const gli::format eTextureFormat ) {
    return static_cast< VkFormat >( eTextureFormat );
}

VkImageType ToImgType( const gli::target eTextureTarget ) {
    switch ( eTextureTarget ) {
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

VkImageViewType ToImgViewType( const gli::target eTextureTarget ) {
    switch ( eTextureTarget ) {
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

gli::texture DuplicateWithMipMaps( const gli::texture& originalTexture ) {
    apemodevk_memory_allocation_scope;

    if ( originalTexture.empty( ) ) {
        return gli::texture( );
    }

    const uint32_t maxExtent = std::max( std::max( originalTexture.extent( ).x, originalTexture.extent( ).y ), originalTexture.extent( ).z );
    const uint32_t maxLod    = static_cast< uint32_t >( std::floor( std::log2( maxExtent ) ) ) + 1;

    gli::texture duplicate( originalTexture.target( ),
                            originalTexture.format( ),
                            originalTexture.extent( ),
                            originalTexture.layers( ),
                            originalTexture.faces( ),
                            maxLod,
                            originalTexture.swizzles( ) );

    for ( gli::texture::size_type ll = 0; ll < duplicate.layers( ); ++ll )
        for ( gli::texture::size_type ff = 0; ff < duplicate.faces( ); ++ff )
            duplicate.copy( originalTexture, ll, ff, originalTexture.base_level( ), ll, ff, duplicate.base_level( ) );

    return duplicate;
}

gli::texture GenerateMipMaps( const gli::texture& texture ) {
    apemodevk_memory_allocation_scope;

    if ( texture.empty( ) ) {
        return gli::texture( );
    }

    switch ( texture.target( ) ) {
        case gli::TARGET_1D:
            return gli::generate_mipmaps( static_cast< const gli::texture1d& >( texture ), gli::FILTER_LINEAR );
        case gli::TARGET_1D_ARRAY:
            return gli::generate_mipmaps( static_cast< const gli::texture1d_array& >( texture ), gli::FILTER_LINEAR );
        case gli::TARGET_2D:
            return gli::generate_mipmaps( static_cast< const gli::texture2d& >( texture ), gli::FILTER_LINEAR );
        case gli::TARGET_2D_ARRAY:
            return gli::generate_mipmaps( static_cast< const gli::texture2d_array& >( texture ), gli::FILTER_LINEAR );
        case gli::TARGET_RECT:
            return gli::generate_mipmaps( static_cast< const gli::texture2d& >( texture ), gli::FILTER_LINEAR );
        case gli::TARGET_RECT_ARRAY:
            return gli::generate_mipmaps( static_cast< const gli::texture2d_array& >( texture ), gli::FILTER_LINEAR );
        case gli::TARGET_3D:
            return gli::generate_mipmaps( static_cast< const gli::texture3d& >( texture ), gli::FILTER_LINEAR );
        case gli::TARGET_CUBE:
            return gli::generate_mipmaps( static_cast< const gli::texture_cube& >( texture ), gli::FILTER_LINEAR );
        case gli::TARGET_CUBE_ARRAY:
            return gli::generate_mipmaps( static_cast< const gli::texture_cube_array& >( texture ), gli::FILTER_LINEAR );
    }

    assert( false );
    return gli::texture();
}

gli::texture LoadTexture( const uint8_t* pImageBytes, uint32_t imageWidth, uint32_t imageHeight ) {
    apemodevk_memory_allocation_scope;

    assert( pImageBytes && imageWidth && imageHeight );
    gli::texture texture =
        gli::texture2d( gli::format::FORMAT_RGBA8_UNORM_PACK8,
                        gli::extent2d( imageWidth, imageHeight ),
                        1,
                        gli::swizzles( gli::SWIZZLE_RED, gli::SWIZZLE_GREEN, gli::SWIZZLE_BLUE, gli::SWIZZLE_ALPHA ) );

    memcpy( texture.data( ), pImageBytes, imageWidth * imageHeight * 4 );
    return texture;
}

gli::texture LoadTexture( const uint8_t*                           pFileContent,
                          size_t                                   fileContentSize,
                          apemodevk::ImageLoader::EImageFileFormat eFileFormat ) {
    apemodevk_memory_allocation_scope;

    assert( pFileContent && fileContentSize );

    gli::texture texture;

    /**
     * @note All the data will be uploaded in switch cases,
     *       all the structures that depend on image type and dimensions
     *       will be filled in switch cases.
     **/
    switch ( eFileFormat ) {
        case apemodevk::ImageLoader::eImageFileFormat_DDS:
        case apemodevk::ImageLoader::eImageFileFormat_KTX: {
            texture = gli::load( (const char*) pFileContent, fileContentSize );
        } break;

        // case apemodevk::ImageLoader::eImageFileFormat_PNG:
        // case apemodevk::ImageLoader::eImageFileFormat_Undefined:
        default: {
            int imageWidth;
            int imageHeight;
            int componentsInFile;

            /* Load png file here from memory buffer */
            if ( stbi_uc* pImageBytes = stbi_load_from_memory( pFileContent, int( fileContentSize ), &imageWidth, &imageHeight, &componentsInFile, STBI_rgb_alpha ) ) {
                texture = LoadTexture( pImageBytes, imageWidth, imageHeight );
                stbi_image_free( pImageBytes );
            }
        } break;
    }

    return texture;
}

apemodevk::unique_ptr< apemodevk::LoadedImage > LoadImageFromGLITexture(
    apemodevk::GraphicsDevice*                 pNode,
    apemodevk::HostBufferPool*                 pHostBufferPool,
    gli::texture                               texture,
    const apemodevk::ImageLoader::LoadOptions& loadOptions ) {
    apemodevk_memory_allocation_scope;

    using namespace apemodevk;

    /* Check if the user needs mipmaps.
     * Note, that DDS and KTX files can contain mipmaps. */
    if ( !texture.empty( ) && loadOptions.bGenerateMipMaps && ( 1 == texture.levels( ) ) ) {
        /* Cannot generate mipmaps the data is compressed. */
        if ( !gli::is_compressed( texture.format( ) ) ) {
            const gli::texture dumplicateWithMipMaps = DuplicateWithMipMaps( texture );
            texture = GenerateMipMaps( dumplicateWithMipMaps );
        }
    }

    if ( texture.empty( ) ) {
        return nullptr;
    }

    auto loadedImage = apemodevk::make_unique< LoadedImage >( );
    if ( !loadedImage ) {
        return nullptr;
    }

    std::vector< VkBufferImageCopy > bufferImageCopies;
    HostBufferPool::SuballocResult   imageBufferSuballocResult;
    InitializeStruct( loadedImage->ImageCreateInfo );
    InitializeStruct( loadedImage->ImgViewCreateInfo );

    pHostBufferPool->Reset( );

    loadedImage->ImageCreateInfo.format        = ToImgFormat( texture.format( ) );
    loadedImage->ImageCreateInfo.imageType     = ToImgType( texture.target( ) );
    loadedImage->ImageCreateInfo.extent.width  = static_cast< uint32_t >( texture.extent( ).x );
    loadedImage->ImageCreateInfo.extent.height = static_cast< uint32_t >( texture.extent( ).y );
    loadedImage->ImageCreateInfo.extent.depth  = static_cast< uint32_t >( texture.extent( ).z );
    loadedImage->ImageCreateInfo.mipLevels     = static_cast< uint32_t >( texture.levels( ) );
    loadedImage->ImageCreateInfo.arrayLayers   = static_cast< uint32_t >( texture.faces( ) );
    loadedImage->ImageCreateInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    loadedImage->ImageCreateInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    loadedImage->ImageCreateInfo.usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    loadedImage->ImageCreateInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    loadedImage->ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

    loadedImage->ImgViewCreateInfo.flags                           = 0;
    loadedImage->ImgViewCreateInfo.format                          = ToImgFormat( texture.format( ) );
    loadedImage->ImgViewCreateInfo.viewType                        = ToImgViewType( texture.target( ) );
    loadedImage->ImgViewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    loadedImage->ImgViewCreateInfo.subresourceRange.levelCount     = static_cast< uint32_t >( texture.levels( ) );
    loadedImage->ImgViewCreateInfo.subresourceRange.layerCount     = static_cast< uint32_t >( texture.faces( ) );
    loadedImage->ImgViewCreateInfo.subresourceRange.baseMipLevel   = 0;
    loadedImage->ImgViewCreateInfo.subresourceRange.baseArrayLayer = 0;

    switch ( loadedImage->ImgViewCreateInfo.viewType ) {
        case VK_IMAGE_VIEW_TYPE_CUBE:
        case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY: {
            loadedImage->ImageCreateInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        } break;
        case VK_IMAGE_VIEW_TYPE_2D_ARRAY: {
            loadedImage->ImageCreateInfo.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;
        } break;

        default:
            break;
    }

    imageBufferSuballocResult = pHostBufferPool->Suballocate( texture.data( ), static_cast< uint32_t >( texture.size( ) ) );

    uint32_t bufferImageCopyIndex = 0;
    bufferImageCopies.resize( ( uint32_t )( texture.levels( ) * texture.faces( ) ) );

    for ( size_t mipLevel = 0; mipLevel < texture.levels( ); ++mipLevel ) {
        for ( size_t face = 0; face < texture.faces( ); ++face ) {
            apemodevk_memory_allocation_scope;

            auto& bufferImageCopy = bufferImageCopies[ bufferImageCopyIndex++ ];
            InitializeStruct( bufferImageCopy );

            uintptr_t    imgAddress    = (uintptr_t) texture.data( );
            uintptr_t    subImgAddress = (uintptr_t) texture.data( 0, face, mipLevel );
            VkDeviceSize imageOffset   = subImgAddress - imgAddress;
            VkDeviceSize bufferOffset  = imageBufferSuballocResult.DynamicOffset + imageOffset;

            bufferImageCopy.imageSubresource.baseArrayLayer = 0;
            bufferImageCopy.imageSubresource.mipLevel       = static_cast< uint32_t >( mipLevel );
            bufferImageCopy.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferImageCopy.imageSubresource.baseArrayLayer = static_cast< uint32_t >( face );
            bufferImageCopy.imageSubresource.layerCount     = 1;
            bufferImageCopy.imageExtent.width               = static_cast< uint32_t >( texture.extent( mipLevel ).x );
            bufferImageCopy.imageExtent.height              = static_cast< uint32_t >( texture.extent( mipLevel ).y );
            bufferImageCopy.imageExtent.depth               = static_cast< uint32_t >( texture.extent( mipLevel ).z );
            bufferImageCopy.bufferOffset                    = bufferOffset;
            bufferImageCopy.bufferImageHeight               = 0; /* Tightly packed according to the imageExtent */
            bufferImageCopy.bufferRowLength                 = 0; /* Tightly packed according to the imageExtent */
        }
    }

    texture = gli::texture( ); /* Free allocated CPU memory. */
    pHostBufferPool->Flush( ); /* Unmap buffers and flush all staging memory ranges */

    VmaAllocationCreateInfo allocationCreateInfo = {};
    allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocationCreateInfo.flags = 0;

    if ( false == loadedImage->hImg.Recreate( pNode->hAllocator, loadedImage->ImageCreateInfo, allocationCreateInfo ) ) {
        return nullptr;
    }

    if ( loadOptions.bImgView ) {
        loadedImage->ImgViewCreateInfo.image = loadedImage->hImg.Handle.pImg;
        if ( false == loadedImage->hImgView.Recreate( *pNode, loadedImage->ImgViewCreateInfo ) ) {
            return nullptr;
        }
    }

    const OneTimeCmdBufferSubmitResult imgLoadedResult =
    apemodevk::TOneTimeCmdBufferSubmit( pNode, 0, loadOptions.bAwaitLoading, [&]( VkCommandBuffer pCmdBuffer ) {
        VkImageMemoryBarrier writeImageMemoryBarrier;
        InitializeStruct( writeImageMemoryBarrier );

        writeImageMemoryBarrier.image                       = loadedImage->hImg.Handle.pImg;
        writeImageMemoryBarrier.dstAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;
        writeImageMemoryBarrier.oldLayout                   = VK_IMAGE_LAYOUT_UNDEFINED;
        writeImageMemoryBarrier.newLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        writeImageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        writeImageMemoryBarrier.subresourceRange.levelCount = static_cast< uint32_t >( texture.levels( ) );
        writeImageMemoryBarrier.subresourceRange.layerCount = static_cast< uint32_t >( texture.faces( ) );
        writeImageMemoryBarrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
        writeImageMemoryBarrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;

        vkCmdPipelineBarrier( pCmdBuffer,
                              VK_PIPELINE_STAGE_HOST_BIT,
                              VK_PIPELINE_STAGE_TRANSFER_BIT,
                              0,
                              0,
                              NULL,
                              0,
                              NULL,
                              1,
                              &writeImageMemoryBarrier );

        vkCmdCopyBufferToImage( pCmdBuffer,
                                imageBufferSuballocResult.DescriptorBufferInfo.buffer,
                                loadedImage->hImg.Handle.pImg,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                static_cast< uint32_t >( bufferImageCopies.size( ) ),
                                bufferImageCopies.data( ) );

        VkImageMemoryBarrier readImgMemoryBarrier;
        InitializeStruct( readImgMemoryBarrier );

        readImgMemoryBarrier.image                       = loadedImage->hImg.Handle.pImg;
        readImgMemoryBarrier.srcAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;
        readImgMemoryBarrier.dstAccessMask               = VK_ACCESS_SHADER_READ_BIT;
        readImgMemoryBarrier.oldLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        readImgMemoryBarrier.newLayout                   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        readImgMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        readImgMemoryBarrier.subresourceRange.levelCount = static_cast< uint32_t >( texture.levels( ) );
        readImgMemoryBarrier.subresourceRange.layerCount = static_cast< uint32_t >( texture.faces( ) );
        readImgMemoryBarrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
        readImgMemoryBarrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
        loadedImage->eImgLayout                          = readImgMemoryBarrier.newLayout;

        vkCmdPipelineBarrier( pCmdBuffer,
                              VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                              0,
                              0,
                              NULL,
                              0,
                              NULL,
                              1,
                              &readImgMemoryBarrier );

        return true;
    } );

    if ( Succeeded( imgLoadedResult ) ) {
        if ( !loadOptions.bAwaitLoading ) {
            loadedImage->QueueId       = imgLoadedResult.QueueId;
            loadedImage->QueueFamilyId = imgLoadedResult.QueueFamilyId;
        }

        return std::move( loadedImage );
    }

    return nullptr;
}

apemodevk::ImageLoader::~ImageLoader( ) {
    Destroy( );
}

bool apemodevk::ImageLoader::Recreate( GraphicsDevice* pInNode ) {
    apemodevk_memory_allocation_scope;

    pNode           = pInNode;
    pHostBufferPool = apemodevk_new HostBufferPool( );
    pHostBufferPool->Recreate( pNode, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, false );

    return true;
}

void apemodevk::ImageLoader::Destroy( ) {
    apemodevk_memory_allocation_scope;
    apemodevk_delete( pHostBufferPool );
}

apemodevk::unique_ptr< apemodevk::LoadedImage > apemodevk::ImageLoader::LoadImageFromFileData(
    const uint8_t* pFileContent, size_t fileContentSize, LoadOptions const& loadOptions ) {
    apemodevk_memory_allocation_scope;
    if ( !pFileContent || !fileContentSize ) {
        return nullptr;
    }

    gli::texture texture = LoadTexture( pFileContent, fileContentSize, loadOptions.eFileFormat );
    if ( texture.empty( ) ) {
        return nullptr;
    }

    return LoadImageFromGLITexture( pNode, pHostBufferPool, texture, loadOptions );
}

apemodevk::unique_ptr< apemodevk::LoadedImage > apemodevk::ImageLoader::LoadImageFromRawImgRGBA8(
    const uint8_t* pImageBytes, uint16_t imageWidth, uint16_t imageHeight, LoadOptions const& loadOptions ) {
    apemodevk_memory_allocation_scope;

    if ( !pImageBytes || !imageWidth || !imageHeight ) {
        return nullptr;
    }

    gli::texture texture = LoadTexture( pImageBytes, imageWidth, imageHeight );
    if ( texture.empty( ) ) {
        return nullptr;
    }

    return LoadImageFromGLITexture( pNode, pHostBufferPool, texture, loadOptions );
}
