#pragma once

#include <GraphicsManager.Vulkan.h>

namespace apemodevk {
    class Swapchain;
    class CommandQueue;
    class GraphicsManager;

    class QueuePool;
    class CommandBufferPool;
    class ShaderCompiler;

    class GraphicsDevice : public NoCopyAssignPolicy {
    public:
        friend Swapchain;
        friend GraphicsManager;

        typedef GraphicsManager::NativeLayerWrapper NativeLayerWrapper;
        typedef VkFormatProperties                  VkFormatPropertiesArray[ VK_FORMAT_RANGE_SIZE ];

        GraphicsDevice( );
        ~GraphicsDevice( );

        bool RecreateResourcesFor( VkPhysicalDevice pPhysicalDevice, uint32_t flags );

        bool IsValid( ) const;
        bool Await( );

        // void *GetUnusedObj( );
        // bool  Acquire( void *pObj );
        // bool  Release( void *pObj );

        QueuePool *              GetQueuePool( );
        const QueuePool *        GetQueuePool( ) const;
        CommandBufferPool *      GetCommandBufferPool( );
        const CommandBufferPool *GetCommandBufferPool( ) const;

        bool ScanDeviceQueues( std::vector< VkQueueFamilyProperties > &QueueProps,
                               std::vector< VkDeviceQueueCreateInfo > &QueueReqs,
                               std::vector< float > &                  QueuePriorities );

        bool ScanDeviceLayerProperties( uint32_t flags );
        bool ScanFormatProperties( );

        operator VkDevice( ) const;
        operator VkPhysicalDevice( ) const;
        operator VkInstance( ) const;

        // struct ObjWithCounter {
        //     void *   pObj       = nullptr;
        //     uint32_t UseCounter = 0;
        // };

        THandle< VkDevice >                   hLogicalDevice;
        THandle< VmaAllocator >               hAllocator;
        VkPhysicalDevice                      pPhysicalDevice;
        VkPhysicalDeviceProperties            AdapterProps;
        VkPhysicalDeviceMemoryProperties      MemoryProps;
        VkPhysicalDeviceFeatures              Features;
        VkFormatPropertiesArray               FormatProperties;
        std::vector< const char * >           DeviceLayers;
        std::vector< const char * >           DeviceExtensions;
        std::vector< VkLayerProperties >      DeviceLayerProps;
        std::vector< VkExtensionProperties >  DeviceExtensionProps;
        std::unique_ptr< QueuePool >          pQueuePool;
        std::unique_ptr< CommandBufferPool >  pCmdBufferPool;
        // std::vector< ObjWithCounter >         ObjPool;
    };
}