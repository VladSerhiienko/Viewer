#include <GraphicsDevice.Vulkan.h>
#include <GraphicsManager.Vulkan.h>
#include <NativeHandles.Vulkan.h>
#include <TInfoStruct.Vulkan.h>

#include <stdlib.h>

apemodevk::GraphicsManager::APIVersion::APIVersion( )
    : Major( VK_API_VERSION_1_0 >> 22 )
    , Minor( ( VK_API_VERSION_1_0 >> 12 ) & 0x3ff )
    , Patch( VK_API_VERSION_1_0 & 0xfff ) {
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

void apemodevk::GraphicsManager::Destroy( ) {
    PrimaryNode   = nullptr;
    SecondaryNode = nullptr;
    hInstance.Destroy( );
}

/*
 * Return 1 (true) if all layer names specified in check_names
 * can be found in given layer properties.
 */
static VkBool32 CheckLayers( uint32_t check_count, const char** check_names, uint32_t layer_count, VkLayerProperties* layers ) {

    for ( uint32_t i = 0; i < check_count; i++ ) {

        VkBool32 found = 0;
        for ( uint32_t j = 0; j < layer_count; j++ ) {
            if ( !strcmp( check_names[ i ], layers[ j ].layerName ) ) {
                found = 1;
                break;
            }
        }

        if ( !found ) {
            return 0;
        }
    }
    return 1;
}

bool EnumerateLayersAndExtensions( bool                        bValidate,
                                   std::vector< const char* >& layer_names,
                                   std::vector< const char* >& extension_names ) {
    VkResult err;

    const char* instance_validation_layers_alt1[] = {"VK_LAYER_LUNARG_standard_validation"};

    const char* instance_validation_layers_alt2[] = {"VK_LAYER_GOOGLE_threading",
                                                     "VK_LAYER_LUNARG_parameter_validation",
                                                     "VK_LAYER_LUNARG_object_tracker",
                                                     "VK_LAYER_LUNARG_core_validation",
                                                     "VK_LAYER_GOOGLE_unique_objects"};

    /* Look for validation layers */
    VkBool32 validation_found = 0;
    if ( bValidate ) {

        uint32_t instance_layer_count = 0;
        err = vkEnumerateInstanceLayerProperties( &instance_layer_count, NULL );
        if ( err )
            return false;

        if ( instance_layer_count > 0 ) {

            std::vector< VkLayerProperties > instance_layers;
            instance_layers.resize( instance_layer_count );

            err = vkEnumerateInstanceLayerProperties( &instance_layer_count, instance_layers.data( ) );
            if ( err )
                return false;

            validation_found = CheckLayers( apemodevk::utils::GetArraySizeU( instance_validation_layers_alt1 ),
                                            instance_validation_layers_alt1,
                                            instance_layer_count,
                                            instance_layers.data( ) );
            if ( validation_found ) {
                for ( auto n : instance_validation_layers_alt1 ) {
                    layer_names.push_back( n );
                }
            } else {
                validation_found = CheckLayers( apemodevk::utils::GetArraySizeU( instance_validation_layers_alt2 ),
                                                instance_validation_layers_alt2,
                                                instance_layer_count,
                                                instance_layers.data( ) );
                if ( validation_found ) {
                    for ( auto n : instance_validation_layers_alt2 ) {
                        layer_names.push_back( n );
                    }
                }
            }
        }

        if ( !validation_found ) {
           return false;
        }
    }

    /* Look for instance extensions */
    VkBool32 surfaceExtFound = 0;
    VkBool32 platformSurfaceExtFound = 0;

    uint32_t instance_extension_count = 0;
    err = vkEnumerateInstanceExtensionProperties( NULL, &instance_extension_count, NULL );
    if ( err )
        return false;

    if ( instance_extension_count > 0 ) {
        std::vector< VkExtensionProperties > instance_extensions;
        instance_extensions.resize( instance_extension_count );

        err = vkEnumerateInstanceExtensionProperties( NULL, &instance_extension_count, instance_extensions.data( ) );
        if ( err )
            return false;

        for ( uint32_t i = 0; i < instance_extension_count; i++ ) {
            if ( !strcmp( VK_KHR_SURFACE_EXTENSION_NAME, instance_extensions[ i ].extensionName ) ) {
                surfaceExtFound = 1;
                extension_names.push_back( VK_KHR_SURFACE_EXTENSION_NAME );
            }

#if defined( VK_USE_PLATFORM_WIN32_KHR )
            if ( !strcmp( VK_KHR_WIN32_SURFACE_EXTENSION_NAME, instance_extensions[ i ].extensionName ) ) {
                platformSurfaceExtFound = 1;
                extension_names.push_back( VK_KHR_WIN32_SURFACE_EXTENSION_NAME );
            }
#elif defined( VK_USE_PLATFORM_XLIB_KHR )
            if ( !strcmp( VK_KHR_XLIB_SURFACE_EXTENSION_NAME, instance_extensions[ i ].extensionName ) ) {
                platformSurfaceExtFound = 1;
                extension_names.push_back( VK_KHR_XLIB_SURFACE_EXTENSION_NAME );
            }
#elif defined( VK_USE_PLATFORM_XCB_KHR )
            if ( !strcmp( VK_KHR_XCB_SURFACE_EXTENSION_NAME, instance_extensions[ i ].extensionName ) ) {
                platformSurfaceExtFound = 1;
                extension_names.push_back( VK_KHR_XCB_SURFACE_EXTENSION_NAME );
            }
#elif defined( VK_USE_PLATFORM_WAYLAND_KHR )
            if ( !strcmp( VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME, instance_extensions[ i ].extensionName ) ) {
                platformSurfaceExtFound = 1;
                extension_names.push_back( VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME );
            }
#elif defined( VK_USE_PLATFORM_DISPLAY_KHR )
            if ( !strcmp( VK_KHR_DISPLAY_EXTENSION_NAME, instance_extensions[ i ].extensionName ) ) {
                platformSurfaceExtFound = 1;
                extension_names.push_back( VK_KHR_DISPLAY_EXTENSION_NAME );
            }
#elif defined( VK_USE_PLATFORM_ANDROID_KHR )
            if ( !strcmp( VK_KHR_ANDROID_SURFACE_EXTENSION_NAME, instance_extensions[ i ].extensionName ) ) {
                platformSurfaceExtFound = 1;
                extension_names.push_back( VK_KHR_ANDROID_SURFACE_EXTENSION_NAME );
            }
#elif defined( VK_USE_PLATFORM_IOS_MVK )
            if ( !strcmp( VK_MVK_IOS_SURFACE_EXTENSION_NAME, instance_extensions[ i ].extensionName ) ) {
                platformSurfaceExtFound = 1;
                extension_names.push_back( VK_MVK_IOS_SURFACE_EXTENSION_NAME );
            }
#elif defined( VK_USE_PLATFORM_MACOS_MVK )
            if ( !strcmp( VK_MVK_MACOS_SURFACE_EXTENSION_NAME, instance_extensions[ i ].extensionName ) ) {
                platformSurfaceExtFound = 1;
                extension_names.push_back( VK_MVK_MACOS_SURFACE_EXTENSION_NAME );
            }
#elif defined( VK_USE_PLATFORM_MIR_KHR )
#endif

            if ( !strcmp( VK_EXT_DEBUG_REPORT_EXTENSION_NAME, instance_extensions[ i ].extensionName ) ) {
                if ( bValidate ) {
                    extension_names.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
                }
            }

            if ( !strcmp( VK_EXT_DEBUG_UTILS_EXTENSION_NAME, instance_extensions[ i ].extensionName ) ) {
                if ( bValidate ) {
                    extension_names.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
                }
            }
        }
    }

    if ( !surfaceExtFound ) {
        return false;
    }

    if ( !platformSurfaceExtFound ) {
        return false;
    }

    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessengerCallback( VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                       VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                                       const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                       void*                                       pUserData ) {
    return false;
}

bool apemodevk::GraphicsManager::RecreateGraphicsNodes( uint32_t                      flags,
                                                        std::unique_ptr< IAllocator > pInAllocator,
                                                        std::unique_ptr< ILogger >    pInLogger,
                                                        const char*                   pszAppName,
                                                        const char*                   pszEngineName ) {
    pAllocator = std::move( pInAllocator );
    pLogger    = std::move( pInLogger );

    bool                       bValidate = false;
    std::vector< const char* > instanceLayers;
    std::vector< const char* > instanceExtensions;

    if ( !EnumerateLayersAndExtensions( bValidate, instanceLayers, instanceExtensions ) )
        return false;

    VkApplicationInfo applicationInfo;
    InitializeStruct( applicationInfo );
    applicationInfo.pNext              = VK_NULL_HANDLE;
    applicationInfo.apiVersion         = VK_API_VERSION_1_0;
    applicationInfo.pApplicationName   = pszAppName;
    applicationInfo.pEngineName        = pszEngineName;
    applicationInfo.applicationVersion = 1;
    applicationInfo.engineVersion      = 1;

    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT;
    InitializeStruct( debugUtilsMessengerCreateInfoEXT );
    debugUtilsMessengerCreateInfoEXT.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugUtilsMessengerCreateInfoEXT.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugUtilsMessengerCreateInfoEXT.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                                   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                                   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugUtilsMessengerCreateInfoEXT.pfnUserCallback = DebugMessengerCallback;

    VkInstanceCreateInfo instanceCreateInfo;
    InitializeStruct( instanceCreateInfo );
    instanceCreateInfo.pApplicationInfo        = &applicationInfo;
    instanceCreateInfo.enabledLayerCount       = GetSizeU( instanceLayers );
    instanceCreateInfo.ppEnabledLayerNames     = instanceLayers.data( );
    instanceCreateInfo.enabledExtensionCount   = GetSizeU( instanceExtensions );
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data( );

    /* This is info for a temp callback to use during CreateInstance.
     *  After the instance is created, we use the instance-based
     * function to register the final callback. */
    if ( bValidate ) {
        // VK_EXT_debug_utils style
        instanceCreateInfo.pNext = &debugUtilsMessengerCreateInfoEXT;
    }

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

    if ( bIsOk && bValidate ) {

        // clang-format off
        // Setup VK_EXT_debug_utils function pointers always (we use them for debug labels and names).
        Funcs.CreateDebugUtilsMessengerEXT  = (PFN_vkCreateDebugUtilsMessengerEXT)  vkGetInstanceProcAddr( hInstance, "vkCreateDebugUtilsMessengerEXT" );
        Funcs.DestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr( hInstance, "vkDestroyDebugUtilsMessengerEXT" );
        Funcs.SubmitDebugUtilsMessageEXT    = (PFN_vkSubmitDebugUtilsMessageEXT)    vkGetInstanceProcAddr( hInstance, "vkSubmitDebugUtilsMessageEXT" );
        Funcs.CmdBeginDebugUtilsLabelEXT    = (PFN_vkCmdBeginDebugUtilsLabelEXT)    vkGetInstanceProcAddr( hInstance, "vkCmdBeginDebugUtilsLabelEXT" );
        Funcs.CmdEndDebugUtilsLabelEXT      = (PFN_vkCmdEndDebugUtilsLabelEXT)      vkGetInstanceProcAddr( hInstance, "vkCmdEndDebugUtilsLabelEXT" );
        Funcs.CmdInsertDebugUtilsLabelEXT   = (PFN_vkCmdInsertDebugUtilsLabelEXT)   vkGetInstanceProcAddr( hInstance, "vkCmdInsertDebugUtilsLabelEXT" );
        Funcs.SetDebugUtilsObjectNameEXT    = (PFN_vkSetDebugUtilsObjectNameEXT)    vkGetInstanceProcAddr( hInstance, "vkSetDebugUtilsObjectNameEXT" );
        // clang-format on

        if ( NULL == Funcs.CreateDebugUtilsMessengerEXT || NULL == Funcs.DestroyDebugUtilsMessengerEXT ||
             NULL == Funcs.SubmitDebugUtilsMessageEXT || NULL == Funcs.CmdBeginDebugUtilsLabelEXT ||
             NULL == Funcs.CmdEndDebugUtilsLabelEXT || NULL == Funcs.CmdInsertDebugUtilsLabelEXT ||
             NULL == Funcs.SetDebugUtilsObjectNameEXT ) {
            return false;
        }

        switch ( CheckedCall( Funcs.CreateDebugUtilsMessengerEXT( hInstance, &debugUtilsMessengerCreateInfoEXT, NULL, &Funcs.DebugUtilsMessengerEXT ) ) ) {
            case VK_SUCCESS:
                break;
            default:
                return false;
        }
    }

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

// VKAPI_ATTR VkBool32 VKAPI_CALL apemodevk::GraphicsManager::DebugCallback( VkFlags                    msgFlags,
//                                                                           VkDebugReportObjectTypeEXT objType,
//                                                                           uint64_t                   srcObject,
//                                                                           size_t                     location,
//                                                                           int32_t                    msgCode,
//                                                                           const char *               pLayerPrefix,
//                                                                           const char *               pMsg,
//                                                                           void *                     pUserData ) {
//     platform::DebugTrace( platform::LogLevel::Warn,
//                           "[vk-debug-cb] [%s] %s msg: %s, tobj: %s, obj: %llu, location: %zu, msgcode: %i",
//                           pLayerPrefix,
//                           ToStringFlags( msgFlags ).c_str(),
//                           pMsg,
//                           ToString( objType ),
//                           srcObject,
//                           location,
//                           msgCode );

//     if ( msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT ) {

//         if ( nullptr == strstr( pMsg, "Nvda.Graphics.Interception" ) )
//             platform::DebugBreak( );

//     } else if ( msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT ) {
//         //platform::DebugBreak( );
//     } else if ( msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT ) {
//     } else if ( msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT ) {
//     }

//     /* False indicates that layer should not bail-out of an
//      * API call that had validation failures. This may mean that the
//      * app dies inside the driver due to invalid parameter(s).
//      * That's what would happen without validation layers, so we'll
//      * keep that behavior here.
//      */

//     return false;
// }

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