#pragma once

#include <apemode/vk/GraphicsManager.Vulkan.h>
#include <apemode/vk/QueuePools.Vulkan.h>

namespace apemodevk {
    class Swapchain;
    class CommandQueue;
    class GraphicsManager;

    class ShaderCompiler;

    class APEMODEVK_API GraphicsDevice : public VolkDeviceTable, public NoCopyAssignPolicy {
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

        bool ScanDeviceQueues( apemodevk::vector< VkQueueFamilyProperties > &queueProps,
                               apemodevk::vector< VkDeviceQueueCreateInfo > &queueReqs,
                               apemodevk::vector< float > &                  queuePriorities );

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
            bool bIncrementalPresentKHR = false;
            bool bDisplayTimingGOOGLE   = false;
        } Ext;

    };
}
