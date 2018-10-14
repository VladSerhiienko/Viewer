#pragma once

#include <apemode/vk/GraphicsDevice.Vulkan.h>
#include <apemode/vk/GraphicsManager.Vulkan.h>
#include <apemode/vk/QueuePools.Vulkan.h>
#include <apemode/vk/Swapchain.Vulkan.h>

namespace apemodevk {

struct GraphicsLogger;
struct GraphicsAllocator;
struct GraphicsMemoryAllocationScope;

struct PlatformSurface {
    VkExtent2D OverrideExtent{0, 0};

#if defined( VK_USE_PLATFORM_MACOS_MVK ) && VK_USE_PLATFORM_MACOS_MVK
    void* pViewMacOS = nullptr;
#elif defined( VK_USE_PLATFORM_WIN32_KHR ) && VK_USE_PLATFORM_WIN32_KHR
    HWND      hWnd      = NULL;
    HINSTANCE hInstance = NULL;
#elif defined( VK_USE_PLATFORM_XLIB_KHR ) && VK_USE_PLATFORM_XLIB_KHR
    Display* pDisplayX11 = nullptr;
    Window   pWindowX11;
#elif defined( VK_USE_PLATFORM_IOS_MVK ) && VK_USE_PLATFORM_IOS_MVK
    void* pViewIOS = nullptr;
#elif defined( VK_USE_PLATFORM_ANDROID_KHR ) && VK_USE_PLATFORM_ANDROID_KHR
    struct ANativeWindow* pANativeWindow = nullptr;
#endif
};

/* Contains handle to window and graphics context.
 * For Vulkan it contains all the core things like VkInstance, VkDevice, VkSurfaceKHR, VkSwapchainKHR
 * since it does not make much sense to move them outside this class.
 * It tries also to handle resizing in OnFrameMove, but there is an error with image state.
 * The barriors in case of resizing must be managed explicitely.
 */
class AppSurface {
public:
    AppSurface( );
    virtual ~AppSurface( );

    bool Initialize( const PlatformSurface* pPlatformSurface );
    bool Resize( VkExtent2D extent );
    void Finalize( );

public:
    std::vector< uint32_t > PresentQueueFamilyIds;

    apemode::unique_ptr< GraphicsAllocator > Alloc;
    apemode::unique_ptr< GraphicsLogger >    Logger;

    apemodevk::unique_ptr< apemodevk::GraphicsManager > Manager;
    apemodevk::Surface                                  Surface;
    apemodevk::Swapchain                                Swapchain;
    apemodevk::GraphicsDevice                           Node;
};

} // namespace apemodevk