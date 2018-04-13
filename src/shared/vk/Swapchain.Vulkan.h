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
        VkPhysicalDevice                    pPhysicalDevice;
        VkInstance                          pInstance;
        VkSurfaceCapabilitiesKHR            SurfaceCaps;
        VkSurfaceTransformFlagsKHR          eSurfaceTransform;
        VkFormat                            eColorFormat;
        VkColorSpaceKHR                     eColorSpace;
        VkPresentModeKHR                    ePresentMode;
        THandle< VkSurfaceKHR > hSurface;

#ifdef _WINDOWS_
        bool Recreate( VkPhysicalDevice pInPhysicalDevice, VkInstance pInInstance, HINSTANCE hInstance, HWND hWindow );
#endif

#ifdef X_PROTOCOL
        bool Recreate( VkPhysicalDevice pInPhysicalDevice, VkInstance pInInstance, Display* pDisplayX11, Window pWindowX11 );
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
        VkDevice                              pDevice;
        VkPhysicalDevice                      pPhysicalDevice;
        VkSurfaceKHR                          pSurface;
        uint32_t                              ImgCount;
        VkExtent2D                            ImgExtent;
        VkImage                               hImgs[ 3 ];
        THandle< VkImageView >    hImgViews[ 3 ];
        THandle< VkSwapchainKHR > hSwapchain;
    };
}
