//#include <GameEngine.GraphicsEcosystem.Precompiled.h>

#include <GraphicsDevice.Vulkan.h>
#include <GraphicsManager.Vulkan.h>
#include <NativeHandles.Vulkan.h>
#include <TInfoStruct.Vulkan.h>

bool apemodevk::GraphicsDevice::ScanDeviceQueues( apemodevk::vector< VkQueueFamilyProperties >& queueProps,
                                                  apemodevk::vector< VkDeviceQueueCreateInfo >& queueReqs,
                                                  apemodevk::vector< float >&                   queuePriorities ) {
    apemodevk_memory_allocation_scope;

    uint32_t queuesFound = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( pPhysicalDevice, &queuesFound, NULL );

    if ( queuesFound != 0 ) {
        queueProps.resize( queuesFound );
        queueReqs.reserve( queuesFound );

        vkGetPhysicalDeviceQueueFamilyProperties( pPhysicalDevice, &queuesFound, queueProps.data( ) );
        platform::LogFmt( platform::LogLevel::Info, "Queue Families: %u", queuesFound );

        uint32_t TotalQueuePrioritiesCount = 0;
        eastl::for_each( queueProps.begin( ), queueProps.end( ), [&]( VkQueueFamilyProperties& queueProp ) {

            platform::LogFmt(
                platform::LogLevel::Info,
                "> Count %u, Flags 0x%X: GRAPHICS (%s), COMPUTE (%s), TRANSFER (%s), SPARSE_BINDING (%s), PROTECTED (%s) ",
                queueProp.queueCount,
                queueProp.queueFlags,
                ( HasFlagAny( queueProp.queueFlags, VK_QUEUE_GRAPHICS_BIT ) ? "T" : "F" ),
                ( HasFlagAny( queueProp.queueFlags, VK_QUEUE_COMPUTE_BIT ) ? "T" : "F" ),
                ( HasFlagAny( queueProp.queueFlags, VK_QUEUE_TRANSFER_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT ) ? "T" : "F" ),
                ( HasFlagAny( queueProp.queueFlags, VK_QUEUE_SPARSE_BINDING_BIT ) ? "T" : "F" ),
                ( HasFlagAny( queueProp.queueFlags, VK_QUEUE_PROTECTED_BIT ) ? "T" : "F" ) );

            TotalQueuePrioritiesCount += queueProp.queueCount;
        } );

        uint32_t QueuePrioritiesAssigned = 0;
        queuePriorities.resize( TotalQueuePrioritiesCount );
        eastl::for_each( queueProps.begin( ), queueProps.end( ), [&]( VkQueueFamilyProperties& queueProp ) {

            VkDeviceQueueCreateInfo queueCreateInfo;
            InitializeStruct( queueCreateInfo );

            queueCreateInfo.pNext            = NULL;
            queueCreateInfo.queueFamilyIndex = static_cast< uint32_t >( eastl::distance( queueProps.data( ), &queueProp ) );
            queueCreateInfo.queueCount       = queueProp.queueCount;
            queueCreateInfo.pQueuePriorities = queuePriorities.data( ) + QueuePrioritiesAssigned;

            QueuePrioritiesAssigned += queueProp.queueCount;
            queueReqs.push_back( queueCreateInfo );
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

void AddName( apemodevk::vector< const char* >& names, const char* pszName );
bool EnumerateLayersAndExtensions( apemodevk::GraphicsDevice*        pNode,
                                   uint32_t                          eFlags,
                                   apemodevk::vector< const char* >& OutLayerNames,
                                   apemodevk::vector< const char* >& OutExtensionNames,
                                   bool&                             bIncrementalPresentKHR,
                                   bool&                             bDisplayTimingGOOGLE,
                                   const char**                      ppszLayers,
                                   size_t                            layerCount,
                                   const char**                      ppszExtensions,
                                   size_t                            extensionCount ) {
    apemodevk_memory_allocation_scope;
    VkResult err = VK_SUCCESS;

    uint32_t deviceLayerCount = 0;
    err = vkEnumerateDeviceLayerProperties( pNode->pPhysicalDevice, &deviceLayerCount, NULL );
    if ( err )
        return false;

    if ( deviceLayerCount > 0 ) {

        apemodevk::vector< VkLayerProperties > deviceLayers;
        deviceLayers.resize( deviceLayerCount );

        err = vkEnumerateDeviceLayerProperties( pNode->pPhysicalDevice, &deviceLayerCount, deviceLayers.data( ) );
        if ( err )
            return false;

        for ( auto& deviceLayer : deviceLayers ) {
            apemodevk::platform::LogFmt( apemodevk::platform::LogLevel::Debug,
                                             "> DeviceLayer: %s (v.%u): %s",
                                             deviceLayer.layerName,
                                             deviceLayer.specVersion,
                                             deviceLayer.description );

            for ( uint32_t j = 0; j < layerCount; ++j ) {
                if ( !strcmp( ppszLayers[ j ], deviceLayer.layerName ) ) {
                    OutLayerNames.push_back( ppszLayers[ j ] );
                }
            }
        }
    }

    uint32_t deviceExtensionCount = 0;
    err = vkEnumerateDeviceExtensionProperties( pNode->pPhysicalDevice, NULL, &deviceExtensionCount, NULL );
    if ( err )
        return false;

    if ( deviceExtensionCount > 0 ) {

        apemodevk::vector< VkExtensionProperties > deviceExtensions;
        deviceExtensions.resize( deviceExtensionCount );

        err = vkEnumerateDeviceExtensionProperties( pNode->pPhysicalDevice, NULL, &deviceExtensionCount, deviceExtensions.data( ) );
        if ( err )
            return false;

        VkBool32 swapchainExtFound = 0;
        for ( uint32_t i = 0; i < deviceExtensionCount; i++ ) {

            apemodevk::platform::LogFmt( apemodevk::platform::LogLevel::Debug,
                                             "> DeviceExtension: %s (v.%u)",
                                             deviceExtensions[ i ].extensionName,
                                             deviceExtensions[ i ].specVersion );

            for ( uint32_t j = 0; j < extensionCount; ++j ) {
                if ( !strcmp( ppszExtensions[ j ], deviceExtensions[ i ].extensionName ) ) {
                    OutExtensionNames.push_back( ppszExtensions[ j ] );
                }
            }

            if ( !strcmp( VK_KHR_SWAPCHAIN_EXTENSION_NAME, deviceExtensions[ i ].extensionName ) ) {
                swapchainExtFound = 1;
                AddName( OutExtensionNames, VK_KHR_SWAPCHAIN_EXTENSION_NAME );
            }

            if ( !strcmp( VK_KHR_INCREMENTAL_PRESENT_EXTENSION_NAME, deviceExtensions[ i ].extensionName ) ) {
                bIncrementalPresentKHR = true;
                AddName( OutExtensionNames, VK_KHR_INCREMENTAL_PRESENT_EXTENSION_NAME );
            }

            if ( !strcmp( VK_GOOGLE_DISPLAY_TIMING_EXTENSION_NAME, deviceExtensions[ i ].extensionName ) ) {
                bIncrementalPresentKHR = true;
                AddName( OutExtensionNames, VK_GOOGLE_DISPLAY_TIMING_EXTENSION_NAME );
            }
        }

        if ( !swapchainExtFound ) {
            return false;
        }

        for ( auto& l : OutLayerNames ) {
            err = vkEnumerateDeviceExtensionProperties( pNode->pPhysicalDevice, l, &deviceExtensionCount, NULL );
            if ( err )
                return false;

            err = vkEnumerateDeviceExtensionProperties( pNode->pPhysicalDevice, l, &deviceExtensionCount, deviceExtensions.data( ) );
            if ( err )
                return false;

            for ( uint32_t i = 0; i < deviceExtensionCount; i++ ) {
                for ( uint32_t j = 0; j < extensionCount; ++j ) {
                    if ( !strcmp( ppszExtensions[ j ], deviceExtensions[ i ].extensionName ) ) {
                        AddName( OutExtensionNames, ppszExtensions[ j ] );
                    }
                }
            }
        }
    }

    return true;
}

bool apemodevk::GraphicsDevice::RecreateResourcesFor( uint32_t         flags,
                                                      VkPhysicalDevice pInPhysicalDevice,
                                                      const char**     ppszLayers,
                                                      size_t           layerCount,
                                                      const char**     ppszExtensions,
                                                      size_t           extensionCount ) {
    apemodevk_memory_allocation_scope;

    pPhysicalDevice = pInPhysicalDevice;
    assert( pPhysicalDevice );

    // Physical device is required to create a related logical device.
    // Likewise, vulkan instance is required for physical device.
    if ( VK_NULL_HANDLE != pPhysicalDevice ) {

        vector< const char* > deviceLayers;
        vector< const char* > deviceExtensions;

        if ( !EnumerateLayersAndExtensions( this,
                                            flags,
                                            deviceLayers,
                                            deviceExtensions,
                                            Ext.bIncrementalPresentKHR,
                                            Ext.bDisplayTimingGOOGLE,
                                            ppszLayers,
                                            layerCount,
                                            ppszExtensions,
                                            extensionCount ) )
            return false;

        vkGetPhysicalDeviceProperties( pPhysicalDevice, &AdapterProps );
        vkGetPhysicalDeviceMemoryProperties( pPhysicalDevice, &MemoryProps );
        vkGetPhysicalDeviceFeatures( pPhysicalDevice, &Features );

        platform::LogFmt( platform::LogLevel::Info, "Device Name: %s", AdapterProps.deviceName );
        platform::LogFmt( platform::LogLevel::Info, "Device Type: %u", AdapterProps.deviceType );

        vector< VkQueueFamilyProperties > QueueProps;
        vector< VkDeviceQueueCreateInfo > QueueReqs;
        vector< float >                   QueuePriorities;

        if ( ScanDeviceQueues( QueueProps, QueueReqs, QueuePriorities ) && ScanFormatProperties( ) ) {
            vkGetPhysicalDeviceFeatures( pPhysicalDevice, &Features );

            VkDeviceCreateInfo deviceCreateInfo      = TNewInitializedStruct< VkDeviceCreateInfo >( );
            deviceCreateInfo.pEnabledFeatures        = &Features;
            deviceCreateInfo.queueCreateInfoCount    = GetSizeU( QueueReqs );
            deviceCreateInfo.pQueueCreateInfos       = QueueReqs.data( );
            deviceCreateInfo.enabledLayerCount       = GetSizeU( deviceLayers );
            deviceCreateInfo.ppEnabledLayerNames     = deviceLayers.data( );
            deviceCreateInfo.enabledExtensionCount   = GetSizeU( deviceExtensions );
            deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data( );

            if ( hLogicalDevice.Recreate( pPhysicalDevice, deviceCreateInfo ) ) {

                PFN_vkGetDeviceProcAddr GetDeviceProcAddr = GetGraphicsManager()->Ext.GetDeviceProcAddr;

                Ext.CreateSwapchainKHR    = (PFN_vkCreateSwapchainKHR)    GetDeviceProcAddr( hLogicalDevice, "vkCreateSwapchainKHR" );
                Ext.DestroySwapchainKHR   = (PFN_vkDestroySwapchainKHR)   GetDeviceProcAddr( hLogicalDevice, "vkDestroySwapchainKHR" );
                Ext.GetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR) GetDeviceProcAddr( hLogicalDevice, "vkGetSwapchainImagesKHR" );
                Ext.AcquireNextImageKHR   = (PFN_vkAcquireNextImageKHR)   GetDeviceProcAddr( hLogicalDevice, "vkAcquireNextImageKHR" );
                Ext.QueuePresentKHR       = (PFN_vkQueuePresentKHR)       GetDeviceProcAddr( hLogicalDevice, "vkQueuePresentKHR" );

                if ( Ext.bDisplayTimingGOOGLE ) {
                    Ext.GetRefreshCycleDurationGOOGLE     = (PFN_vkGetRefreshCycleDurationGOOGLE)     GetDeviceProcAddr( hLogicalDevice, "vkGetRefreshCycleDurationGOOGLE" );
                    Ext.GetPastPresentationTimingGOOGLE   = (PFN_vkGetPastPresentationTimingGOOGLE)   GetDeviceProcAddr( hLogicalDevice, "vkGetPastPresentationTimingGOOGLE" );
                }

                VmaAllocatorCreateInfo allocatorCreateInfo = TNewInitializedStruct< VmaAllocatorCreateInfo >( );
                allocatorCreateInfo.physicalDevice         = pPhysicalDevice;
                allocatorCreateInfo.device                 = hLogicalDevice;
                allocatorCreateInfo.pAllocationCallbacks   = GetAllocationCallbacks( );

                if ( !hAllocator.Recreate( allocatorCreateInfo ) )
                    return false;

                if ( !Queues.Inititalize( this,
                                          QueueProps.data( ),
                                          QueueProps.data( ) + QueueProps.size( ),
                                          QueuePriorities.data( ),
                                          QueuePriorities.data( ) + QueuePriorities.size( ) ) )
                    return false;

                // clang-format off
                if ( !CmdBuffers.Inititalize( this,
                                              QueueProps.data( ),
                                              QueueProps.data( ) + QueueProps.size( ) ) )
                    return false;
                // clang-format on
            }

            return true;
        }
    }

    return false;
}

apemodevk::GraphicsDevice::GraphicsDevice( ) {
}

apemodevk::GraphicsDevice::~GraphicsDevice( ) {
}

void apemodevk::GraphicsDevice::Destroy( ) {
    if ( hLogicalDevice ) {
        Queues.Destroy( );
        CmdBuffers.Destroy( );
        hAllocator.Destroy( );
        hLogicalDevice.Destroy( );
    }
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
    return VK_SUCCESS == CheckedResult( vkDeviceWaitIdle( hLogicalDevice ) );
}

apemodevk::QueuePool* apemodevk::GraphicsDevice::GetQueuePool( ) {
    return &Queues;
}

const apemodevk::QueuePool* apemodevk::GraphicsDevice::GetQueuePool( ) const {
    return &Queues;
}

apemodevk::CommandBufferPool* apemodevk::GraphicsDevice::GetCommandBufferPool( ) {
    return &CmdBuffers;
}

const apemodevk::CommandBufferPool* apemodevk::GraphicsDevice::GetCommandBufferPool( ) const {
    return &CmdBuffers;
}

#pragma warning(push, 4)
#pragma warning(disable: 4127) // warning C4127: conditional expression is constant
#pragma warning(disable: 4100) // warning C4100: '...': unreferenced formal parameter
#pragma warning(disable: 4189) // warning C4189: '...': local variable is initialized but not referenced
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#pragma warning(pop)