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

        typedef std::vector< float >                           FloatVector;
        // typedef TInfoStruct< VkLayerProperties >::Vector       VkLayerPropertiesVector;
        // typedef TInfoStruct< VkQueueFamilyProperties >::Vector VkQueueFamilyPropertiesVector;
        // typedef TInfoStruct< VkDeviceQueueCreateInfo >::Vector VkDeviceQueueCreateInfoVector;
        typedef GraphicsManager::NativeLayerWrapper            NativeLayerWrapper;
        typedef VkFormatProperties                             VkFormatPropertiesArray[ VK_FORMAT_RANGE_SIZE ];

        static std::unique_ptr< GraphicsDevice > MakeNewUnique( );
        static std::unique_ptr< GraphicsDevice > MakeNullUnique( );

        GraphicsDevice( );
        ~GraphicsDevice( );

        bool RecreateResourcesFor( VkPhysicalDevice pPhysicalDevice, uint32_t flags );

        bool IsValid( ) const;
        bool Await( );

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

        THandle< VkDevice >                  hLogicalDevice;
        THandle< VmaAllocator >              hAllocator;
        VkPhysicalDevice                     pPhysicalDevice;
        VkPhysicalDeviceProperties           AdapterProps;
        VkPhysicalDeviceMemoryProperties     MemoryProps;
        VkPhysicalDeviceFeatures             Features;
        VkFormatPropertiesArray              FormatProperties;
        std::vector< const char * >          DeviceLayers;
        std::vector< const char * >          DeviceExtensions;
        std::vector< VkLayerProperties >     DeviceLayerProps;
        std::vector< VkExtensionProperties > DeviceExtensionProps;
        std::unique_ptr< QueuePool >         pQueuePool;
        std::unique_ptr< CommandBufferPool > pCmdBufferPool;
    };
}