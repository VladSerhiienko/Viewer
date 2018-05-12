#pragma once

#include <GraphicsManager.Vulkan.h>
#include <QueuePools.Vulkan.h>

namespace apemodevk {
    class Swapchain;
    class CommandQueue;
    class GraphicsManager;

    class ShaderCompiler;

    class GraphicsDevice : public NoCopyAssignPolicy {
    public:
        friend Swapchain;
        friend GraphicsManager;
        typedef VkFormatProperties VkFormatPropertiesArray[ VK_FORMAT_RANGE_SIZE ];

        GraphicsDevice( );
        ~GraphicsDevice( );

        bool RecreateResourcesFor( uint32_t         eFlags,
                                   VkPhysicalDevice pPhysicalDevice,
                                   const char **    ppszLayers,
                                   size_t           layerCount,
                                   const char **    ppszExtensions,
                                   size_t           extensionCount );

        void Destroy( );

        bool IsValid( ) const;
        bool Await( );

        QueuePool *              GetQueuePool( );
        const QueuePool *        GetQueuePool( ) const;
        CommandBufferPool *      GetCommandBufferPool( );
        const CommandBufferPool *GetCommandBufferPool( ) const;

        bool ScanDeviceQueues( std::vector< VkQueueFamilyProperties > &queueProps,
                               std::vector< VkDeviceQueueCreateInfo > &queueReqs,
                               std::vector< float > &                  queuePriorities );

        bool ScanFormatProperties( );

        operator VkDevice( ) const;
        operator VkPhysicalDevice( ) const;
        operator VkInstance( ) const;

        THandle< VkDevice >              hLogicalDevice;
        THandle< VmaAllocator >          hAllocator;
        VkPhysicalDevice                 pPhysicalDevice;
        VkPhysicalDeviceProperties       AdapterProps;
        VkPhysicalDeviceMemoryProperties MemoryProps;
        VkPhysicalDeviceFeatures         Features;
        VkFormatPropertiesArray          FormatProperties;
        QueuePool                        Queues;
        CommandBufferPool                CmdBuffers;

        struct {
            PFN_vkCreateSwapchainKHR              CreateSwapchainKHR              = nullptr;
            PFN_vkDestroySwapchainKHR             DestroySwapchainKHR             = nullptr;
            PFN_vkGetSwapchainImagesKHR           GetSwapchainImagesKHR           = nullptr;
            PFN_vkAcquireNextImageKHR             AcquireNextImageKHR             = nullptr;
            PFN_vkQueuePresentKHR                 QueuePresentKHR                 = nullptr;
            PFN_vkGetRefreshCycleDurationGOOGLE   GetRefreshCycleDurationGOOGLE   = nullptr;
            PFN_vkGetPastPresentationTimingGOOGLE GetPastPresentationTimingGOOGLE = nullptr;
            bool                                  bIncrementalPresentKHR          = false;
            bool                                  bDisplayTimingGOOGLE            = false;
        } Ext;

    };
}