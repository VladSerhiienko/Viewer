#pragma once

#include <NativeHandles.Vulkan.h>
#include <TInfoStruct.Vulkan.h>

namespace apemodevk {
    class GraphicsDevice;

    class GraphicsManager : public NoCopyAssignPolicy {
    public:
        friend class GraphicsDevice;

        enum InitFlags : uint32_t {
            kNormal                   = 0, // Normal initialization.
            kEnable_RENDERDOC_Capture = 1, // Must be enabled for RenderDoc tool.
            kEnable_LUNARG_vktrace    = 2, // Must be enable for vktrace tool.
            kEnable_LUNARG_api_dump   = 4, // Traces every call to stdout.
        };

        struct APIVersion : public apemodevk::ScalableAllocPolicy {
            uint32_t Major, Minor, Patch;
            APIVersion( );
        };

        struct NativeLayerWrapper : public apemodevk::ScalableAllocPolicy {
            bool                                 bIsUnnamed;
            VkLayerProperties                    Layer;
            std::vector< VkExtensionProperties > Extensions;

            bool IsUnnamedLayer( ) const;
            bool IsValidInstanceLayer( ) const;
            bool IsValidDeviceLayer( ) const;
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

        static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback( VkFlags                    msgFlags,
                                                             VkDebugReportObjectTypeEXT objType,
                                                             uint64_t                   srcObject,
                                                             size_t                     location,
                                                             int32_t                    msgCode,
                                                             const char *               pLayerPrefix,
                                                             const char *               pMsg,
                                                             void *                     pUserData );

        /* Destruction order must be preserved */

        std::unique_ptr< IAllocator >        pAllocator;
        APIVersion                           Version;
        std::vector< const char * >          InstanceLayers;
        std::vector< const char * >          InstanceExtensions;
        std::vector< VkLayerProperties >     InstanceLayerProps;
        std::vector< VkExtensionProperties > InstanceExtensionProps;
        std::vector< NativeLayerWrapper >    LayerWrappers;
        std::unique_ptr< ILogger >           pLogger;
        std::unique_ptr< GraphicsDevice >    PrimaryNode;
        std::unique_ptr< GraphicsDevice >    SecondaryNode;
        THandle< VkInstance >                hInstance;

        /* Initializes and returns GraphicsManager instance. */
        friend GraphicsManager* GetGraphicsManager( );

        bool RecreateGraphicsNodes( uint32_t                      flags,
                                    std::unique_ptr< IAllocator > pAlloc,
                                    std::unique_ptr< ILogger >    pLogger,
                                    const char*                   pszAppName,
                                    const char*                   pszEngineName );
        void Destroy( );

        GraphicsDevice*              GetPrimaryGraphicsNode( );
        GraphicsDevice*              GetSecondaryGraphicsNode( );
        IAllocator*                  GetAllocator( );
        ILogger*                     GetLogger( );
        const VkAllocationCallbacks* GetAllocationCallbacks( ) const;

    private:
        GraphicsManager( );
        ~GraphicsManager( );

        bool                ScanInstanceLayerProperties( uint32_t flags );
        bool                ScanAdapters( uint32_t flags );
        NativeLayerWrapper &GetUnnamedLayer( );
    };

    /* Returns GraphicsManager instance */
    GraphicsManager* GetGraphicsManager( );

    /* Returns allocation callbacks of the GraphicsManager.
     * @see GraphicsManager::GetAllocationCallbacks() */
    const VkAllocationCallbacks* GetAllocationCallbacks( );
}
