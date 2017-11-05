#include <CommandQueue.Vulkan.h>
#include <GraphicsManager.KnownExtensions.Vulkan.h>
#include <Swapchain.Vulkan.h>

/// -------------------------------------------------------------------------------------------------------------------
/// Swapchain
/// -------------------------------------------------------------------------------------------------------------------


uint32_t const
apemodevk::Swapchain::kExtentMatchFullscreen = -1;
uint32_t const
apemodevk::Swapchain::kExtentMatchWindow     = 0;
uint64_t const
apemodevk::Swapchain::kMaxTimeout            = uint64_t( -1 );
uint32_t const
apemodevk::Swapchain::kMaxImgs               = 3;

apemodevk::Swapchain::Swapchain( ) {
}

apemodevk::Swapchain::~Swapchain( ) {
    for ( auto& hBuffer : hImgs )
        if ( hBuffer ) {
            apemodevk::TDispatchableHandle< VkImage > hImg;
            hImg.Deleter.hLogicalDevice = pDevice;
            hImg.Handle                 = hBuffer;
            hImg.Destroy( );
        }
}

bool apemodevk::Swapchain::ExtractSwapchainBuffers( VkImage * OutBufferImgs) {
    apemode_assert(hSwapchain.IsNotNull(), "Not initialized.");

    uint32_t OutSwapchainBufferCount = 0;
    if ( apemode_likely(  VK_SUCCESS == CheckedCall( vkGetSwapchainImagesKHR( pDevice, hSwapchain, &OutSwapchainBufferCount, nullptr ) ) ) ) {
        if (OutSwapchainBufferCount > kMaxImgs) {
            platform::DebugBreak();
            return false;
        }

        if ( apemode_likely( VK_SUCCESS == CheckedCall( vkGetSwapchainImagesKHR( pDevice, hSwapchain, &OutSwapchainBufferCount, OutBufferImgs ) ) ) )
            return true;
    }

    apemode_halt("vkGetSwapchainImagesKHR failed.");
    return false;

}


bool apemodevk::Swapchain::Recreate( VkDevice                   pInDevice,
                                     VkPhysicalDevice           pInPhysicalDevice,
                                     VkSurfaceKHR               pInSurface,
                                     uint32_t                   InImgCount,
                                     uint32_t                   DesiredColorWidth,
                                     uint32_t                   DesiredColorHeight,
                                     VkFormat                   eColorFormat,
                                     VkColorSpaceKHR            eColorSpace,
                                     VkSurfaceTransformFlagsKHR eSurfaceTransform,
                                     VkPresentModeKHR           ePresentMode ) {
    pSurface        = pInSurface;
    pDevice         = pInDevice;
    pPhysicalDevice = pInPhysicalDevice;

    for ( uint32_t i = 0; i < ImgCount; ++i ) {
        hImgViews[ i ].Destroy( );
    }

    // Determine the number of VkImage's to use in the swap chain.
    // We desire to own only 1 image at a time, besides the
    // images being displayed and queued for display.

    ImgCount         = InImgCount;
    ImgExtent.width  = DesiredColorWidth;
    ImgExtent.height = DesiredColorHeight;

    TInfoStruct< VkSwapchainCreateInfoKHR > SwapchainDesc;
    SwapchainDesc->surface          = pInSurface;
    SwapchainDesc->minImageCount    = ImgCount;
    SwapchainDesc->imageFormat      = eColorFormat;
    SwapchainDesc->imageColorSpace  = eColorSpace;
    SwapchainDesc->imageExtent      = ImgExtent;
    SwapchainDesc->imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    SwapchainDesc->preTransform     = static_cast< VkSurfaceTransformFlagBitsKHR >( eSurfaceTransform );
    SwapchainDesc->compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    SwapchainDesc->imageArrayLayers = 1;
    SwapchainDesc->imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    SwapchainDesc->presentMode      = ePresentMode;
    SwapchainDesc->oldSwapchain     = hSwapchain;
    SwapchainDesc->clipped          = true;

    if ( !hSwapchain.Recreate( pInDevice, SwapchainDesc ) ) {
        apemode_halt( "Failed to create swapchain." );
        return false;
    }

    if ( !ExtractSwapchainBuffers( hImgs ) ) {
        apemode_halt( "Failed to extract swapchain buffers." );
        return false;
    }

    TInfoStruct< VkImageViewCreateInfo > imgViewCreateInfo;
    imgViewCreateInfo->viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    imgViewCreateInfo->format                          = eColorFormat;
    imgViewCreateInfo->components.r                    = VK_COMPONENT_SWIZZLE_R;
    imgViewCreateInfo->components.g                    = VK_COMPONENT_SWIZZLE_G;
    imgViewCreateInfo->components.b                    = VK_COMPONENT_SWIZZLE_B;
    imgViewCreateInfo->components.a                    = VK_COMPONENT_SWIZZLE_A;
    imgViewCreateInfo->subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    imgViewCreateInfo->subresourceRange.baseArrayLayer = 0;
    imgViewCreateInfo->subresourceRange.baseMipLevel   = 0;
    imgViewCreateInfo->subresourceRange.layerCount     = 1;
    imgViewCreateInfo->subresourceRange.levelCount     = 1;

    for ( uint32_t i = 0; i < ImgCount; ++i ) {
        imgViewCreateInfo->image = hImgs[ i ];
        hImgViews[ i ].Recreate( pInDevice, imgViewCreateInfo );
    }

    /* TODO: Warning after resizing, consider changing image layouts manually. */
    return true;
}

uint32_t apemodevk::Swapchain::GetBufferCount( ) const {
    return ImgCount;
}
bool apemodevk::Surface::SetSurfaceProperties( ) {
    if ( hSurface.IsNotNull( ) ) {
        uint32_t SurfaceFormatCount = 0;
        if ( VK_SUCCESS == CheckedCall( vkGetPhysicalDeviceSurfaceFormatsKHR( pPhysicalDevice, hSurface, &SurfaceFormatCount, nullptr ) ) ) {

            std::vector< VkSurfaceFormatKHR > SurfaceFormats( SurfaceFormatCount );
            if ( VK_SUCCESS == CheckedCall( vkGetPhysicalDeviceSurfaceFormatsKHR( pPhysicalDevice, hSurface, &SurfaceFormatCount, SurfaceFormats.data( ) ) ) ) {
                const bool bCanChooseAny = SurfaceFormatCount == 1 && SurfaceFormats[ 0 ].format == VK_FORMAT_UNDEFINED;
                eColorFormat             = bCanChooseAny ? VK_FORMAT_B8G8R8A8_UNORM : SurfaceFormats[ 0 ].format;
                eColorSpace              = SurfaceFormats[ 0 ].colorSpace;
            }
        }

        // We fall back to FIFO which is always available.
        ePresentMode = VK_PRESENT_MODE_FIFO_KHR;

        uint32_t PresentModeCount = 0;
        if ( VK_SUCCESS == CheckedCall( vkGetPhysicalDeviceSurfacePresentModesKHR( pPhysicalDevice, hSurface, &PresentModeCount, nullptr ) ) ) {

            std::vector< VkPresentModeKHR > PresentModes;
            PresentModes.resize( PresentModeCount );

            if ( VK_SUCCESS == CheckedCall( vkGetPhysicalDeviceSurfacePresentModesKHR( pPhysicalDevice, hSurface, &PresentModeCount, PresentModes.data( ) ) ) ) {
                for ( auto i = 0u; i < PresentModeCount; i++ ) {
                    auto& CurrentPresentMode = PresentModes[ i ];
                    if ( CurrentPresentMode == VK_PRESENT_MODE_MAILBOX_KHR ) {
                        // If mailbox mode is available, use it, as is the lowest-latency non- tearing mode.
                        ePresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                        break;
                    }

                    // If not, try IMMEDIATE which will usually be available, and is fastest (though it tears).
                    if ( ( ePresentMode != VK_PRESENT_MODE_MAILBOX_KHR ) &&
                         ( CurrentPresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR ) )
                        ePresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                }
            }

            if ( VK_SUCCESS != CheckedCall( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( pPhysicalDevice, hSurface, &SurfaceCaps ) ) ) {
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

#ifdef _WINDOWS_
bool apemodevk::Surface::Recreate( VkPhysicalDevice pInPhysicalDevice, VkInstance pInInstance, HINSTANCE hInstance, HWND hWindow ) {
    pInstance       = pInInstance;
    pPhysicalDevice = pInPhysicalDevice;

    assert( nullptr != pInstance );
    assert( nullptr != pPhysicalDevice );

    TInfoStruct< VkWin32SurfaceCreateInfoKHR > SurfaceDesc;
    SurfaceDesc->hwnd      = hWindow;
    SurfaceDesc->hinstance = hInstance;

    return hSurface.Recreate( pInInstance, SurfaceDesc ) && SetSurfaceProperties( );
}
#endif

#ifdef X_PROTOCOL
bool apemodevk::Surface::Recreate( VkPhysicalDevice pInPhysicalDevice,
                                   VkInstance       pInInstance,
                                   Display*         pDisplayX11,
                                   Window           pWindowX11 ) {
    pInstance       = pInInstance;
    pPhysicalDevice = pInPhysicalDevice;

    TInfoStruct< VkXlibSurfaceCreateInfoKHR > SurfaceDesc;
    SurfaceDesc->window = pWindowX11;
    SurfaceDesc->dpy    = pDisplayX11;

    return hSurface.Recreate( pInInstance, SurfaceDesc ) && SetSurfaceProperties( );
}
#endif
