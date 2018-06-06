#pragma once

#include <GraphicsDevice.Vulkan.h>

namespace apemodevk {
    class CommandBuffer;
    class CommandQueue;
    class GraphicsDevice;
    class ColorResourceView;
    class RenderPassResources;

    class Surface : public NoCopyAssignPolicy {
    public:
        VkPhysicalDevice           pPhysicalDevice;
        VkInstance                 pInstance;
        VkSurfaceCapabilitiesKHR   SurfaceCaps;
        VkSurfaceTransformFlagsKHR eSurfaceTransform;
        VkFormat                   eColorFormat;
        VkColorSpaceKHR            eColorSpace;
        VkPresentModeKHR           ePresentMode;
        THandle< VkSurfaceKHR >    hSurface;

#ifdef VK_USE_PLATFORM_WIN32_KHR
        bool Recreate( VkPhysicalDevice pInPhysicalDevice, VkInstance pInInstance, HINSTANCE hInstance, HWND hWindow ) {
            assert( nullptr != pInInstance );
            assert( nullptr != pInPhysicalDevice );
            assert( nullptr != hWindow );
            assert( nullptr != hInstance );

            pInstance       = pInInstance;
            pPhysicalDevice = pInPhysicalDevice;

            VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfoKHR;
            InitializeStruct( win32SurfaceCreateInfoKHR );

            win32SurfaceCreateInfoKHR.hwnd      = hWindow;
            win32SurfaceCreateInfoKHR.hinstance = hInstance;

            return hSurface.Recreate( pInInstance, win32SurfaceCreateInfoKHR ) && SetSurfaceProperties( );
        }
#elif VK_USE_PLATFORM_MACOS_MVK || VK_USE_PLATFORM_IOS_MVK
        bool Recreate( VkPhysicalDevice pInPhysicalDevice, VkInstance pInInstance, void* pView ) {
            assert( nullptr != pInPhysicalDevice );
            assert( nullptr != pInInstance );
            assert( nullptr != pView );

            pInstance       = pInInstance;
            pPhysicalDevice = pInPhysicalDevice;

#if VK_USE_PLATFORM_IOS_MVK
            VkIOSSurfaceCreateInfoMVK surfaceCreateInfoMVK;
#elif VK_USE_PLATFORM_MACOS_MVK
            VkMacOSSurfaceCreateInfoMVK surfaceCreateInfoMVK;
#endif

            InitializeStruct( surfaceCreateInfoMVK );
            surfaceCreateInfoMVK.sType = VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK;
            surfaceCreateInfoMVK.pNext = NULL;
            surfaceCreateInfoMVK.flags = 0;
            surfaceCreateInfoMVK.pView = pView;

            return hSurface.Recreate( pInInstance, surfaceCreateInfoMVK ) && SetSurfaceProperties( );
        }
#elif VK_USE_PLATFORM_XLIB_KHR
        bool Recreate( VkPhysicalDevice pInPhysicalDevice, VkInstance pInInstance, Display* pDisplayX11, Window pWindowX11 ) {
            assert( nullptr != pInPhysicalDevice );
            assert( nullptr != pInInstance );
            assert( nullptr != pDisplayX11 );

            pInstance       = pInInstance;
            pPhysicalDevice = pInPhysicalDevice;

            VkXlibSurfaceCreateInfoKHR xlibSurfaceCreateInfoKHR;
            InitializeStruct( xlibSurfaceCreateInfoKHR );

            xlibSurfaceCreateInfoKHR.window = pWindowX11;
            xlibSurfaceCreateInfoKHR.dpy    = pDisplayX11;

            return hSurface.Recreate( pInInstance, xlibSurfaceCreateInfoKHR ) && SetSurfaceProperties( );
        }
#endif

    protected:
        bool SetSurfaceProperties( );
    };

    class Swapchain : public NoCopyAssignPolicy {
    public:
        static uint32_t const kExtentMatchFullscreen;
        static uint32_t const kExtentMatchWindow;
        static uint64_t const kMaxTimeout;
        static uint32_t const kMaxImgs;

        Swapchain( );
        ~Swapchain( );

        /**
         * Initialized surface, render targets` and depth stencils` views.
         * @return True if initialization went ok, false otherwise.
         */
        bool Recreate( VkDevice                   pInDevice,
                       VkPhysicalDevice           pInPhysicalDevice,
                       VkSurfaceKHR               pInSurface,
                       uint32_t                   InImgCount,
                       uint32_t                   InDesiredColorWidth,
                       uint32_t                   InDesiredColorHeight,
                       VkFormat                   eColorFormat,
                       VkColorSpaceKHR            eColorSpace,
                       VkSurfaceTransformFlagsKHR eSurfaceTransform,
                       VkPresentModeKHR           ePresentMode );

        bool ExtractSwapchainBuffers( VkImage* OutSwapchainBufferImgs );

        uint32_t GetBufferCount( ) const;

    public:
        VkDevice                  pDevice;
        VkPhysicalDevice          pPhysicalDevice;
        VkSurfaceKHR              pSurface;
        uint32_t                  ImgCount;
        VkExtent2D                ImgExtent;
        VkImage                   hImgs[ 3 ];
        THandle< VkImageView >    hImgViews[ 3 ];
        THandle< VkSwapchainKHR > hSwapchain;
    };
}
