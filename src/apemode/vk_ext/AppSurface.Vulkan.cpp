

#include "AppSurface.Vulkan.h"
#include <algorithm>
//#include <apemode/platform/memory/MemoryManager.h>

apemodevk::AppSurface::AppSurface( ) {
}

apemodevk::AppSurface::~AppSurface( ) {
}

//// clang-format off
//struct apemode::GraphicsAllocator : apemodevk::GraphicsManager::IAllocator {
//    void* Malloc( size_t             size,
//                  size_t             alignment,
//                  const char*        sourceFile,
//                  const unsigned int sourceLine,
//                  const char*        sourceFunc ) override {
//        return apemode::platform::allocate( size, alignment, sourceFile, sourceLine, sourceFunc );
//    }
//
//    void* Realloc( void*              pOriginal,
//                   size_t             size,
//                   size_t             alignment,
//                   const char*        sourceFile,
//                   const unsigned int sourceLine,
//                   const char*        sourceFunc ) override {
//        return apemode::platform::reallocate( pOriginal, size, alignment, sourceFile, sourceLine, sourceFunc );
//    }
//
//    void Free( void*              pMemory,
//               const char*        sourceFile,
//               const unsigned int sourceLine,
//               const char*        sourceFunc ) override {
//        return apemode::platform::deallocate( pMemory, sourceFile, sourceLine, sourceFunc );
//    }
//
//    void GetPrevMemoryAllocationScope( const char*&  pszPrevSourceFile,
//                                       unsigned int& prevSourceLine,
//                                       const char*&  pszPrevSourceFunc ) const override {
//        apemode::platform::GetPrevMemoryAllocationScope( pszPrevSourceFile, prevSourceLine, pszPrevSourceFunc );
//    }
//
//    void StartMemoryAllocationScope( const char*        pszSourceFile,
//                                     const unsigned int sourceLine,
//                                     const char*        pszSourceFunc ) const override {
//        apemode::platform::StartMemoryAllocationScope( pszSourceFile, sourceLine, pszSourceFunc );
//    }
//
//    void EndMemoryAllocationScope( const char*        pszPrevSourceFile,
//                                   const unsigned int prevSourceLine,
//                                   const char*        pszPrevSourceFunc ) const override {
//        apemode::platform::EndMemoryAllocationScope( pszPrevSourceFile, prevSourceLine, pszPrevSourceFunc );
//    }
//};
//
//// clang-format on
//
//struct apemode::GraphicsLogger : apemodevk::GraphicsManager::ILogger {
//    //spdlog::logger* pLogger = apemode::AppState::Get( )->GetLogger( );
//    void Log( const LogLevel eLevel, const char* pszMsg ) override {
//        //pLogger->log( static_cast< spdlog::level::level_enum >( eLevel ), pszMsg );
//    }
//};

// bool apemodevk::AppSurface::Initialize( uint32_t width, uint32_t height, const char* name )
bool apemodevk::AppSurface::Initialize( const PlatformSurface*                  pPlatformSurface,
                                        apemodevk::GraphicsManager::ILogger*    pGraphicsLogger,
                                        apemodevk::GraphicsManager::IAllocator* pGraphicsAllocator,
                                        const char**                            /*ppszLayers*/,
                                        size_t                                  requiredLayerCount,
                                        size_t                                  optionalLayerCount,
                                        const char**                            /*ppszExtensions*/,
                                        size_t                                  requiredExtensionCount,
                                        size_t                                  optionalExtensionCount ) {
    apemodevk_memory_allocation_scope;

    //LogInfo( "AppSurface: Initializing." );

    uint32_t graphicsManagerFlags = 0;

#ifdef _DEBUG
    graphicsManagerFlags |= apemodevk::GraphicsManager::kEnableValidation;
#endif

    const char* ppszLayers[ 3 ] = {nullptr};
    size_t layerCount = 0;

    const char** ppszExtensions = nullptr;
    size_t extentionCount = 0;

#ifdef __APPLE__
    graphicsManagerFlags &= ~apemodevk::GraphicsManager::kEnableValidation;
    ppszLayers[ layerCount ] = "MoltenVK";
    ++layerCount;
#else

    /*if ( TGetOption< bool >( "renderdoc", false ) ) {
        ppszLayers[ layerCount ] = "VK_LAYER_RENDERDOC_Capture";
        graphicsManagerFlags = 0;
        ++layerCount;
    }

    if ( TGetOption< bool >( "vktrace", false ) ) {
        ppszLayers[ layerCount ] = "VK_LAYER_LUNARG_vktrace";
        graphicsManagerFlags = 0;
        ++layerCount;
    }

    if ( TGetOption< bool >( "vkapidump", false ) ) {
        ppszLayers[ layerCount ] = "VK_LAYER_LUNARG_api_dump";
        graphicsManagerFlags = 0;
        ++layerCount;
    }*/

#endif

    Manager.reset( apemodevk::CreateGraphicsManager( graphicsManagerFlags,
                                                     pGraphicsAllocator,
                                                     pGraphicsLogger,
                                                     "Viewer",
                                                     "VkApeEngine",
                                                     ppszLayers,
                                                     layerCount,
                                                     ppszExtensions,
                                                     extentionCount ) );
    if ( !Manager ) {
        return false;
    }

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
    if ( currentExtent.width == uint32_t( -1 ) || currentExtent.height != uint32_t( -1 ) ) {
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
    apemodevk_memory_allocation_scope;

    PresentQueueFamilyIds.clear( );

    Surface.Destroy( );
    Swapchain.Destroy( );
    Node.Destroy( );
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
