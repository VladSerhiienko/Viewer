//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <GraphicsDevice.Vulkan.h>
#include <GraphicsManager.Vulkan.h>

#include <NativeHandles.Vulkan.h>

#include <TInfoStruct.Vulkan.h>

#include <GraphicsManager.KnownExtensions.Vulkan.h>
#include <GraphicsManager.KnownLayers.Vulkan.h>

#include <stdlib.h>

/// -------------------------------------------------------------------------------------------------------------------
/// GraphicsEcosystem PrivateContent
/// -------------------------------------------------------------------------------------------------------------------

apemodevk::GraphicsManager::APIVersion::APIVersion( )
    : Major( VK_API_VERSION_1_0 >> 22 )
    , Minor( ( VK_API_VERSION_1_0 >> 12 ) & 0x3ff )
    , Patch( VK_API_VERSION_1_0 & 0xfff ) {
}

bool apemodevk::GraphicsManager::ScanInstanceLayerProperties( uint32_t flags ) {
    struct SpecialLayerOrExtension {
        uint32_t    eFlag;
        const char* pName;
    };

    InstanceLayers.clear( );
    InstanceLayerProps.clear( );
    InstanceExtensions.clear( );
    InstanceExtensionProps.clear( );

    uint32_t PropCount = 0;
    uint32_t LayerCount = 0;

    switch ( CheckedCall( vkEnumerateInstanceLayerProperties( &LayerCount, NULL ) ) ) {
        case VK_SUCCESS:
            InstanceLayerProps.resize( LayerCount );
            CheckedCall( vkEnumerateInstanceLayerProperties( &LayerCount, InstanceLayerProps.data( ) ) );
            break;

        default:
            return false;
    }

    InstanceLayers.reserve( LayerCount + 1 );
    InstanceLayers.push_back( nullptr );
    std::transform( InstanceLayerProps.begin( ),
                    InstanceLayerProps.end( ),
                    std::back_inserter( InstanceLayers ),
                    [&]( VkLayerProperties const& ExtProp ) { return ExtProp.layerName; } );

    for ( SpecialLayerOrExtension specialLayer : {
              SpecialLayerOrExtension{kEnable_LUNARG_vktrace, "VK_LAYER_LUNARG_vktrace"},
              SpecialLayerOrExtension{kEnable_LUNARG_api_dump, "VK_LAYER_LUNARG_api_dump"},
              SpecialLayerOrExtension{kEnable_RENDERDOC_Capture, "VK_LAYER_RENDERDOC_Capture"},
              /*SpecialLayerOrExtension{kEnable_RENDERDOC_Capture, "VK_LAYER_LUNARG_core_validation"},
              SpecialLayerOrExtension{kEnable_RENDERDOC_Capture, "VK_LAYER_LUNARG_monitor"},
              SpecialLayerOrExtension{kEnable_RENDERDOC_Capture, "VK_LAYER_LUNARG_object_tracker"},
              SpecialLayerOrExtension{kEnable_RENDERDOC_Capture, "VK_LAYER_LUNARG_parameter_validation"},
              SpecialLayerOrExtension{kEnable_RENDERDOC_Capture, "VK_LAYER_LUNARG_screenshot"},
              SpecialLayerOrExtension{kEnable_RENDERDOC_Capture, "VK_LAYER_LUNARG_standard_validation"},*/
              SpecialLayerOrExtension{8, "VK_LAYER_GOOGLE_threading"},
              SpecialLayerOrExtension{8, "VK_LAYER_GOOGLE_unique_objects"},
              SpecialLayerOrExtension{8, "VK_LAYER_NV_optimus"},
              SpecialLayerOrExtension{8, "VK_LAYER_NV_nsight"},
          } ) {
        if ( false == HasFlagEq( flags, specialLayer.eFlag ) ) {
            auto specialLayerIt = std::find_if( ++InstanceLayers.begin( ), InstanceLayers.end( ), [&]( const char* pName ) {
                return 0 == strcmp( pName, specialLayer.pName );
            } );

            if ( specialLayerIt != InstanceLayers.end( ) )
                InstanceLayers.erase( specialLayerIt );
        }
    }

    for ( auto pInstanceLayer : InstanceLayers ) {
        switch ( CheckedCall( vkEnumerateInstanceExtensionProperties( pInstanceLayer, &PropCount, NULL ) ) ) {
            case VK_SUCCESS: {
                uint32_t FirstInstanceExtension = static_cast< uint32_t >( InstanceExtensionProps.size( ) );
                InstanceExtensionProps.resize( InstanceExtensionProps.size( ) + PropCount );
                CheckedCall( vkEnumerateInstanceExtensionProperties( pInstanceLayer, &PropCount, InstanceExtensionProps.data( ) + FirstInstanceExtension ) );
            } break;

            default:
                return false;
        }
    }

    std::sort( InstanceExtensionProps.begin( ),
               InstanceExtensionProps.end( ),
               [&]( VkExtensionProperties const& a, VkExtensionProperties const& b ) {
                   return 0 > strcmp( a.extensionName, b.extensionName );
               } );
    InstanceExtensionProps.erase( std::unique( InstanceExtensionProps.begin( ),
                                               InstanceExtensionProps.end( ),
                                               [&]( VkExtensionProperties const& a, VkExtensionProperties const& b ) {
                                                   return 0 == strcmp( a.extensionName, b.extensionName );
                                               } ),
                                  InstanceExtensionProps.end( ) );

    InstanceLayers.erase( InstanceLayers.begin( ) );

    platform::DebugTrace( platform::LogLevel::Info, "Instance Layers (%u):", InstanceLayers.size( ) );
    std::for_each( InstanceLayers.begin( ), InstanceLayers.end( ), [&]( const char* pName ) {
        platform::DebugTrace( platform::LogLevel::Info, "> %s", pName );
    } );

    InstanceExtensions.reserve( InstanceExtensionProps.size( ) );
    std::transform( InstanceExtensionProps.begin( ),
                    InstanceExtensionProps.end( ),
                    std::back_inserter( InstanceExtensions ),
                    [&]( VkExtensionProperties const& ExtProp ) { return ExtProp.extensionName; } );

    /*
        APEMODE VK: > VK_EXT_debug_report
        APEMODE VK: > VK_EXT_display_surface_counter
        APEMODE VK: > VK_KHR_get_physical_device_properties2
        APEMODE VK: > VK_KHR_get_surface_capabilities2
        APEMODE VK: > VK_KHR_surface
        APEMODE VK: > VK_KHR_win32_surface
        APEMODE VK: > VK_KHX_device_group_creation
        APEMODE VK: > VK_NV_external_memory_capabilities
    */

    for ( SpecialLayerOrExtension specialLayer : {SpecialLayerOrExtension{8, "VK_NV_external_memory_capabilities"},
                                                  SpecialLayerOrExtension{8, "VK_KHX_device_group_creation"},
                                                  //SpecialLayerOrExtension{8, "VK_KHR_win32_surface"},
                                                  //SpecialLayerOrExtension{8, "VK_KHR_surface"},
                                                  SpecialLayerOrExtension{8, "VK_KHR_get_surface_capabilities2"},
                                                  SpecialLayerOrExtension{8, "VK_KHR_get_physical_device_properties2"},
                                                  SpecialLayerOrExtension{8, "VK_EXT_display_surface_counter"}} ) {
        if ( false == HasFlagEq( flags, specialLayer.eFlag ) ) {
            auto specialLayerIt = std::find_if( InstanceExtensions.begin( ),
                                                InstanceExtensions.end( ),
                                                [&]( const char* pName ) { return 0 == strcmp( pName, specialLayer.pName ); } );

            if ( specialLayerIt != InstanceExtensions.end( ) )
                InstanceExtensions.erase( specialLayerIt );
        }
    }

    platform::DebugTrace( platform::LogLevel::Info, "Instance Extensions (%u):", InstanceExtensions.size( ) );
    std::for_each( InstanceExtensions.begin( ), InstanceExtensions.end( ), [&]( const char* pName ) {
        platform::DebugTrace( platform::LogLevel::Info, "> %s", pName );
    } );

    return true;
}

bool apemodevk::GraphicsManager::ScanAdapters( uint32_t flags ) {

    uint32_t adaptersFound = 0;
    CheckedCall( vkEnumeratePhysicalDevices( hInstance, &adaptersFound, NULL ) );

    std::vector< VkPhysicalDevice > adapters( adaptersFound );
    CheckedCall( vkEnumeratePhysicalDevices( hInstance, &adaptersFound, adapters.data( ) ) );

    // TODO: Choose the best 2 nodes here.
    //       Ensure the integrated GPU is always the secondary one.

    std::for_each( adapters.begin( ), adapters.end( ), [&]( VkPhysicalDevice const &adapter ) {
        GraphicsDevice *pCurrentNode = nullptr;

        if ( GetPrimaryGraphicsNode( ) == nullptr ) {
            PrimaryNode.reset( new GraphicsDevice( ) );
            pCurrentNode = GetPrimaryGraphicsNode( );
        } else if ( GetSecondaryGraphicsNode( ) == nullptr ) {
            SecondaryNode.reset( new GraphicsDevice( ) );
            pCurrentNode = GetSecondaryGraphicsNode( );
        }

        if ( pCurrentNode != nullptr ) {
            pCurrentNode->RecreateResourcesFor( adapter, flags );
        }
    } );

    return adaptersFound != 0;
}

apemodevk::GraphicsManager::NativeLayerWrapper &apemodevk::GraphicsManager::GetUnnamedLayer( ) {
    apemode_assert( LayerWrappers.front( ).IsUnnamedLayer( ), "vkEnumerateInstanceExtensionProperties failed." );
    return LayerWrappers.front( );
}

bool apemodevk::GraphicsManager::RecreateGraphicsNodes( uint32_t                      flags,
                                                        std::unique_ptr< IAllocator > pInAllocator,
                                                        std::unique_ptr< ILogger >    pInLogger ) {
    pAllocator = std::move( pInAllocator );
    pLogger    = std::move( pInLogger );

    if ( !ScanInstanceLayerProperties( flags ) )
        return false;

    VkApplicationInfo applicationInfo;
    InitializeStruct( applicationInfo );
    applicationInfo.pNext              = VK_NULL_HANDLE;
    applicationInfo.apiVersion         = VK_API_VERSION_1_0;
    applicationInfo.pApplicationName   = AppName.c_str( );
    applicationInfo.pEngineName        = EngineName.c_str( );
    applicationInfo.applicationVersion = 1;
    applicationInfo.engineVersion      = 1;

    // clang-format off
    auto debugFlags
        = VK_DEBUG_REPORT_ERROR_BIT_EXT
        | VK_DEBUG_REPORT_DEBUG_BIT_EXT
        | VK_DEBUG_REPORT_WARNING_BIT_EXT
        | VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
    // clang-format on

    VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo;
    InitializeStruct( debugReportCallbackCreateInfo );
    debugReportCallbackCreateInfo.pfnCallback = DebugCallback;
    debugReportCallbackCreateInfo.pUserData   = this;
    debugReportCallbackCreateInfo.sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    debugReportCallbackCreateInfo.flags       = debugFlags;

    VkInstanceCreateInfo instanceCreateInfo;
    InitializeStruct( instanceCreateInfo );
    instanceCreateInfo.pNext                   = &debugReportCallbackCreateInfo;
    instanceCreateInfo.pApplicationInfo        = &applicationInfo;
    instanceCreateInfo.enabledLayerCount       = GetSizeU( InstanceLayers );
    instanceCreateInfo.ppEnabledLayerNames     = InstanceLayers.data( );
    instanceCreateInfo.enabledExtensionCount   = GetSizeU( InstanceExtensions );
    instanceCreateInfo.ppEnabledExtensionNames = InstanceExtensions.data( );

    /*
    Those layers cannot be used as standalone layers (please, see vktrace, renderdoc docs)
    -----------------------------------------------------------------
        Layer VK_LAYER_LUNARG_vktrace (impl 0x00000001, spec 0x00400003):
                Extension VK_KHR_surface (spec 0x00000019)
                Extension VK_KHR_win32_surface (spec 0x00000005)
                Extension VK_EXT_debug_report (spec 0x00000001)
    -----------------------------------------------------------------
        Layer VK_LAYER_RENDERDOC_Capture (impl 0x0000001a, spec 0x000d2001):
                Extension VK_KHR_surface (spec 0x00000019)
                Extension VK_KHR_win32_surface (spec 0x00000005)
                Extension VK_EXT_debug_report (spec 0x00000001)
    -----------------------------------------------------------------
    */

    bool bIsOk = hInstance.Recreate( instanceCreateInfo );
    apemode_assert( bIsOk, "vkCreateInstance failed." );

    if ( !ScanAdapters( flags ) )
        return false;

    return true;
}

const char * ToString( VkDebugReportObjectTypeEXT debugReportObjectTypeEXT )
{
    switch ( debugReportObjectTypeEXT ) {
        case VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_KHR_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_KHR_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_MODE_KHR_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_MODE_KHR_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_OBJECT_TABLE_NVX_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_OBJECT_TABLE_NVX_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX_EXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_KHR_EXT:
            return "VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_KHR_EXT";
        default:
            return "?";
    }
}

std::string ToStringFlags( VkFlags flags )
{
    std::stringstream ss;

    ss << "[";
    if ( ( flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT ) == VK_DEBUG_REPORT_INFORMATION_BIT_EXT ) {
        ss << "info|";
    }
    if ( ( flags & VK_DEBUG_REPORT_WARNING_BIT_EXT ) == VK_DEBUG_REPORT_WARNING_BIT_EXT ) {
        ss << "warn|";
    }
    if ( ( flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT ) == VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT ) {
        ss << "perf|";
    }
    if ( ( flags & VK_DEBUG_REPORT_ERROR_BIT_EXT ) == VK_DEBUG_REPORT_ERROR_BIT_EXT ) {
        ss << "err|";
    }
    if ( ( flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT ) == VK_DEBUG_REPORT_DEBUG_BIT_EXT ) {
        ss << "debug|";
    }

    ss.seekp( ss.tellp( ) - std::stringstream::pos_type( 1 ), ss.beg );
    ss << "]";

    return ss.str( );
}

VKAPI_ATTR VkBool32 VKAPI_CALL apemodevk::GraphicsManager::DebugCallback( VkFlags                    msgFlags,
                                                                          VkDebugReportObjectTypeEXT objType,
                                                                          uint64_t                   srcObject,
                                                                          size_t                     location,
                                                                          int32_t                    msgCode,
                                                                          const char *               pLayerPrefix,
                                                                          const char *               pMsg,
                                                                          void *                     pUserData ) {
    platform::DebugTrace( platform::LogLevel::Warn,
                          "[vk-debug-cb] [%s] %s msg: %s, tobj: %s, obj: %llu, location: %zu, msgcode: %i",
                          pLayerPrefix,
                          ToStringFlags( msgFlags ).c_str(),
                          pMsg,
                          ToString( objType ),
                          srcObject,
                          location,
                          msgCode );

    if ( msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT ) {

        if ( nullptr == strstr( pMsg, "Nvda.Graphics.Interception" ) )
            platform::DebugBreak( );

    } else if ( msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT ) {
        platform::DebugBreak( );
    } else if ( msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT ) {
    } else if ( msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT ) {
    }

    /* False indicates that layer should not bail-out of an
     * API call that had validation failures. This may mean that the
     * app dies inside the driver due to invalid parameter(s).
     * That's what would happen without validation layers, so we'll
     * keep that behavior here.
     */

    return false;
}

bool apemodevk::GraphicsManager::NativeLayerWrapper::IsUnnamedLayer( ) const {
    return bIsUnnamed;
}

bool apemodevk::GraphicsManager::NativeLayerWrapper::IsValidInstanceLayer( ) const {
    if ( IsUnnamedLayer( ) ) {

        auto const pKnownExtensionBeginIt = Vulkan::KnownInstanceExtensions;
        auto const pKnownExtensionEndIt   = Vulkan::KnownInstanceExtensions + Vulkan::KnownInstanceExtensionCount;

        size_t knownExtensionsFound = std::count_if( pKnownExtensionBeginIt, pKnownExtensionEndIt, [this]( char const *KnownExtension ) {

            auto const pExtensionBeginIt = Extensions.begin( );
            auto const pExtensionEndIt   = Extensions.end( );

            auto const pFoundExtensionIt = std::find_if( pExtensionBeginIt, pExtensionEndIt, [&]( VkExtensionProperties const& Extension ) {
                return strcmp( KnownExtension, Extension.extensionName ) == 0;
            } );

            bool const bIsExtensionFound = pFoundExtensionIt != pExtensionEndIt;
            apemode_assert( bIsExtensionFound, "Extension '%s' was not found.", KnownExtension );

            return bIsExtensionFound;
        } );

        return knownExtensionsFound == Vulkan::KnownInstanceExtensionCount;
    }

    return true;
}

/// -------------------------------------------------------------------------------------------------------------------
/// GraphicsManager
/// -------------------------------------------------------------------------------------------------------------------

apemodevk::GraphicsManager* apemodevk::GetGraphicsManager( ) {
    static apemodevk::GraphicsManager graphicsManagerInstance;
    return &graphicsManagerInstance;
}

const VkAllocationCallbacks* apemodevk::GetAllocationCallbacks( ) {
    return GetGraphicsManager( )->GetAllocationCallbacks( );
}

apemodevk::GraphicsManager::GraphicsManager( ) {
}

apemodevk::GraphicsManager::~GraphicsManager( ) {
}

apemodevk::GraphicsManager::IAllocator* apemodevk::GraphicsManager::GetAllocator( ) {
    return pAllocator.get( );
}

apemodevk::GraphicsManager::ILogger* apemodevk::GraphicsManager::GetLogger( ) {
    return pLogger.get( );
}

const VkAllocationCallbacks* apemodevk::GraphicsManager::GetAllocationCallbacks( ) const {
    static VkAllocationCallbacks allocationCallbacks{nullptr,
                                                     &AllocationCallbacks::AllocationFunction,
                                                     &AllocationCallbacks::ReallocationFunction,
                                                     &AllocationCallbacks::FreeFunction,
                                                     &AllocationCallbacks::InternalAllocationNotification,
                                                     &AllocationCallbacks::InternalFreeNotification};
    return &allocationCallbacks;
}

apemodevk::GraphicsDevice *apemodevk::GraphicsManager::GetPrimaryGraphicsNode( ) {
    return PrimaryNode.get( );
}

apemodevk::GraphicsDevice *apemodevk::GraphicsManager::GetSecondaryGraphicsNode( ) {
    return SecondaryNode.get( );
}

void* apemodevk::GraphicsManager::AllocationCallbacks::AllocationFunction( void*,
                                                                           size_t                  size,
                                                                           size_t                  alignment,
                                                                           VkSystemAllocationScope allocationScope ) {
    return GetGraphicsManager( )->GetAllocator( )->Malloc( size, alignment, __FILE__, __LINE__, __FUNCTION__ );
}

void* apemodevk::GraphicsManager::AllocationCallbacks::ReallocationFunction( void*,
                                                                             void*                   pOriginal,
                                                                             size_t                  size,
                                                                             size_t                  alignment,
                                                                             VkSystemAllocationScope allocationScope ) {
    return GetGraphicsManager( )->GetAllocator( )->Realloc( pOriginal, size, alignment, __FILE__, __LINE__, __FUNCTION__ );
}

void apemodevk::GraphicsManager::AllocationCallbacks::FreeFunction( void*,
                                                                    void* pMemory ) {
    return GetGraphicsManager( )->GetAllocator( )->Free( pMemory, __FILE__, __LINE__, __FUNCTION__ );
}

const char* ToString( VkInternalAllocationType allocationType ) {
    switch ( allocationType ) {
        case VK_INTERNAL_ALLOCATION_TYPE_EXECUTABLE:
            return "VK_INTERNAL_ALLOCATION_TYPE_EXECUTABLE";
        default:
            return "?";
    }
}

const char* ToString( VkSystemAllocationScope allocationScope ) {
    switch ( allocationScope ) {
        case VK_SYSTEM_ALLOCATION_SCOPE_COMMAND:
            return "VK_SYSTEM_ALLOCATION_SCOPE_COMMAND";
        case VK_SYSTEM_ALLOCATION_SCOPE_OBJECT:
            return "VK_SYSTEM_ALLOCATION_SCOPE_OBJECT";
        case VK_SYSTEM_ALLOCATION_SCOPE_CACHE:
            return "VK_SYSTEM_ALLOCATION_SCOPE_CACHE";
        case VK_SYSTEM_ALLOCATION_SCOPE_DEVICE:
            return "VK_SYSTEM_ALLOCATION_SCOPE_DEVICE";
        case VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE:
            return "VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE";
        default:
            return "?";
    }
}

void apemodevk::GraphicsManager::AllocationCallbacks::InternalAllocationNotification( void*,
                                                                                      size_t                   size,
                                                                                      VkInternalAllocationType allocationType,
                                                                                      VkSystemAllocationScope  allocationScope ) {
    platform::DebugTrace( platform::LogLevel::Debug,
                          "[vk-internal-notification] allocated: %uz, type: %s, scope: %s",
                          size,
                          ToString( allocationType ),
                          ToString( allocationScope ) );
}

void apemodevk::GraphicsManager::AllocationCallbacks::InternalFreeNotification( void*,
                                                                                size_t                   size,
                                                                                VkInternalAllocationType allocationType,
                                                                                VkSystemAllocationScope  allocationScope ) {
    platform::DebugTrace( platform::LogLevel::Debug,
                          "[vk-internal-notification] freed: %uz, type: %s, scope: %s",
                          size,
                          ToString( allocationType ),
                          ToString( allocationScope ) );
}

void apemodevk::platform::Log( apemodevk::platform::LogLevel level, char const* pszMsg ) {
    GetGraphicsManager( )->GetLogger( )->Log( level, pszMsg );
}