

#include <AppState.h>
#include <AppSurfaceSdlVk.h>
#include <SceneRendererVk.h>
#include <MemoryManager.h>

apemode::AppSurfaceSdlVk::AppSurfaceSdlVk( ) {
    Impl = kAppSurfaceImpl_SdlVk;
}

apemode::AppSurfaceSdlVk::~AppSurfaceSdlVk( ) {
    Finalize( );
}

// clang-format off
struct apemode::GraphicsAllocator : apemodevk::GraphicsManager::IAllocator {
    void* Malloc( size_t             size,
                  size_t             alignment,
                  const char*        sourceFile,
                  const unsigned int sourceLine,
                  const char*        sourceFunc ) override {
        return apemode::allocate( size, alignment, sourceFile, sourceLine, sourceFunc );
    }

    void* Realloc( void*              pOriginal,
                   size_t             size,
                   size_t             alignment,
                   const char*        sourceFile,
                   const unsigned int sourceLine,
                   const char*        sourceFunc ) override {
        return apemode::reallocate( pOriginal, size, alignment, sourceFile, sourceLine, sourceFunc );
    }

    void Free( void*              pMemory,
               const char*        sourceFile,
               const unsigned int sourceLine,
               const char*        sourceFunc ) override {
        return apemode::deallocate( pMemory, sourceFile, sourceLine, sourceFunc );
    }

    void GetPrevMemoryAllocationScope( const char*&  pszPrevSourceFile,
                                       unsigned int& prevSourceLine,
                                       const char*&  pszPrevSourceFunc ) const override {
        apemode::GetPrevMemoryAllocationScope( pszPrevSourceFile, prevSourceLine, pszPrevSourceFunc );
    }

    void StartMemoryAllocationScope( const char*        pszSourceFile,
                                     const unsigned int sourceLine,
                                     const char*        pszSourceFunc ) const override {
        apemode::StartMemoryAllocationScope( pszSourceFile, sourceLine, pszSourceFunc );
    }

    void EndMemoryAllocationScope( const char*        pszPrevSourceFile,
                                   const unsigned int prevSourceLine,
                                   const char*        pszPrevSourceFunc ) const override {
        apemode::EndMemoryAllocationScope( pszPrevSourceFile, prevSourceLine, pszPrevSourceFunc );
    }
};

// clang-format on

struct apemode::GraphicsLogger : apemodevk::GraphicsManager::ILogger {
    spdlog::logger* pLogger = apemode::AppState::Get( )->GetLogger( );
    void Log( const LogLevel eLevel, const char* pszMsg ) override {
        pLogger->log( static_cast< spdlog::level::level_enum >( eLevel ), pszMsg );
    }
};

bool apemode::AppSurfaceSdlVk::Initialize( uint32_t width, uint32_t height, const char* name ) {
    apemode_memory_allocation_scope;

    LogInfo( "AppSurfaceSdlVk: Initializing." );

    if ( !AppSurfaceSdlBase::Initialize( width, height, name ) ) {
        return false;
    }

    uint32_t graphicsManagerFlags = 0;

#ifdef _DEBUG
    graphicsManagerFlags |= apemodevk::GraphicsManager::kEnableValidation;
#endif

    const char* ppszLayers[ 3 ] = {nullptr};
    size_t layerCount = 0;

    const char** ppszExtensions = nullptr;
    size_t extentionCount = 0;

    if ( TGetOption< bool >( "renderdoc", false ) ) {
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
    }

    Logger = apemode::make_unique< GraphicsLogger >( );
    Alloc  = apemode::make_unique< GraphicsAllocator >( );

    Manager.reset( apemodevk::CreateGraphicsManager( graphicsManagerFlags,
                                                     Alloc.get( ),
                                                     Logger.get( ),
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

#ifdef VK_USE_PLATFORM_XLIB_KHR
    Surface.Recreate( Node.pPhysicalDevice, Manager->hInstance, pDisplayX11, pWindowX11 );
#elif VK_USE_PLATFORM_WIN32_KHR
    Surface.Recreate( Node.pPhysicalDevice, Manager->hInstance, hInstance, hWnd );
#elif VK_USE_PLATFORM_MACOS_MVK || VK_USE_PLATFORM_IOS_MVK
    Surface.Recreate( Node.pPhysicalDevice, Manager->hInstance, pView );
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

    VkExtent2D currentExtent = {0, 0};

    if ( Surface.SurfaceCaps.currentExtent.width == apemodevk::Swapchain::kExtentMatchFullscreen &&
         Surface.SurfaceCaps.currentExtent.height == apemodevk::Swapchain::kExtentMatchFullscreen ) {

        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        apemode_assert( bIsDefined, "Unexpected." );

        uint32_t desiredColorWidth  = GetWidth( );
        uint32_t desiredColorHeight = GetHeight( );

        const bool bMatchesWindow = desiredColorWidth == apemodevk::Swapchain::kExtentMatchWindow &&
                                    desiredColorHeight == apemodevk::Swapchain::kExtentMatchWindow;

        const bool bMatchesFullscreen = desiredColorWidth == apemodevk::Swapchain::kExtentMatchFullscreen &&
                                        desiredColorHeight == apemodevk::Swapchain::kExtentMatchFullscreen;

        if ( !bMatchesWindow && !bMatchesFullscreen ) {
            currentExtent.width  = desiredColorWidth;
            currentExtent.height = desiredColorHeight;
        }
    } else {
        // If the surface size is defined, the swap chain size must match
        currentExtent = Surface.SurfaceCaps.currentExtent;
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

    LastWidth  = Swapchain.ImgExtent.width;
    LastHeight = Swapchain.ImgExtent.height;

    return true;
}

void apemode::AppSurfaceSdlVk::Finalize( ) {
    apemode_memory_allocation_scope;

    PresentQueueFamilyIds.clear( );

    Surface.Destroy( );
    Swapchain.Destroy( );
    Node.Destroy( );

    AppSurfaceSdlBase::Finalize( );
}

void apemode::AppSurfaceSdlVk::OnFrameMove( ) {
    const uint32_t width  = GetWidth( );
    const uint32_t height = GetHeight( );

    if ( width != LastWidth || height != LastHeight ) {
        apemode_memory_allocation_scope;
        apemodevk::CheckedResult( vkDeviceWaitIdle( Node ) );

        LastWidth  = width;
        LastHeight = height;

        VkExtent2D extent2D{width, height};

        const bool bResized = Swapchain.Recreate( &Node,
                                                  &Surface,
                                                  Swapchain.GetBufferCount( ),
                                                  extent2D,
                                                  Surface.eColorFormat,
                                                  Surface.eColorSpace,
                                                  Surface.eSurfaceTransform,
                                                  Surface.ePresentMode );
        SDL_assert( bResized );
        (void) bResized;
    }
}

void* apemode::AppSurfaceSdlVk::GetGraphicsHandle( ) {
    return reinterpret_cast< void* >( &Node );
}

apemode::SceneRendererBase* apemode::AppSurfaceSdlVk::CreateSceneRenderer( ) {
    apemode_memory_allocation_scope;
    return apemode_new apemode::SceneRendererVk( );
}
