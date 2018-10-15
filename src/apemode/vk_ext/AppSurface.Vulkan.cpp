

#include "AppSurface.Vulkan.h"
#include <algorithm>

apemodevk::AppSurface::AppSurface( ) {
}

apemodevk::AppSurface::~AppSurface( ) {
    if ( GetGraphicsManager( ) ) {
        Finalize( );
    }
}

bool apemodevk::AppSurface::Initialize( const PlatformSurface*                  pPlatformSurface,
                                        apemodevk::GraphicsManager::ILogger*    pGraphicsLogger,
                                        apemodevk::GraphicsManager::IAllocator* pGraphicsAllocator,
                                        const char**                            ppszLayers,
                                        size_t                                  requiredLayerCount,
                                        size_t                                  optionalLayerCount,
                                        const char**                            ppszExtensions,
                                        size_t                                  requiredExtensionCount,
                                        size_t                                  optionalExtensionCount ) {

    uint32_t graphicsManagerFlags = 0;

#ifndef __APPLE__
#ifdef _DEBUG
    graphicsManagerFlags |= apemodevk::GraphicsManager::kEnableValidation;
#endif
#endif

    Manager = apemodevk::CreateGraphicsManager( graphicsManagerFlags,
                                                pGraphicsAllocator,
                                                pGraphicsLogger,
                                                "Viewer",
                                                "VkApeEngine",
                                                ppszLayers,
                                                requiredLayerCount,
                                                ppszExtensions,
                                                requiredExtensionCount );
    if ( !Manager ) {
        return false;
    }

    apemodevk_memory_allocation_scope;
    apemodevk::platform::Log( platform::Info, "apemodevk::AppSurface::Initialize" );

    if ( !Node.RecreateResourcesFor( 0, Manager->ppAdapters[ 0 ], nullptr, 0, nullptr, 0 ) ) {
        return false;
    }

#if defined( VK_USE_PLATFORM_WIN32_KHR ) && VK_USE_PLATFORM_WIN32_KHR
    Surface.Recreate( Node.pPhysicalDevice, Manager->hInstance, pPlatformSurface->hInstance, pPlatformSurface->hWnd );
#endif
#if defined( VK_USE_PLATFORM_XLIB_KHR ) && VK_USE_PLATFORM_XLIB_KHR
    Surface.Recreate( Node.pPhysicalDevice, Manager->hInstance, pPlatformSurface->pDisplayX11, pPlatformSurface->pWindowX11 );
#endif
#ifdef defined( VK_USE_PLATFORM_IOS_MVK ) && VK_USE_PLATFORM_IOS_MVK
    Surface.Recreate( Node.pPhysicalDevice, Manager->hInstance, pPlatformSurface->pViewIOS );
#endif
#ifdef defined( VK_USE_PLATFORM_MACOS_MVK ) && VK_USE_PLATFORM_MACOS_MVK
    Surface.Recreate( Node.pPhysicalDevice, Manager->hInstance, pPlatformSurface->pViewMacOS );
#endif
#ifdef defined( VK_USE_PLATFORM_ANDROID_KHR ) && VK_USE_PLATFORM_ANDROID_KHR
    Surface.Recreate( Node.pPhysicalDevice, Manager->hInstance, pPlatformSurface->pANativeWindow );
#endif

    uint32_t queueFamilyIndex = 0;
    apemodevk::QueueFamilyPool* queueFamilyPool  = nullptr;

    while ( auto currentQueueFamilyPool = Node.GetQueuePool( )->GetPool( queueFamilyIndex++ ) ) {
        if ( currentQueueFamilyPool->SupportsPresenting( Surface.hSurface ) ) {
            PresentQueueFamilyIds.push_back( queueFamilyIndex - 1 );
            break;
        }
    }

    // Determine the number of VkImage's to use in the swap chain.
    // We desire to own only 1 image at a time, besides the images being displayed and queued for display.
    uint32_t ImgCount = std::min< uint32_t >( apemodevk::Swapchain::kMaxImgs, Surface.SurfaceCaps.minImageCount + 1 );
    if ( ( Surface.SurfaceCaps.maxImageCount > 0 ) && ( Surface.SurfaceCaps.maxImageCount < ImgCount ) ) {

        // Application must settle for fewer images than desired.
        ImgCount = Surface.SurfaceCaps.maxImageCount;
    }

    VkExtent2D currentExtent = Surface.SurfaceCaps.currentExtent;
    if ( currentExtent.width == uint32_t( -1 ) || currentExtent.height == uint32_t( -1 ) ) {
        currentExtent = pPlatformSurface->OverrideExtent;

        if ( currentExtent.width < Surface.SurfaceCaps.minImageExtent.width ) {
            currentExtent.width = Surface.SurfaceCaps.minImageExtent.width;
        } else if ( currentExtent.width > Surface.SurfaceCaps.maxImageExtent.width ) {
            currentExtent.width = Surface.SurfaceCaps.maxImageExtent.width;
        }

        if ( currentExtent.height < Surface.SurfaceCaps.minImageExtent.height ) {
            currentExtent.height = Surface.SurfaceCaps.minImageExtent.height;
        } else if ( currentExtent.height > Surface.SurfaceCaps.maxImageExtent.height ) {
            currentExtent.height = Surface.SurfaceCaps.maxImageExtent.height;
        }
    }

    if ( !Swapchain.Recreate( &Node,
                              &Surface,
                              ImgCount,
                              currentExtent,
                              Surface.eColorFormat,
                              Surface.eColorSpace,
                              Surface.eSurfaceTransform,
                              Surface.ePresentMode ) ) {
        return false;
    }

    return true;
}

void apemodevk::AppSurface::Finalize( ) {
    {
        apemodevk_memory_allocation_scope;
        Surface.Destroy( );
        Swapchain.Destroy( );
        Node.Destroy( );
        apemodevk::vector< uint32_t >( ).swap( PresentQueueFamilyIds );
    }

    DestroyGraphicsManager( );
}

bool apemodevk::AppSurface::Resize( const VkExtent2D extent ) {
    if ( extent.width != Swapchain.ImgExtent.width || extent.height != Swapchain.ImgExtent.height ) {
        apemodevk_memory_allocation_scope;
        apemodevk::CheckedResult( vkDeviceWaitIdle( Node ) );

        const bool bResized = Swapchain.Recreate( &Node,
                                                  &Surface,
                                                  Swapchain.GetBufferCount( ),
                                                  extent,
                                                  Surface.eColorFormat,
                                                  Surface.eColorSpace,
                                                  Surface.eSurfaceTransform,
                                                  Surface.ePresentMode );
        assert( bResized );
        return bResized;
    }

    return false;
}
