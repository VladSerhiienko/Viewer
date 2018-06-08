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

void apemodevk::GraphicsManager::Destroy( ) {
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

void AddName( std::vector< const char* >& names, const char* pszName ) {
    apemodevk_memory_allocation_scope;
    for ( auto& n : names ) {
        if ( !strcmp( n, pszName ) ) {
            return;
        }
    }

    names.push_back( pszName );
}

bool EnumerateLayersAndExtensions( uint32_t                    eFlags,
                                   std::vector< const char* >& OutLayerNames,
                                   std::vector< const char* >& OutExtensionNames,
                                   bool&                       bDebugReport,
                                   bool&                       bDebugMessanger,
                                   const char**                ppszLayers,
                                   size_t                      layerCount,
                                   const char**                ppszExtensions,
                                   size_t                      extensionCount ) {
    VkResult err = VK_SUCCESS;

    OutLayerNames.clear( );
    OutExtensionNames.clear( );

    const bool bValidate = HasFlagEq( eFlags, apemodevk::GraphicsManager::kEnableValidation );

    const char* instance_validation_layers_alt1[] = {"VK_LAYER_LUNARG_standard_validation"};

    const char* instance_validation_layers_alt2[] = {"VK_LAYER_GOOGLE_threading",
                                                     "VK_LAYER_LUNARG_parameter_validation",
                                                     "VK_LAYER_LUNARG_object_tracker",
                                                     "VK_LAYER_LUNARG_core_validation",
                                                     "VK_LAYER_GOOGLE_unique_objects"};

    /* Look for validation layers */
    VkBool32 validation_found = 0;
    if ( bValidate ) {

        uint32_t instanceLayeCount = 0;
        err = vkEnumerateInstanceLayerProperties( &instanceLayeCount, NULL );
        if ( err )
            return false;

        if ( instanceLayeCount > 0 ) {

            std::vector< VkLayerProperties > allInstanceLayers;
            allInstanceLayers.resize( instanceLayeCount );

            err = vkEnumerateInstanceLayerProperties( &instanceLayeCount, allInstanceLayers.data( ) );
            if ( err )
                return false;

            for ( auto& instanceLayerName : allInstanceLayers ) {
                apemodevk::platform::DebugTrace( apemodevk::platform::LogLevel::Debug,
                                                 "> Layer: %s (%u): %s",
                                                 instanceLayerName.layerName,
                                                 instanceLayerName.specVersion,
                                                 instanceLayerName.description );

                for ( uint32_t j = 0; j < layerCount; ++j ) {
                    if ( !strcmp( ppszLayers[ j ], instanceLayerName.layerName ) ) {
                        OutLayerNames.push_back( ppszLayers[ j ] );
                    }
                }
            }

            validation_found = CheckLayers( apemodevk::utils::GetArraySizeU( instance_validation_layers_alt1 ),
                                            instance_validation_layers_alt1,
                                            instanceLayeCount,
                                            allInstanceLayers.data( ) );
            if ( validation_found ) {
                for ( auto n : instance_validation_layers_alt1 ) {
                    AddName( OutLayerNames, n );
                }
            } else {
                validation_found = CheckLayers( apemodevk::utils::GetArraySizeU( instance_validation_layers_alt2 ),
                                                instance_validation_layers_alt2,
                                                instanceLayeCount,
                                                allInstanceLayers.data( ) );
                if ( validation_found ) {
                    for ( auto n : instance_validation_layers_alt2 ) {
                        AddName( OutLayerNames, n );
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
        std::vector< VkExtensionProperties > allInstanceExtensions;
        allInstanceExtensions.resize( instance_extension_count );

        err = vkEnumerateInstanceExtensionProperties( NULL, &instance_extension_count, allInstanceExtensions.data( ) );
        if ( err )
            return false;

        for ( uint32_t i = 0; i < instance_extension_count; i++ ) {

            apemodevk::platform::DebugTrace( apemodevk::platform::LogLevel::Debug,
                                             "> Extension: %s (%u)",
                                             allInstanceExtensions[ i ].extensionName,
                                             allInstanceExtensions[ i ].specVersion );

            for ( uint32_t j = 0; j < extensionCount; ++j ) {
                if ( !strcmp( ppszExtensions[ j ], allInstanceExtensions[ i ].extensionName ) ) {
                    OutExtensionNames.push_back( ppszExtensions[ j ] );
                }
            }

            if ( !strcmp( VK_KHR_SURFACE_EXTENSION_NAME, allInstanceExtensions[ i ].extensionName ) ) {
                surfaceExtFound = 1;
                AddName( OutExtensionNames, VK_KHR_SURFACE_EXTENSION_NAME );
            }

#if defined( VK_USE_PLATFORM_WIN32_KHR )
            if ( !strcmp( VK_KHR_WIN32_SURFACE_EXTENSION_NAME, allInstanceExtensions[ i ].extensionName ) ) {
                platformSurfaceExtFound = 1;
                AddName( OutExtensionNames, VK_KHR_WIN32_SURFACE_EXTENSION_NAME );
            }
#elif defined( VK_USE_PLATFORM_XLIB_KHR )
            if ( !strcmp( VK_KHR_XLIB_SURFACE_EXTENSION_NAME, allInstanceExtensions[ i ].extensionName ) ) {
                platformSurfaceExtFound = 1;
                AddName( OutExtensionNames, VK_KHR_XLIB_SURFACE_EXTENSION_NAME );
            }
#elif defined( VK_USE_PLATFORM_XCB_KHR )
            if ( !strcmp( VK_KHR_XCB_SURFACE_EXTENSION_NAME, allInstanceExtensions[ i ].extensionName ) ) {
                platformSurfaceExtFound = 1;
                AddName( OutExtensionNames, VK_KHR_XCB_SURFACE_EXTENSION_NAME );
            }
#elif defined( VK_USE_PLATFORM_WAYLAND_KHR )
            if ( !strcmp( VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME, allInstanceExtensions[ i ].extensionName ) ) {
                platformSurfaceExtFound = 1;
                AddName( OutExtensionNames, VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME );
            }
#elif defined( VK_USE_PLATFORM_DISPLAY_KHR )
            if ( !strcmp( VK_KHR_DISPLAY_EXTENSION_NAME, allInstanceExtensions[ i ].extensionName ) ) {
                platformSurfaceExtFound = 1;
                AddName( OutExtensionNames, VK_KHR_DISPLAY_EXTENSION_NAME );
            }
#elif defined( VK_USE_PLATFORM_ANDROID_KHR )
            if ( !strcmp( VK_KHR_ANDROID_SURFACE_EXTENSION_NAME, allInstanceExtensions[ i ].extensionName ) ) {
                platformSurfaceExtFound = 1;
                AddName( OutExtensionNames, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME );
            }
#elif defined( VK_USE_PLATFORM_IOS_MVK )
            if ( !strcmp( VK_MVK_IOS_SURFACE_EXTENSION_NAME, allInstanceExtensions[ i ].extensionName ) ) {
                platformSurfaceExtFound = 1;
                AddName( OutExtensionNames, VK_MVK_IOS_SURFACE_EXTENSION_NAME );
            }
#elif defined( VK_USE_PLATFORM_MACOS_MVK )
            if ( !strcmp( VK_MVK_MACOS_SURFACE_EXTENSION_NAME, allInstanceExtensions[ i ].extensionName ) ) {
                platformSurfaceExtFound = 1;
                AddName( OutExtensionNames, VK_MVK_MACOS_SURFACE_EXTENSION_NAME );
            }
#elif defined( VK_USE_PLATFORM_MIR_KHR )
#endif

            if ( !strcmp( VK_EXT_DEBUG_REPORT_EXTENSION_NAME, allInstanceExtensions[ i ].extensionName ) ) {
                if ( bValidate ) {
                    bDebugReport = true;
                    AddName( OutExtensionNames, VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
                }
            }

            if ( !strcmp( VK_EXT_DEBUG_UTILS_EXTENSION_NAME, allInstanceExtensions[ i ].extensionName ) ) {
                if ( bValidate ) {
                    bDebugMessanger = true;
                    AddName( OutExtensionNames, VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
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

VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback( VkFlags                    msgFlags,
                                                    VkDebugReportObjectTypeEXT objType,
                                                    uint64_t                   srcObject,
                                                    size_t                     location,
                                                    int32_t                    msgCode,
                                                    const char*                pLayerPrefix,
                                                    const char*                pMsg,
                                                    void*                      pUserData );

VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessengerCallback( VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                       VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                                       const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                       void*                                       pUserData );

bool apemodevk::GraphicsManager::Initialize( uint32_t                eFlags,
                                             IAllocator*             pInAlloc,
                                             ILogger*                pInLogger,
                                             IMemoryAllocationScope* pInMemoryAllocationScope,
                                             const char*             pszAppName,
                                             const char*             pszEngineName,
                                             const char**            ppszLayers,
                                             size_t                  layerCount,
                                             const char**            ppszExtensions,
                                             size_t                  extensionCount ) {
    pAllocator             = pInAlloc;
    pLogger                = pInLogger;
    pMemoryAllocationScope = pInMemoryAllocationScope;

    AllocCallbacks = VkAllocationCallbacks{this, /* pUserData */
                                           &AllocationCallbacks::AllocationFunction,
                                           &AllocationCallbacks::ReallocationFunction,
                                           &AllocationCallbacks::FreeFunction,
                                           &AllocationCallbacks::InternalAllocationNotification,
                                           &AllocationCallbacks::InternalFreeNotification};

    const bool bValidate = HasFlagEq( eFlags, kEnableValidation );

    std::vector< const char* > instanceLayers;
    std::vector< const char* > instanceExtensions;

    if ( !EnumerateLayersAndExtensions( eFlags,
                                        instanceLayers,
                                        instanceExtensions,
                                        Ext.bDebugReport,
                                        Ext.bDebugMessenger,
                                        ppszLayers,
                                        layerCount,
                                        ppszExtensions,
                                        extensionCount ) )
        return false;

    VkApplicationInfo applicationInfo;
    InitializeStruct( applicationInfo );
    applicationInfo.pNext              = VK_NULL_HANDLE;
    applicationInfo.apiVersion         = VK_API_VERSION_1_0;
    applicationInfo.pApplicationName   = pszAppName;
    applicationInfo.pEngineName        = pszEngineName;
    applicationInfo.applicationVersion = 1;
    applicationInfo.engineVersion      = 1;

    // clang-format off
    VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfoEXT;
    InitializeStruct( debugReportCallbackCreateInfoEXT );
    debugReportCallbackCreateInfoEXT.pfnCallback = DebugReportCallback;
    debugReportCallbackCreateInfoEXT.pUserData   = this;
    debugReportCallbackCreateInfoEXT.sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    debugReportCallbackCreateInfoEXT.flags       = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT |
                                                   VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_INFORMATION_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT;
    InitializeStruct( debugUtilsMessengerCreateInfoEXT );
    debugUtilsMessengerCreateInfoEXT.pfnUserCallback = DebugMessengerCallback;
    debugUtilsMessengerCreateInfoEXT.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugUtilsMessengerCreateInfoEXT.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugUtilsMessengerCreateInfoEXT.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    // clang-format on

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

    if ( bValidate && Ext.bDebugMessenger ) {
        // VK_EXT_debug_utils style
        instanceCreateInfo.pNext = &debugUtilsMessengerCreateInfoEXT;
    } else if ( bValidate && Ext.bDebugReport ) {
        // VK_EXT_debug_report style
        instanceCreateInfo.pNext = &debugReportCallbackCreateInfoEXT;
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

    if ( !hInstance.Recreate( instanceCreateInfo ) ) {
        platform::DebugTrace( platform::LogLevel::Err, "Failed to create instance." );
        return false;
    }

    // clang-format off
    Ext.GetDeviceProcAddr                         = (PFN_vkGetDeviceProcAddr)                         vkGetInstanceProcAddr( hInstance, "vkGetDeviceProcAddr" );
    Ext.GetPhysicalDeviceSurfaceSupportKHR        = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)        vkGetInstanceProcAddr( hInstance, "vkGetPhysicalDeviceSurfaceSupportKHR" );
    Ext.GetPhysicalDeviceSurfaceCapabilitiesKHR   = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)   vkGetInstanceProcAddr( hInstance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR" );
    Ext.GetPhysicalDeviceSurfaceFormatsKHR        = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)        vkGetInstanceProcAddr( hInstance, "vkGetPhysicalDeviceSurfaceFormatsKHR" );
    Ext.GetPhysicalDeviceSurfacePresentModesKHR   = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)   vkGetInstanceProcAddr( hInstance, "vkGetPhysicalDeviceSurfacePresentModesKHR" );
    // clang-format on

    if ( NULL == Ext.GetPhysicalDeviceSurfaceSupportKHR || NULL == Ext.GetPhysicalDeviceSurfaceCapabilitiesKHR ||
         NULL == Ext.GetPhysicalDeviceSurfaceFormatsKHR || NULL == Ext.GetPhysicalDeviceSurfacePresentModesKHR ||
         NULL == Ext.GetDeviceProcAddr ) {
        return false;
    }

    if ( bValidate && Ext.bDebugMessenger ) {

        // clang-format off
        // VK_EXT_debug_utils
        Ext.CreateDebugUtilsMessengerEXT  = (PFN_vkCreateDebugUtilsMessengerEXT)  vkGetInstanceProcAddr( hInstance, "vkCreateDebugUtilsMessengerEXT" );
        Ext.DestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr( hInstance, "vkDestroyDebugUtilsMessengerEXT" );
        Ext.SubmitDebugUtilsMessageEXT    = (PFN_vkSubmitDebugUtilsMessageEXT)    vkGetInstanceProcAddr( hInstance, "vkSubmitDebugUtilsMessageEXT" );
        Ext.CmdBeginDebugUtilsLabelEXT    = (PFN_vkCmdBeginDebugUtilsLabelEXT)    vkGetInstanceProcAddr( hInstance, "vkCmdBeginDebugUtilsLabelEXT" );
        Ext.CmdEndDebugUtilsLabelEXT      = (PFN_vkCmdEndDebugUtilsLabelEXT)      vkGetInstanceProcAddr( hInstance, "vkCmdEndDebugUtilsLabelEXT" );
        Ext.CmdInsertDebugUtilsLabelEXT   = (PFN_vkCmdInsertDebugUtilsLabelEXT)   vkGetInstanceProcAddr( hInstance, "vkCmdInsertDebugUtilsLabelEXT" );
        Ext.SetDebugUtilsObjectNameEXT    = (PFN_vkSetDebugUtilsObjectNameEXT)    vkGetInstanceProcAddr( hInstance, "vkSetDebugUtilsObjectNameEXT" );
        // clang-format on

        if ( Ext.CreateDebugUtilsMessengerEXT && Ext.DestroyDebugUtilsMessengerEXT && Ext.SubmitDebugUtilsMessageEXT &&
             Ext.CmdBeginDebugUtilsLabelEXT && Ext.CmdEndDebugUtilsLabelEXT && Ext.CmdInsertDebugUtilsLabelEXT &&
             Ext.SetDebugUtilsObjectNameEXT ) {

            if ( VK_SUCCESS != CheckedCall( Ext.CreateDebugUtilsMessengerEXT( hInstance, &debugUtilsMessengerCreateInfoEXT, NULL, &Ext.DebugUtilsMessengerEXT ) ) ) {
                return false;
            }
        }
    }

    uint32_t GPUCount = 0;
    if ( VK_SUCCESS != CheckedCall( vkEnumeratePhysicalDevices( hInstance, &GPUCount, NULL ) ) || !GPUCount ) {
        return false;
    }

    ppAdapters.resize( GPUCount );
    if ( VK_SUCCESS != CheckedCall( vkEnumeratePhysicalDevices( hInstance, &GPUCount, ppAdapters.data( ) ) ) ) {
        return false;
    }

    return true;
}

const char* ToString( VkDebugReportObjectTypeEXT eDebugReportObjectTypeEXT ) {
    switch ( eDebugReportObjectTypeEXT ) {
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

/* TODO: Complete error logging */
VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback( VkFlags                    msgFlags,
                                                    VkDebugReportObjectTypeEXT objType,
                                                    uint64_t                   srcObject,
                                                    size_t                     location,
                                                    int32_t                    msgCode,
                                                    const char*                pLayerPrefix,
                                                    const char*                pMsg,
                                                    void*                      pUserData ) {

    apemodevk::platform::LogLevel lvl = apemodevk::platform::LogLevel::Trace;
    if ( msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT ) {
        lvl = apemodevk::platform::LogLevel::Err;
    } else if ( msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT ) {
        lvl = apemodevk::platform::LogLevel::Warn;
    } else if ( msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT ) {
        lvl = apemodevk::platform::LogLevel::Info;
    } else if ( msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT ) {
        lvl = apemodevk::platform::LogLevel::Debug;
    }

    apemodevk::platform::DebugTrace( lvl, "[vk-drprt] [%s] (%s) %s", pLayerPrefix, ToString( objType ), pMsg );
    return false;
}

/* TODO: Complete error logging */
VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessengerCallback( VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                       VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                                       const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                       void*                                       pUserData ) {

    apemodevk::platform::LogLevel lvl = apemodevk::platform::LogLevel::Trace;
    if ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT ) {
        lvl = apemodevk::platform::LogLevel::Debug;
    } else if ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT ) {
        lvl = apemodevk::platform::LogLevel::Info;
    } else if ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ) {
        lvl = apemodevk::platform::LogLevel::Warn;
    } else if ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ) {
        lvl = apemodevk::platform::LogLevel::Err;
    }

    apemodevk::platform::DebugTrace( lvl, "[vk-dmsgr] [%s] %s", pCallbackData->pMessageIdName, pCallbackData->pMessage );
    return false;
}

static apemodevk::GraphicsManager * sGraphicsManagerInstance;

apemodevk::GraphicsManager* apemodevk::GetGraphicsManager( ) {
    return sGraphicsManagerInstance;
}

void apemodevk::DestroyGraphicsManager( ) {
    if ( sGraphicsManagerInstance ) {
        auto pAlloc  = sGraphicsManagerInstance->GetAllocator( );
        auto pLogger = sGraphicsManagerInstance->GetLogger( );

        sGraphicsManagerInstance->Destroy( );
        sGraphicsManagerInstance->~GraphicsManager( );

        pAlloc->Free( sGraphicsManagerInstance, __FILE__, __LINE__, __FUNCTION__ );
        pLogger->Log( apemodevk::platform::LogLevel::Err, "Destroyed GraphicsManager." );

        sGraphicsManagerInstance = nullptr;
    }
}

apemodevk::GraphicsManager* apemodevk::CreateGraphicsManager(
    uint32_t                                             eFlags,
    apemodevk::GraphicsManager::IAllocator*              pInAlloc,
    apemodevk::GraphicsManager::ILogger*                 pInLogger,
    apemodevk::GraphicsManager::IMemoryAllocationScope* pMemoryAllocationScope,
    const char*                                          pszAppName,
    const char*                                          pszEngineName,
    const char**                                         ppszLayers,
    size_t                                               layerCount,
    const char**                                         ppszExtensions,
    size_t                                               extensionCount ) {
    assert( sGraphicsManagerInstance == nullptr );

    void* pGraphicsManagerMemory = pInAlloc->Malloc( sizeof( GraphicsManager ), apemodevk::kAlignment, __FILE__, __LINE__, __FUNCTION__ );
    if ( !pGraphicsManagerMemory ) {
        pInLogger->Log( apemodevk::platform::LogLevel::Err, "Failed to allocate memory for GraphicsManager." );
        return nullptr;
    }

    sGraphicsManagerInstance = new ( pGraphicsManagerMemory ) GraphicsManager( );
    if ( !sGraphicsManagerInstance->Initialize( eFlags,
                                                pInAlloc,
                                                pInLogger,
                                                pMemoryAllocationScope,
                                                pszAppName,
                                                pszEngineName,
                                                ppszLayers,
                                                layerCount,
                                                ppszExtensions,
                                                extensionCount ) ) {
        sGraphicsManagerInstance->Destroy( );
        sGraphicsManagerInstance->~GraphicsManager( );
        sGraphicsManagerInstance = nullptr;

        pInAlloc->Free( pGraphicsManagerMemory, __FILE__, __LINE__, __FUNCTION__ );
        pInLogger->Log( apemodevk::platform::LogLevel::Err, "Failed to initialize GraphicsManager" );

        return nullptr;
    }

    pInLogger->Log( apemodevk::platform::LogLevel::Info, "Initialized GraphicsManager" );
    return GetGraphicsManager( );
}

const VkAllocationCallbacks* apemodevk::GetAllocationCallbacks( ) {
    return GetGraphicsManager( )->GetAllocationCallbacks( );
}

apemodevk::GraphicsManager::GraphicsManager( ) {
}

apemodevk::GraphicsManager::~GraphicsManager( ) {
}

apemodevk::GraphicsManager::IAllocator* apemodevk::GraphicsManager::GetAllocator( ) {
    return pAllocator;
}

apemodevk::GraphicsManager::ILogger* apemodevk::GraphicsManager::GetLogger( ) {
    return pLogger;
}

const apemodevk::GraphicsManager::IMemoryAllocationScope* apemodevk::GraphicsManager::GetMemoryAllocationScope( ) const {
    return pMemoryAllocationScope;
}

const VkAllocationCallbacks* apemodevk::GraphicsManager::GetAllocationCallbacks( ) const {
    return &AllocCallbacks;
}

void* apemodevk::GraphicsManager::AllocationCallbacks::AllocationFunction( void*                   pUserData,
                                                                           size_t                  size,
                                                                           size_t                  alignment,
                                                                           VkSystemAllocationScope allocationScope ) {
    return GetGraphicsManager( )->GetAllocator( )->Malloc( size, alignment, __FILE__, __LINE__, __FUNCTION__ );
}

void* apemodevk::GraphicsManager::AllocationCallbacks::ReallocationFunction( void*                   pUserData,
                                                                             void*                   pOriginal,
                                                                             size_t                  size,
                                                                             size_t                  alignment,
                                                                             VkSystemAllocationScope allocationScope ) {
    return GetGraphicsManager( )->GetAllocator( )->Realloc( pOriginal, size, alignment, __FILE__, __LINE__, __FUNCTION__ );
}

void apemodevk::GraphicsManager::AllocationCallbacks::FreeFunction( void* pUserData,
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

void apemodevk::GraphicsManager::AllocationCallbacks::InternalAllocationNotification( void*                    pUserData,
                                                                                      size_t                   size,
                                                                                      VkInternalAllocationType allocationType,
                                                                                      VkSystemAllocationScope  allocationScope ) {
    platform::DebugTrace( platform::LogLevel::Debug,
                          "[vk-intnt] allocated: %uz, type: %s, scope: %s",
                          size,
                          ToString( allocationType ),
                          ToString( allocationScope ) );
}

void apemodevk::GraphicsManager::AllocationCallbacks::InternalFreeNotification( void*                    pUserData,
                                                                                size_t                   size,
                                                                                VkInternalAllocationType allocationType,
                                                                                VkSystemAllocationScope  allocationScope ) {
    platform::DebugTrace( platform::LogLevel::Debug,
                          "[vk-intnt] freed: %uz, type: %s, scope: %s",
                          size,
                          ToString( allocationType ),
                          ToString( allocationScope ) );
}

void apemodevk::platform::Log( apemodevk::platform::LogLevel eLevel, char const* pszMsg ) {
    GetGraphicsManager( )->GetLogger( )->Log( eLevel, pszMsg );
}

void apemodevk::GetPrevMemoryAllocationScope( const char*&  pszSourceFile, unsigned int& sourceLine, const char*&  pszSourceFunc ) {
    return GetGraphicsManager( )->GetMemoryAllocationScope( )->GetPrevMemoryAllocationScope( pszSourceFile, sourceLine, pszSourceFunc );
}
void apemodevk::StartMemoryAllocationScope( const char* pszSourceFile, const unsigned int sourceLine, const char* pszSourceFunc ) {
    return GetGraphicsManager( )->GetMemoryAllocationScope( )->StartMemoryAllocationScope( pszSourceFile, sourceLine, pszSourceFunc );
}
void apemodevk::EndMemoryAllocationScope( const char* pszSourceFile, const unsigned int sourceLine, const char* pszSourceFunc ) {
    return GetGraphicsManager( )->GetMemoryAllocationScope( )->EndMemoryAllocationScope( pszSourceFile, sourceLine, pszSourceFunc );
}

void* operator new[]( std::size_t               size,
                      std::nothrow_t const&     eNothrowTag,
                      apemodevk::EAllocationTag eAllocTag,
                      const char*               pszSourceFile,
                      const unsigned int        sourceLine,
                      const char*               pszSourceFunc ) noexcept {
    return apemodevk::GetGraphicsManager( )->GetAllocator( )->Malloc( size, apemodevk::kAlignment, __FILE__, __LINE__, __FUNCTION__ );
}

void* operator new[]( std::size_t               size,
                      apemodevk::EAllocationTag eAllocTag,
                      const char*               pszSourceFile,
                      const unsigned int        sourceLine,
                      const char*               pszSourceFunc ) throw( ) {
    return apemodevk::GetGraphicsManager( )->GetAllocator( )->Malloc( size, apemodevk::kAlignment, __FILE__, __LINE__, __FUNCTION__ );
}

void operator delete[]( void*                     pMemory,
                        apemodevk::EAllocationTag eAllocTag,
                        const char*               pszSourceFile,
                        const unsigned int        sourceLine,
                        const char*               pszSourceFunc ) throw( ) {
    return apemodevk::GetGraphicsManager( )->GetAllocator( )->Free( pMemory, __FILE__, __LINE__, __FUNCTION__ );
}

void* operator new( std::size_t               size,
                    std::nothrow_t const&     eNothrowTag,
                    apemodevk::EAllocationTag eAllocTag,
                    const char*               pszSourceFile,
                    const unsigned int        sourceLine,
                    const char*               pszSourceFunc ) noexcept {
    return apemodevk::GetGraphicsManager( )->GetAllocator( )->Malloc( size, apemodevk::kAlignment, __FILE__, __LINE__, __FUNCTION__ );
}

void* operator new( std::size_t               size,
                    apemodevk::EAllocationTag eAllocTag,
                    const char*               pszSourceFile,
                    const unsigned int        sourceLine,
                    const char*               pszSourceFunc ) throw( ) {
    return apemodevk::GetGraphicsManager( )->GetAllocator( )->Malloc( size, apemodevk::kAlignment, __FILE__, __LINE__, __FUNCTION__ );
}

void operator delete( void*                     pMemory,
                      apemodevk::EAllocationTag eAllocTag,
                      const char*               pszSourceFile,
                      const unsigned int        sourceLine,
                      const char*               pszSourceFunc ) throw( ) {
    return apemodevk::GetGraphicsManager( )->GetAllocator( )->Free( pMemory, __FILE__, __LINE__, __FUNCTION__ );
}