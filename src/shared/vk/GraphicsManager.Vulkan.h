#pragma once

#include <NativeHandles.Vulkan.h>
#include <TInfoStruct.Vulkan.h>

namespace apemodevk {
    class GraphicsDevice;

    class GraphicsManager : public NoCopyAssignPolicy {
    public:
        friend class GraphicsDevice;

        enum InitFlags : uint32_t {
            kNormal           = 0,      // Normal initialization.
            kEnableValidation = 1 << 0, // Enable validation layers.
        };

        struct APIVersion : public apemodevk::ScalableAllocPolicy {
            uint32_t Major, Minor, Patch;
            APIVersion( );
        };

        struct ILogger {
            using LogLevel = platform::LogLevel;
            virtual void Log( LogLevel lvl, const char* pszMsg ) = 0;
        };

        struct AllocationCallbacks {
            static void* AllocationFunction( void*                   pUserData,
                                             size_t                  size,
                                             size_t                  alignment,
                                             VkSystemAllocationScope allocationScope );

            static void* ReallocationFunction( void*                   pUserData,
                                               void*                   pOriginal,
                                               size_t                  size,
                                               size_t                  alignment,
                                               VkSystemAllocationScope allocationScope );

            static void FreeFunction( void* pUserData,
                                      void* pMemory );

            static void InternalAllocationNotification( void*                    pUserData,
                                                        size_t                   size,
                                                        VkInternalAllocationType allocationType,
                                                        VkSystemAllocationScope  allocationScope );

            static void InternalFreeNotification( void*                    pUserData,
                                                  size_t                   size,
                                                  VkInternalAllocationType allocationType,
                                                  VkSystemAllocationScope  allocationScope );
        };

        /* Allocator interface, allows tracking. */
        struct IAllocator {
            /* malloc */
            virtual void* Malloc( size_t             size,
                                  size_t             alignment,
                                  const char*        sourceFile,
                                  const unsigned int sourceLine,
                                  const char*        sourceFunc ) = 0;

            /* realloc */
            virtual void* Realloc( void*              pOriginal,
                                   size_t             size,
                                   size_t             alignment,
                                   const char*        sourceFile,
                                   const unsigned int sourceLine,
                                   const char*        sourceFunc ) = 0;

            /* free */
            virtual void Free( void*              pMemory,
                               const char*        sourceFile,
                               const unsigned int sourceLine,
                               const char*        sourceFunc ) = 0;
        };

        /* Destruction order must be preserved */

        std::unique_ptr< IAllocator >     pAllocator;
        APIVersion                        Version;
        std::unique_ptr< ILogger >        pLogger;
        THandle< VkInstance >             hInstance;
        std::vector< VkPhysicalDevice >   ppAdapters;

        struct {
            PFN_vkGetDeviceProcAddr                       GetDeviceProcAddr                            = nullptr;
            PFN_vkGetPhysicalDeviceSurfaceSupportKHR      GetPhysicalDeviceSurfaceSupportKHR           = nullptr;
            PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR GetPhysicalDeviceSurfaceCapabilitiesKHR      = nullptr;
            PFN_vkGetPhysicalDeviceSurfaceFormatsKHR      GetPhysicalDeviceSurfaceFormatsKHR           = nullptr;
            PFN_vkGetPhysicalDeviceSurfacePresentModesKHR GetPhysicalDeviceSurfacePresentModesKHR      = nullptr;
            PFN_vkCreateDebugUtilsMessengerEXT            CreateDebugUtilsMessengerEXT                 = nullptr;
            PFN_vkDestroyDebugUtilsMessengerEXT           DestroyDebugUtilsMessengerEXT                = nullptr;
            PFN_vkSubmitDebugUtilsMessageEXT              SubmitDebugUtilsMessageEXT                   = nullptr;
            PFN_vkCmdBeginDebugUtilsLabelEXT              CmdBeginDebugUtilsLabelEXT                   = nullptr;
            PFN_vkCmdEndDebugUtilsLabelEXT                CmdEndDebugUtilsLabelEXT                     = nullptr;
            PFN_vkCmdInsertDebugUtilsLabelEXT             CmdInsertDebugUtilsLabelEXT                  = nullptr;
            PFN_vkSetDebugUtilsObjectNameEXT              SetDebugUtilsObjectNameEXT                   = nullptr;
            VkDebugUtilsMessengerEXT                      DebugUtilsMessengerEXT                       = nullptr;
            bool                                          bDebugReport                                 = false;
            bool                                          bDebugMessanger                              = false;
        } Ext;

        /* Initializes and returns GraphicsManager instance. */
        friend GraphicsManager* GetGraphicsManager( );

        bool Initialize( uint32_t                      flags,
                         std::unique_ptr< IAllocator > pAlloc,
                         std::unique_ptr< ILogger >    pLogger,
                         const char*                   pszAppName,
                         const char*                   pszEngineName,
                         const char**                  ppszLayers,
                         size_t                        layerCount,
                         const char**                  ppszExtensions,
                         size_t                        extensionCount );
        void Destroy( );

        IAllocator* GetAllocator( );
        ILogger*    GetLogger( );

        const VkAllocationCallbacks* GetAllocationCallbacks( ) const;

    private:
        GraphicsManager( );
        ~GraphicsManager( );
    };

    /* Returns GraphicsManager instance */
    GraphicsManager* GetGraphicsManager( );

    /* Returns allocation callbacks of the GraphicsManager.
     * @see GraphicsManager::GetAllocationCallbacks() */
    const VkAllocationCallbacks* GetAllocationCallbacks( );
}
