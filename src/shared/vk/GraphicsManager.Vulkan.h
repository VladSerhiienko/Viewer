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
            APIVersion( bool bDump = true );
        };

        struct NativeLayerWrapper : public apemodevk::ScalableAllocPolicy {
            typedef TInfoStruct< VkExtensionProperties >::Vector VkExtensionPropertiesVector;

            bool                             bIsUnnamed;
            TInfoStruct< VkLayerProperties > Layer;
            VkExtensionPropertiesVector      Extensions;

            bool IsUnnamedLayer( ) const;
            bool IsValidInstanceLayer( ) const;
            bool IsValidDeviceLayer( ) const;
        };

        struct AllocationCallbacks {
            static void* AllocationFunction( void*                   pUserData,
                                             size_t                  size,
                                             size_t                  alignment,
                                             VkSystemAllocationScope allocationScope );

            static void* ReallocationFunction( void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope );

            static void FreeFunction( void* pUserData, void* pMemory );
        };

        /* Allocator interface, allows tracking. */
        struct IAllocator {
            /* malloc */
            virtual void* Allocation( size_t             size,
                                      size_t             alignment,
                                      const char*        sourceFile,
                                      const unsigned int sourceLine,
                                      const char*        sourceFunc ) = 0;

            /* realloc */
            virtual void* Reallocation( void*              pOriginal,
                                        size_t             size,
                                        size_t             alignment,
                                        const char*        sourceFile,
                                        const unsigned int sourceLine,
                                        const char*        sourceFunc ) = 0;

            /* free */
            virtual void  Free( void*              pUserData,
                                void*              pMemory,
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

        static VKAPI_ATTR VkBool32 VKAPI_CALL BreakCallback( VkFlags                    msgFlags,
                                                             VkDebugReportObjectTypeEXT objType,
                                                             uint64_t                   srcObject,
                                                             size_t                     location,
                                                             int32_t                    msgCode,
                                                             const char *               pLayerPrefix,
                                                             const char *               pMsg,
                                                             void *                     pUserData );

        std::unique_ptr< GraphicsDevice >    PrimaryNode;
        std::unique_ptr< GraphicsDevice >    SecondaryNode;
        APIVersion                           Version;
        std::string                          AppName;
        std::string                          EngineName;
        std::vector< const char * >          InstanceLayers;
        std::vector< const char * >          InstanceExtensions;
        std::vector< VkLayerProperties >     InstanceLayerProps;
        std::vector< VkExtensionProperties > InstanceExtensionProps;
        std::vector< NativeLayerWrapper >    LayerWrappers;
        THandle< VkInstance >                hInstance;
        std::unique_ptr< IAllocator >        pAllocator;

        /* Initializes the instance if needed, and returns it. */
        static GraphicsManager *Get( );

        bool            RecreateGraphicsNodes( uint32_t flags, std::unique_ptr< IAllocator > pAlloc );
        GraphicsDevice* GetPrimaryGraphicsNode( );
        GraphicsDevice* GetSecondaryGraphicsNode( );
        IAllocator*     GetAllocator( );

    private:
        GraphicsManager( );
        ~GraphicsManager( );

        bool                ScanInstanceLayerProperties( uint32_t flags );
        bool                ScanAdapters( uint32_t flags );
        bool                InitializeInstance( uint32_t flags );
        NativeLayerWrapper &GetUnnamedLayer( );
    };
}
