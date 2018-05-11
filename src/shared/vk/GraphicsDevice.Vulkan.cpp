//#include <GameEngine.GraphicsEcosystem.Precompiled.h>

#include <GraphicsDevice.Vulkan.h>
#include <GraphicsManager.Vulkan.h>

#include <QueuePools.Vulkan.h>
#include <ShaderCompiler.Vulkan.h>

#include <GraphicsManager.KnownExtensions.Vulkan.h>
#include <NativeHandles.Vulkan.h>

#include <TInfoStruct.Vulkan.h>

bool apemodevk::GraphicsDevice::ScanDeviceQueues( std::vector< VkQueueFamilyProperties >& QueueProps,
                                                  std::vector< VkDeviceQueueCreateInfo >& QueueReqs,
                                                  std::vector< float >&                   QueuePriorities ) {
    uint32_t QueuesFound = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( pPhysicalDevice, &QueuesFound, NULL );

    if ( QueuesFound != 0 ) {
        QueueProps.resize( QueuesFound );
        QueueReqs.reserve( QueuesFound );

        vkGetPhysicalDeviceQueueFamilyProperties( pPhysicalDevice, &QueuesFound, QueueProps.data( ) );
        platform::DebugTrace( platform::LogLevel::Info, "Queue Families: %u", QueuesFound );

        uint32_t TotalQueuePrioritiesCount = 0;
        std::for_each( QueueProps.begin( ), QueueProps.end( ), [&]( VkQueueFamilyProperties& QueueProp ) {
            platform::DebugTrace( platform::LogLevel::Info, "> Flags: %x, Count: %u", QueueProp.queueFlags, QueueProp.queueCount );
            TotalQueuePrioritiesCount += QueueProp.queueCount;
        } );

        uint32_t QueuePrioritiesAssigned = 0;
        QueuePriorities.resize( TotalQueuePrioritiesCount );
        std::for_each( QueueProps.begin( ), QueueProps.end( ), [&]( VkQueueFamilyProperties& QueueProp ) {

            VkDeviceQueueCreateInfo queueCreateInfo;
            InitializeStruct( queueCreateInfo );

            queueCreateInfo.pNext            = NULL;
            queueCreateInfo.queueFamilyIndex = static_cast< uint32_t >( std::distance( QueueProps.data( ), &QueueProp ) );
            queueCreateInfo.queueCount       = QueueProp.queueCount;
            queueCreateInfo.pQueuePriorities = QueuePriorities.data( ) + QueuePrioritiesAssigned;

            QueuePrioritiesAssigned += QueueProp.queueCount;

            QueueReqs.push_back(queueCreateInfo);
        } );
    }

    return true;
}

bool apemodevk::GraphicsDevice::ScanFormatProperties( ) {
    VkFormat NativeFormatIt = VK_FORMAT_UNDEFINED;
    for ( ; NativeFormatIt < VK_FORMAT_RANGE_SIZE; ) {
        const VkFormat NativeFormat = NativeFormatIt;
        vkGetPhysicalDeviceFormatProperties( pPhysicalDevice, NativeFormat, &FormatProperties[ NativeFormat ] );
        NativeFormatIt = static_cast< VkFormat >( NativeFormatIt + 1 );
    }

    return true;
}

bool apemodevk::GraphicsDevice::RecreateResourcesFor( VkPhysicalDevice InAdapterHandle, uint32_t flags ) {
    apemode_assert( Args != nullptr, "Ecosystem is required in case of Vulkan." );

    pPhysicalDevice = InAdapterHandle;

    // Physical device is required to create a related logical device.
    // Likewise, vulkan instance is required for physical device.
    if ( VK_NULL_HANDLE != pPhysicalDevice ) {
        vkGetPhysicalDeviceProperties( pPhysicalDevice, &AdapterProps );
        vkGetPhysicalDeviceMemoryProperties( pPhysicalDevice, &MemoryProps );
        vkGetPhysicalDeviceFeatures( pPhysicalDevice, &Features );

        platform::DebugTrace( platform::LogLevel::Info, "Device Name: %s", AdapterProps.deviceName );
        platform::DebugTrace( platform::LogLevel::Info, "Device Type: %u", AdapterProps.deviceType );

        std::vector< VkQueueFamilyProperties > QueueProps;
        std::vector< VkDeviceQueueCreateInfo > QueueReqs;
        std::vector< float >                   QueuePriorities;

        if ( ScanDeviceQueues( QueueProps, QueueReqs, QueuePriorities ) && ScanFormatProperties( ) ) {

            VkPhysicalDeviceFeatures Features;
            vkGetPhysicalDeviceFeatures( pPhysicalDevice, &Features );

            VkDeviceCreateInfo deviceCreateInfo      = TNewInitializedStruct< VkDeviceCreateInfo >( );
            deviceCreateInfo.pEnabledFeatures        = &Features;
            deviceCreateInfo.queueCreateInfoCount    = GetSizeU( QueueReqs );
            deviceCreateInfo.pQueueCreateInfos       = QueueReqs.data( );
            // deviceCreateInfo.enabledLayerCount       = (uint32_t) DeviceLayers.size( );
            // deviceCreateInfo.ppEnabledLayerNames     = DeviceLayers.data( );
            // deviceCreateInfo.enabledExtensionCount   = (uint32_t) DeviceExtensions.size( );
            // deviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions.data( );

            const auto bOk = hLogicalDevice.Recreate( pPhysicalDevice, deviceCreateInfo );
            apemode_assert( bOk, "vkCreateDevice failed." );

            if ( bOk ) {

                VmaAllocatorCreateInfo allocatorCreateInfo = {};
                allocatorCreateInfo.physicalDevice         = pPhysicalDevice;
                allocatorCreateInfo.device                 = hLogicalDevice;
                allocatorCreateInfo.pAllocationCallbacks   = GetAllocationCallbacks( );

                hAllocator.Recreate( allocatorCreateInfo );

                pQueuePool.reset( new QueuePool( hLogicalDevice,
                                                 pPhysicalDevice,
                                                 QueueProps.data( ),
                                                 QueueProps.data( ) + QueueProps.size( ),
                                                 QueuePriorities.data( ),
                                                 QueuePriorities.data( ) + QueuePriorities.size( ) ) );

                pCmdBufferPool.reset( new CommandBufferPool( hLogicalDevice,
                                                             pPhysicalDevice,
                                                             QueueProps.data( ),
                                                             QueueProps.data( ) + QueueProps.size( ) ) );
            }

            return bOk;
        }
    }

    return false;
}

/// -------------------------------------------------------------------------------------------------------------------
/// GraphicsDevice
/// -------------------------------------------------------------------------------------------------------------------

apemodevk::GraphicsDevice::GraphicsDevice( ) {
}

apemodevk::GraphicsDevice::~GraphicsDevice( ) {
}

apemodevk::GraphicsDevice::operator VkDevice( ) const {
    return hLogicalDevice;
}

apemodevk::GraphicsDevice::operator VkPhysicalDevice( ) const {
    return pPhysicalDevice;
}

apemodevk::GraphicsDevice::operator VkInstance( ) const {
    return GetGraphicsManager( )->hInstance;
}

bool apemodevk::GraphicsDevice::IsValid( ) const {
    return hLogicalDevice.IsNotNull( );
}

bool apemodevk::GraphicsDevice::Await( ) {
    apemode_assert( IsValid( ), "Must be valid." );
    return VK_SUCCESS == CheckedCall( vkDeviceWaitIdle( hLogicalDevice ) );
}

apemodevk::QueuePool* apemodevk::GraphicsDevice::GetQueuePool( ) {
    return pQueuePool.get( );
}

const apemodevk::QueuePool* apemodevk::GraphicsDevice::GetQueuePool( ) const {
    return pQueuePool.get( );
}

apemodevk::CommandBufferPool * apemodevk::GraphicsDevice::GetCommandBufferPool()
{
    return pCmdBufferPool.get( );
}

const apemodevk::CommandBufferPool* apemodevk::GraphicsDevice::GetCommandBufferPool( ) const {
    return pCmdBufferPool.get( );
}

#pragma warning(push, 4)
#pragma warning(disable: 4127) // warning C4127: conditional expression is constant
#pragma warning(disable: 4100) // warning C4100: '...': unreferenced formal parameter
#pragma warning(disable: 4189) // warning C4189: '...': local variable is initialized but not referenced
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#pragma warning(pop)