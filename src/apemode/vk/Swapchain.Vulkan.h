#pragma once

#include <apemode/vk/GraphicsManager.Vulkan.h>
#include <apemode/vk/GraphicsDevice.Vulkan.h>

namespace apemodevk {
    class CommandBuffer;
    class CommandQueue;
    class GraphicsDevice;
    class ColorResourceView;
    class RenderPassResources;

    class APEMODEVK_API Surface : public NoCopyAssignPolicy {
    public:
        VkPhysicalDevice           pPhysicalDevice;
        VkInstance                 pInstance;
        VkSurfaceCapabilitiesKHR   SurfaceCaps;
        VkSurfaceTransformFlagsKHR eSurfaceTransform;
        VkFormat                   eColorFormat;
        VkColorSpaceKHR            eColorSpace;
        VkPresentModeKHR           ePresentMode;
        THandle< VkSurfaceKHR >    hSurface;

        ~Surface( );
        void Destroy( );

#ifdef VK_USE_PLATFORM_WIN32_KHR
        inline bool Recreate( VkPhysicalDevice pInPhysicalDevice, VkInstance pInInstance, HINSTANCE hInstance, HWND hWindow ) {
            apemodevk_memory_allocation_scope;

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
        inline bool Recreate( VkPhysicalDevice pInPhysicalDevice, VkInstance pInInstance, void* pView ) {
            apemodevk_memory_allocation_scope;

            assert( nullptr != pInPhysicalDevice );
            assert( nullptr != pInInstance );
            assert( nullptr != pView );

            pInstance       = pInInstance;
            pPhysicalDevice = pInPhysicalDevice;

#if VK_USE_PLATFORM_IOS_MVK
            VkIOSSurfaceCreateInfoMVK moltenSurfaceCreateInfo = {};
            moltenSurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK;
#elif VK_USE_PLATFORM_MACOS_MVK
            VkMacOSSurfaceCreateInfoMVK moltenSurfaceCreateInfo = {};
            moltenSurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
#endif

            moltenSurfaceCreateInfo.pNext = NULL;
            moltenSurfaceCreateInfo.flags = 0;
            moltenSurfaceCreateInfo.pView = pView;

            return hSurface.Recreate( pInInstance, moltenSurfaceCreateInfo ) && SetSurfaceProperties( );
        }
#elif VK_USE_PLATFORM_XLIB_KHR
        inline bool Recreate( VkPhysicalDevice pInPhysicalDevice, VkInstance pInInstance, Display* pDisplayX11, Window pWindowX11 ) {
            apemodevk_memory_allocation_scope;

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
#elif VK_USE_PLATFORM_ANDROID_KHR
        inline bool Recreate( VkPhysicalDevice pInPhysicalDevice, VkInstance pInInstance, struct ANativeWindow *pANativeWindow ) {
            apemodevk_memory_allocation_scope;

            assert( nullptr != pInPhysicalDevice );
            assert( nullptr != pInInstance );
            assert( nullptr != pDisplayX11 );

            pInstance       = pInInstance;
            pPhysicalDevice = pInPhysicalDevice;

            VkAndroidSurfaceCreateInfoKHR androidSurfaceCreateInfoKHR;
            InitializeStruct( androidSurfaceCreateInfoKHR );

            androidSurfaceCreateInfoKHR.window = pANativeWindow;

            return hSurface.Recreate( pInInstance, androidSurfaceCreateInfoKHR ) && SetSurfaceProperties( );
        }
#endif

    protected:
        bool SetSurfaceProperties( );
    };
    
    class APEMODEVK_API PresentEngine : public NoCopyAssignPolicy {
    public:
        struct Frame {
         THandle<VkSemaphore> ImgAcquiredSemaphore;
         THandle<VkSemaphore> ImgCompleteSemaphore;
         THandle<VkSemaphore> ImgOwnershipTransferredSemaphore;
        };
        
        apemodevk::GraphicsDevice*  pNode;
        apemodevk::Surface*        pSurface;
        apemodevk::vector<Frame> Frames;
        uint32_t GraphicsQueueFamilyId = -1;
        uint32_t PresentQueueFamilyId = -1;
        
        bool UsesSeparatePresentQueue() const {
            return GraphicsQueueFamilyId != PresentQueueFamilyId;
        }
        
        bool Initialize(apemodevk::GraphicsDevice* pInNode,
                        apemodevk::Surface*        pInSurface,
                        uint32_t frameCount) {
            assert(pNode);
            assert(frameCount);
            
            pNode = pInNode;
            pSurface = pInSurface;
            Frames.resize(frameCount);
            
            GraphicsQueueFamilyId = -1;
            PresentQueueFamilyId = -1;
            
            auto r = pNode->GetQueuePool()->QueueFamilyIds.equal_range(VK_QUEUE_GRAPHICS_BIT);
            if(r.first == pNode->GetQueuePool()->QueueFamilyIds.end()) {
                return false;
            }
            
        
            GraphicsQueueFamilyId = r.first->second;
            
            for ( auto it = r.first ; it != r.second; ++it ) {
                auto p = pNode->GetQueuePool()->GetPool(it->second);
                if (p && p->SupportsPresenting(pSurface->hSurface.Handle)) {
                    GraphicsQueueFamilyId = it->second;
                    PresentQueueFamilyId = it->second;
                    break;
                }
            }
            
            
            
            for (auto & queuePool : pNode->GetQueuePool()->Pools) {
            
            }
        
        }
    };

    class APEMODEVK_API Swapchain : public NoCopyAssignPolicy {
    public:
        static uint32_t const kExtentMatchFullscreen;
        static uint32_t const kExtentMatchWindow;
        static uint64_t const kMaxTimeout;
        static uint32_t const kMaxImgs;

        struct Buffer {
            VkImage                hImg;
            THandle< VkImageView > hImgView;
        };

        Swapchain( );
        ~Swapchain( );

        void Destroy( );

        /**
         * Initialized surface, render targets` and depth stencils` views.
         * @return True if initialization went ok, false otherwise.
         */
        bool Recreate( apemodevk::GraphicsDevice* pInNode,
                       apemodevk::Surface*        pInSurface,
                       uint32_t                   desiredImgCount,
                       VkExtent2D                 desiredImgExtent,
                       VkFormat                   eColorFormat,
                       VkColorSpaceKHR            eColorSpace,
                       VkSurfaceTransformFlagsKHR eSurfaceTransform,
                       VkPresentModeKHR           ePresentMode );

        bool ExtractSwapchainBuffers( VkImage* pOutBuffers );
        uint32_t GetBufferCount( ) const;

    public:
        apemodevk::GraphicsDevice*  pNode;
        apemodevk::Surface*         pSurface;
        apemodevk::vector< Buffer > Buffers;
        VkExtent2D                  ImgExtent;
        THandle< VkSwapchainKHR >   hSwapchain;
    };
}
