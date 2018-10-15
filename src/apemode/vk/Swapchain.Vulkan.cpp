#include <Platform.Vulkan.h>
#include <Swapchain.Vulkan.h>

/// -------------------------------------------------------------------------------------------------------------------
/// Swapchain
/// -------------------------------------------------------------------------------------------------------------------

uint32_t const apemodevk::Swapchain::kExtentMatchFullscreen = -1;
uint32_t const apemodevk::Swapchain::kExtentMatchWindow     = 0;
uint64_t const apemodevk::Swapchain::kMaxTimeout            = uint64_t( -1 );
uint32_t const apemodevk::Swapchain::kMaxImgs               = 3;

apemodevk::Surface::~Surface( ) {
    Destroy( );
}

void apemodevk::Surface::Destroy( ) {
    hSurface.Destroy( );
}

apemodevk::Swapchain::Swapchain( ) {
}

apemodevk::Swapchain::~Swapchain( ) {
    Destroy( );
}

void apemodevk::Swapchain::Destroy( ) {
    if ( pNode ) {
        pNode = nullptr;

        for ( auto& buffer : Buffers )
            buffer.hImgView.Destroy( );
        apemodevk::vector< Buffer >( ).swap( Buffers );

        hSwapchain.Destroy( );
    }
}

bool apemodevk::Swapchain::ExtractSwapchainBuffers( VkImage* pOutBuffers ) {
    apemodevk_memory_allocation_scope;
    apemode_assert( hSwapchain.IsNotNull( ), "Not initialized." );

    uint32_t swapchainBufferCount = 0;
    if ( apemode_likely( VK_SUCCESS == CheckedResult( vkGetSwapchainImagesKHR( pNode->hLogicalDevice, hSwapchain, &swapchainBufferCount, nullptr ) ) ) ) {
        if ( swapchainBufferCount > kMaxImgs ) {
            platform::DebugBreak( );
            return false;
        }

        if ( apemode_likely( VK_SUCCESS == CheckedResult( vkGetSwapchainImagesKHR( pNode->hLogicalDevice, hSwapchain, &swapchainBufferCount, pOutBuffers ) ) ) )
            return true;
    }

    apemode_halt("vkGetSwapchainImagesKHR failed.");
    return false;

}

bool apemodevk::Swapchain::Recreate( apemodevk::GraphicsDevice* pInNode,
                                     apemodevk::Surface*        pInSurface,
                                     uint32_t                   desiredImgCount,
                                     VkExtent2D                 desiredImgExtent,
                                     VkFormat                   eColorFormat,
                                     VkColorSpaceKHR            eColorSpace,
                                     VkSurfaceTransformFlagsKHR eSurfaceTransform,
                                     VkPresentModeKHR           ePresentMode ) {
    apemodevk_memory_allocation_scope;

    pNode    = pInNode;
    pSurface = pInSurface;

    for ( auto& buffer : Buffers )
        buffer.hImgView.Destroy( );

    uint32_t bufferCount = desiredImgCount;
    {
        bufferCount = eastl::max( bufferCount, pSurface->SurfaceCaps.minImageCount );
        bufferCount = eastl::min( bufferCount, pSurface->SurfaceCaps.maxImageCount );

        if ( bufferCount != desiredImgCount ) {
            platform::LogFmt( platform::LogLevel::Warn,
                              "Desired swapchain image count does not comply with surface capabilities: "
                              "desired=%u and actual=%.",
                              desiredImgCount,
                              bufferCount );
        }
    }

    // Determine the number of VkImage's to use in the swap chain.
    // We desire to own only 1 image at a time, besides the
    // images being displayed and queued for display.

    ImgExtent = desiredImgExtent;

    VkSwapchainCreateInfoKHR swapchainCreateInfoKHR;
    InitializeStruct( swapchainCreateInfoKHR );

    swapchainCreateInfoKHR.surface          = pInSurface->hSurface;
    swapchainCreateInfoKHR.minImageCount    = bufferCount;
    swapchainCreateInfoKHR.imageFormat      = eColorFormat;
    swapchainCreateInfoKHR.imageColorSpace  = eColorSpace;
    swapchainCreateInfoKHR.imageExtent      = ImgExtent;
    swapchainCreateInfoKHR.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfoKHR.preTransform     = static_cast< VkSurfaceTransformFlagBitsKHR >( eSurfaceTransform );
    swapchainCreateInfoKHR.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfoKHR.imageArrayLayers = 1;
    swapchainCreateInfoKHR.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfoKHR.presentMode      = ePresentMode;
    swapchainCreateInfoKHR.oldSwapchain     = hSwapchain;
    swapchainCreateInfoKHR.clipped          = true;

    if ( !hSwapchain.Recreate( pNode->hLogicalDevice, swapchainCreateInfoKHR ) ) {
        platform::LogFmt( platform::LogLevel::Err, "Failed to create swapchain." );
        return false;
    }

    apemodevk::vector< VkImage > hhBuffers;
    hhBuffers.resize( bufferCount );

    if ( !ExtractSwapchainBuffers( hhBuffers.data( ) ) ) {
        platform::LogFmt( platform::LogLevel::Err, "Failed to extract swapchain buffers." );
        return false;
    }

    VkImageViewCreateInfo imageViewCreateInfo;
    InitializeStruct( imageViewCreateInfo );

    imageViewCreateInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format                          = eColorFormat;
    imageViewCreateInfo.components.r                    = VK_COMPONENT_SWIZZLE_R;
    imageViewCreateInfo.components.g                    = VK_COMPONENT_SWIZZLE_G;
    imageViewCreateInfo.components.b                    = VK_COMPONENT_SWIZZLE_B;
    imageViewCreateInfo.components.a                    = VK_COMPONENT_SWIZZLE_A;
    imageViewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
    imageViewCreateInfo.subresourceRange.layerCount     = 1;
    imageViewCreateInfo.subresourceRange.levelCount     = 1;

    Buffers.resize( bufferCount );

    for ( uint32_t i = 0; i < bufferCount; ++i ) {
        imageViewCreateInfo.image = hhBuffers[ i ];

        if ( !Buffers[ i ].hImgView.Recreate( pNode->hLogicalDevice, imageViewCreateInfo ) ) {
            platform::LogFmt( platform::LogLevel::Err, "Failed to create swapchain image view." );
            return false;
        }

        Buffers[ i ].hImg = imageViewCreateInfo.image;
    }

    /* TODO: Warning after resizing, consider changing image layouts manually. */
    return true;
}

uint32_t apemodevk::Swapchain::GetBufferCount( ) const {
    return static_cast< uint32_t >( Buffers.size( ) );
}

bool apemodevk::Surface::SetSurfaceProperties( ) {
    apemodevk_memory_allocation_scope;

    if ( hSurface.IsNotNull( ) ) {
        uint32_t surfaceFormatCount = 0;
        if ( VK_SUCCESS == CheckedResult( vkGetPhysicalDeviceSurfaceFormatsKHR( pPhysicalDevice, hSurface, &surfaceFormatCount, nullptr ) ) ) {

            vector< VkSurfaceFormatKHR > SurfaceFormats( surfaceFormatCount );
            if ( VK_SUCCESS == CheckedResult( vkGetPhysicalDeviceSurfaceFormatsKHR( pPhysicalDevice, hSurface, &surfaceFormatCount, SurfaceFormats.data( ) ) ) ) {
                const bool bCanChooseAny = surfaceFormatCount == 1 && SurfaceFormats[ 0 ].format == VK_FORMAT_UNDEFINED;
                eColorFormat             = bCanChooseAny ? VK_FORMAT_B8G8R8A8_UNORM : SurfaceFormats[ 0 ].format;
                eColorSpace              = SurfaceFormats[ 0 ].colorSpace;
            }
        }

        // We fall back to FIFO which is always available.
        ePresentMode = VK_PRESENT_MODE_FIFO_KHR;

        uint32_t presentModeCount = 0;
        if ( VK_SUCCESS == CheckedResult( vkGetPhysicalDeviceSurfacePresentModesKHR( pPhysicalDevice, hSurface, &presentModeCount, nullptr ) ) ) {

            vector< VkPresentModeKHR > presentModes;
            presentModes.resize( presentModeCount );

            if ( VK_SUCCESS == CheckedResult( vkGetPhysicalDeviceSurfacePresentModesKHR( pPhysicalDevice, hSurface, &presentModeCount, presentModes.data( ) ) ) ) {
                for ( auto i = 0u; i < presentModeCount; i++ ) {
                    auto& currentPresentMode = presentModes[ i ];
                    if ( currentPresentMode == VK_PRESENT_MODE_MAILBOX_KHR ) {
                        // If mailbox mode is available, use it, as is the lowest-latency non- tearing mode.
                        ePresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                        break;
                    }

                    // If not, try IMMEDIATE which will usually be available, and is fastest (though it tears).
                    if ( ( ePresentMode != VK_PRESENT_MODE_MAILBOX_KHR ) &&
                         ( currentPresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR ) )
                        ePresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                }
            }

            if ( VK_SUCCESS != CheckedResult( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( pPhysicalDevice, hSurface, &SurfaceCaps ) ) ) {
                apemode_halt( "vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed." );
                return false;
            }

            const bool bSurfaceSupportsIdentity = apemodevk::HasFlagEq( SurfaceCaps.supportedTransforms, VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR );
            eSurfaceTransform = bSurfaceSupportsIdentity ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : SurfaceCaps.currentTransform;

            return true;
        }
    }

    platform::DebugBreak( );
    return false;
}

