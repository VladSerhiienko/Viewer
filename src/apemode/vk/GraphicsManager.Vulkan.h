#pragma once

#include <apemode/vk/NativeHandles.Vulkan.h>
#include <apemode/vk/TInfoStruct.Vulkan.h>

namespace apemodevk {
    class GraphicsDevice;

    class APEMODEVK_API GraphicsManager : public NoCopyAssignPolicy {
    public:
        friend class GraphicsDevice;

        enum InitFlags : uint32_t {
            kNormal           = 0,      // Normal initialization.
            kEnableValidation = 1 << 0, // Enable validation layers.
        };

        struct APEMODEVK_API APIVersion : public apemodevk::ScalableAllocPolicy {
            uint32_t Major, Minor, Patch;
            APIVersion( );
        };

        struct APEMODEVK_API ILogger {
            using LogLevel = platform::LogLevel;
            virtual void Log( LogLevel lvl, const char* pszMsg ) = 0;
        };

        struct APEMODEVK_API AllocationCallbacks {
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
        struct APEMODEVK_API IAllocator {
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

            /* Allocator interface, allows tracking. */

            virtual void GetPrevMemoryAllocationScope( const char*&  pszPrevSourceFile,
                                                       unsigned int& prevSourceLine,
                                                       const char*&  pszPrevSourceFunc ) const  = 0;
            virtual void StartMemoryAllocationScope( const char*        pszSourceFile,
                                                     const unsigned int sourceLine,
                                                     const char*        pszSourceFunc ) const   = 0;
            virtual void EndMemoryAllocationScope( const char*        pszPrevSourceFile,
                                                   const unsigned int prevSourceLine,
                                                   const char*        pszPrevSourceFunc ) const = 0;
        };

        /* Destruction order must be preserved */

        IAllocator*                pAllocator;
        ILogger*                   pLogger;
        APIVersion                 Version;
        THandle< VkInstance >      hInstance;
        vector< VkPhysicalDevice > ppAdapters;
        VkAllocationCallbacks      AllocCallbacks;

        struct {
            bool bDebugReport = false;
            bool bDebugMessenger = false;
            VkDebugUtilsMessengerEXT DebugUtilsMessengerEXT = nullptr;
        } Ext;

        /* Initializes and returns GraphicsManager instance. */
        friend GraphicsManager* CreateGraphicsManager( uint32_t                eFlags,
                                                       IAllocator*             pInAlloc,
                                                       ILogger*                pInLogger,
                                                       const char*             pszAppName,
                                                       const char*             pszEngineName,
                                                       const char**            ppszLayers,
                                                       size_t                  layerCount,
                                                       const char**            ppszExtensions,
                                                       size_t                  extensionCount );
        friend GraphicsManager* GetGraphicsManager( );
        friend void             DestroyGraphicsManager( );

        bool Initialize( uint32_t                flags,
                         IAllocator*             pAlloc,
                         ILogger*                pLogger,
                         const char*             pszAppName,
                         const char*             pszEngineName,
                         const char**            ppszLayers,
                         size_t                  layerCount,
                         const char**            ppszExtensions,
                         size_t                  extensionCount );
        void Destroy( );

        IAllocator*                  GetAllocator( );
        ILogger*                     GetLogger( );
        const VkAllocationCallbacks* GetAllocationCallbacks( ) const;

    private:
        GraphicsManager( );
        ~GraphicsManager( );
    };

    /* Returns GraphicsManager instance */
    GraphicsManager* CreateGraphicsManager( uint32_t                                 eFlags,
                                            GraphicsManager::IAllocator*             pInAlloc,
                                            GraphicsManager::ILogger*                pInLogger,
                                            const char*                              pszAppName,
                                            const char*                              pszEngineName,
                                            const char**                             ppszLayers,
                                            size_t                                   layerCount,
                                            const char**                             ppszExtensions,
                                            size_t                                   extensionCount );
    GraphicsManager* GetGraphicsManager( );
    void             DestroyGraphicsManager( );

    /* Returns allocation callbacks of the GraphicsManager.
     * @see GraphicsManager::GetAllocationCallbacks() */
    const VkAllocationCallbacks* GetAllocationCallbacks( );

    uint64_t GetPerformanceCounter( );
    uint64_t GetNanoseconds( uint64_t counter );

    namespace platform {
        template <>
        struct TDelete< GraphicsManager > {
            void operator( )( GraphicsManager* pObj ) {
                if ( pObj )
                    DestroyGraphicsManager( );
            }
        };
    } // namespace platform

} // namespace apemodevk
