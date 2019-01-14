#pragma once

#include <apemode/vk/THandle.Vulkan.h>

namespace apemodevk
{
    template <>
    struct THandleDeleter< VkInstance > {
        void operator( )( VkInstance &Handle ) {
            if ( Handle == nullptr )
                return;

            vkDestroyInstance( Handle, GetAllocationCallbacks( ) );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct THandleDeleter< VkDevice >  {
        void operator( )( VkDevice &Handle ) {
            if ( Handle == VK_NULL_HANDLE )
                return;

            vkDestroyDevice( Handle, GetAllocationCallbacks( ) );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct THandleDeleter< VkQueue > {
        void operator( )( VkQueue &Handle ) {
            // Queues are created along with a logical device creation during vkCreateDevice. Because of this, no
            // explicit deletion of queues is required. All queues associated with a logical device are destroyed when
            // vkDestroyDevice is called on that device.
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct THandleDeleter< VkSurfaceKHR > {
        VkInstance hInstance;

        THandleDeleter( ) : hInstance( VK_NULL_HANDLE ) {
        }

        void operator( )( VkSurfaceKHR &Handle ) {
            if ( Handle == nullptr )
                return;

            apemode_assert( hInstance != nullptr, "Instance is required." );
            vkDestroySurfaceKHR( hInstance, Handle, GetAllocationCallbacks( ) );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct THandleDeleter< VkSwapchainKHR > {
        VkDevice hLogicalDevice;

        THandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ) {
        }

        void operator( )( VkSwapchainKHR &Handle ) {
            if ( Handle == nullptr )
                return;

            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            vkDestroySwapchainKHR( hLogicalDevice, Handle, GetAllocationCallbacks( ) );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct THandleDeleter< VkCommandPool > {
        VkDevice hLogicalDevice;

        THandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ) {
        }

        void operator( )( VkCommandPool &Handle ) {
            if ( Handle == nullptr )
                return;

            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            vkDestroyCommandPool( hLogicalDevice, Handle, GetAllocationCallbacks( ) );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct THandleDeleter< VkCommandBuffer > {
        VkDevice      hLogicalDevice;
        VkCommandPool CmdPool;

        THandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ), CmdPool( VK_NULL_HANDLE ) {
        }

        void operator( )( VkCommandBuffer &Handle ) {
            if ( Handle == nullptr )
                return;

            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            vkFreeCommandBuffers( hLogicalDevice, CmdPool, 1, &Handle );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct THandleDeleter< VkFence > {
        VkDevice hLogicalDevice;

        THandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ) {
        }

        void operator( )( VkFence &Handle ) {
            if ( Handle == nullptr )
                return;

            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            vkDestroyFence( hLogicalDevice, Handle, GetAllocationCallbacks( ) );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct THandleDeleter< VkEvent > {
        VkDevice hLogicalDevice;

        THandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ) {
        }

        void operator( )( VkEvent &Handle ) {
            if ( Handle == nullptr )
                return;

            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            vkDestroyEvent( hLogicalDevice, Handle, GetAllocationCallbacks( ) );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct THandleDeleter< VkImage > {
        VkDevice         hLogicalDevice;
        VkPhysicalDevice PhysicalDeviceHandle;

        THandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ), PhysicalDeviceHandle( VK_NULL_HANDLE ) {
        }

        void operator( )( VkImage &Handle ) {
            if ( Handle == nullptr || hLogicalDevice == nullptr )
                return;

            // Image can be assigned without ownership (by swapchain, for example).
            // apemode_assert(hLogicalDevice != nullptr, "Device is required.");
            vkDestroyImage( hLogicalDevice, Handle, GetAllocationCallbacks( ) );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct THandleDeleter< VkSampler > {
        VkDevice         hLogicalDevice;

        THandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ) {
        }

        void operator( )( VkSampler &Handle ) {
            if ( Handle == nullptr )
                return;

            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            vkDestroySampler( hLogicalDevice, Handle, GetAllocationCallbacks( ) );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct THandleDeleter< VkImageView > {
        VkDevice hLogicalDevice;

        THandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ) {
        }

        void operator( )( VkImageView &Handle ) {
            if ( Handle == nullptr )
                return;

            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            vkDestroyImageView( hLogicalDevice, Handle, GetAllocationCallbacks( ) );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct THandleDeleter< VkDeviceMemory > {
        VkDevice hLogicalDevice;

        THandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ) {
        }

        void operator( )( VkDeviceMemory &Handle ) {
            if ( Handle == nullptr )
                return;

            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            vkFreeMemory( hLogicalDevice, Handle, GetAllocationCallbacks( ) );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct THandleDeleter< VkBuffer > {
        VkDevice         hLogicalDevice;
        VkPhysicalDevice PhysicalDeviceHandle;

        THandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ), PhysicalDeviceHandle( VK_NULL_HANDLE ) {
        }

        void operator( )( VkBuffer &Handle ) {
            if ( Handle == nullptr )
                return;

            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            vkDestroyBuffer( hLogicalDevice, Handle, GetAllocationCallbacks( ) );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct THandleDeleter< VkBufferView > {
        VkDevice hLogicalDevice;

        THandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ) {
        }

        void operator( )( VkBufferView &Handle ) {
            if ( Handle == nullptr )
                return;

            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            vkDestroyBufferView( hLogicalDevice, Handle, GetAllocationCallbacks( ) );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct THandleDeleter< VkFramebuffer > {
        VkDevice hLogicalDevice;

        THandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ) {
        }

        void operator( )( VkFramebuffer &Handle ) {
            if ( Handle == nullptr )
                return;

            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            vkDestroyFramebuffer( hLogicalDevice, Handle, GetAllocationCallbacks( ) );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct THandleDeleter< VkRenderPass > {
        VkDevice hLogicalDevice;

        THandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ) {
        }

        void operator( )( VkRenderPass &Handle ) {
            if ( Handle == nullptr )
                return;

            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            vkDestroyRenderPass( hLogicalDevice, Handle, GetAllocationCallbacks( ) );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct THandleDeleter< VkPipelineCache > {
        VkDevice hLogicalDevice;

        THandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ) {
        }

        void operator( )( VkPipelineCache &Handle ) {
            if ( Handle == nullptr )
                return;

            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            vkDestroyPipelineCache( hLogicalDevice, Handle, GetAllocationCallbacks( ) );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct THandleDeleter< VkPipeline > {
        VkDevice hLogicalDevice;

        THandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ) {
        }

        void operator( )( VkPipeline &Handle ) {
            if ( Handle == nullptr )
                return;

            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            vkDestroyPipeline( hLogicalDevice, Handle, GetAllocationCallbacks( ) );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct THandleDeleter< VkDescriptorSetLayout > {
        VkDevice hLogicalDevice;

        THandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ) {
        }

        void operator( )( VkDescriptorSetLayout &Handle ) {
            if ( Handle == nullptr )
                return;

            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            vkDestroyDescriptorSetLayout( hLogicalDevice, Handle, GetAllocationCallbacks( ) );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct THandleDeleter< VkDescriptorSet > {
        VkDevice         hLogicalDevice;
        VkDescriptorPool hDescPool;

        THandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ), hDescPool( VK_NULL_HANDLE ) {
        }

        void operator( )( VkDescriptorSet &Handle ) {
            if ( Handle == nullptr )
                return;

            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            const VkResult eIsFreed = vkFreeDescriptorSets( hLogicalDevice, hDescPool, 1, &Handle );
            apemode_assert( VK_SUCCESS == eIsFreed, "Failed to free descriptor set (vkFreeDescriptorSets)." );
            (void)eIsFreed;

            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct THandleDeleter< VkDescriptorPool > {
        VkDevice hLogicalDevice;

        THandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ) {
        }

        void operator( )( VkDescriptorPool &Handle ) {
            if ( Handle == nullptr )
                return;

            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            vkDestroyDescriptorPool( hLogicalDevice, Handle, GetAllocationCallbacks( ) );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct THandleDeleter< VkPipelineLayout > {
        VkDevice hLogicalDevice;

        THandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ) {
        }

        void operator( )( VkPipelineLayout &Handle ) {
            if ( Handle == nullptr )
                return;

            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            vkDestroyPipelineLayout( hLogicalDevice, Handle, GetAllocationCallbacks( ) );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct THandleDeleter< VkShaderModule > {
        VkDevice hLogicalDevice;

        THandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ) {
        }

        void operator( )( VkShaderModule &Handle ) {
            if ( Handle == nullptr )
                return;

            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            vkDestroyShaderModule( hLogicalDevice, Handle, GetAllocationCallbacks( ) );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct THandleDeleter< VkSemaphore > {
        VkDevice hLogicalDevice;

        THandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ) {
        }

        void operator( )( VkSemaphore &Handle ) {
            if ( Handle == nullptr )
                return;

            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            vkDestroySemaphore( hLogicalDevice, Handle, GetAllocationCallbacks( ) );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct THandleDeleter< VmaAllocator > {
        void operator( )( VmaAllocator &Handle ) {
            if ( Handle == VK_NULL_HANDLE )
                return;

            vmaDestroyAllocator( Handle );
            Handle = VK_NULL_HANDLE;
        }
    };

    template < typename TNativeHandle >
    struct THandle : public THandleBase< TNativeHandle > {};

    template <>
    struct THandle< VkInstance > : public THandleBase< VkInstance > {
        bool Recreate( VkInstanceCreateInfo const &CreateInfo ) {
            Deleter( Handle );
            return VK_SUCCESS == CheckedResult( vkCreateInstance( &CreateInfo, GetAllocationCallbacks( ), &Handle ) );
        }
    };

    template <>
    struct THandle< VkDevice > : public THandleBase< VkDevice > {
        bool Recreate( VkPhysicalDevice pPhysicalDevice, VkDeviceCreateInfo const &CreateInfo ) {
            apemode_assert( pPhysicalDevice != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            return VK_SUCCESS == CheckedResult( vkCreateDevice( pPhysicalDevice, &CreateInfo, GetAllocationCallbacks( ), &Handle ) );
        }
    };

    template <>
    struct THandle< VkQueue > : public THandleBase< VkQueue > {
        void Recreate( VkDevice InLogicalDeviceHandle, uint32_t queueFamilyId, uint32_t queueId ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            if ( InLogicalDeviceHandle != nullptr ) {
                vkGetDeviceQueue( InLogicalDeviceHandle, queueFamilyId, queueId, &Handle );
            }
        }

        void WaitIdle( ) {
            vkQueueWaitIdle( Handle );
        }
    };

#ifdef VK_USE_PLATFORM_WIN32_KHR
    template <>
    struct THandle< VkSurfaceKHR > : public THandleBase< VkSurfaceKHR > {
        bool Recreate( VkInstance InInstanceHandle, VkWin32SurfaceCreateInfoKHR const &CreateInfo ) {
            apemode_assert( InInstanceHandle != VK_NULL_HANDLE, "Instance is required." );

            Deleter( Handle );
            Deleter.hInstance = InInstanceHandle;
            return VK_SUCCESS == CheckedResult( vkCreateWin32SurfaceKHR( InInstanceHandle, &CreateInfo, GetAllocationCallbacks( ), &Handle ) );
        }
    };
#elif VK_USE_PLATFORM_XLIB_KHR
    template <>
    struct THandle< VkSurfaceKHR > : public THandleBase< VkSurfaceKHR > {
        bool Recreate( VkInstance InInstanceHandle, VkXlibSurfaceCreateInfoKHR const &CreateInfo ) {
            apemode_assert( InInstanceHandle != VK_NULL_HANDLE, "Instance is required." );

            Deleter( Handle );
            Deleter.hInstance = InInstanceHandle;
            return VK_SUCCESS == CheckedResult( vkCreateXlibSurfaceKHR( InInstanceHandle, &CreateInfo, GetAllocationCallbacks( ), &Handle ) );
        }
    };
#elif VK_USE_PLATFORM_MACOS_MVK
    template <>
    struct THandle< VkSurfaceKHR > : public THandleBase< VkSurfaceKHR > {
        bool Recreate( VkInstance InInstanceHandle, VkMacOSSurfaceCreateInfoMVK const &CreateInfo ) {
            apemode_assert( InInstanceHandle != VK_NULL_HANDLE, "Instance is required." );

            Deleter( Handle );
            Deleter.hInstance = InInstanceHandle;
            return VK_SUCCESS == CheckedResult( vkCreateMacOSSurfaceMVK( InInstanceHandle, &CreateInfo, GetAllocationCallbacks( ), &Handle ) );
        }
    };
#elif VK_USE_PLATFORM_IOS_MVK
    template <>
    struct THandle< VkSurfaceKHR > : public THandleBase< VkSurfaceKHR > {
        bool Recreate( VkInstance InInstanceHandle, VkIOSSurfaceCreateInfoMVK const &CreateInfo ) {
            apemode_assert( InInstanceHandle != VK_NULL_HANDLE, "Instance is required." );

            Deleter( Handle );
            Deleter.hInstance = InInstanceHandle;
            return VK_SUCCESS == CheckedResult( vkCreateIOSSurfaceMVK( InInstanceHandle, &CreateInfo, GetAllocationCallbacks( ), &Handle ) );
        }
    };
#endif

    template <>
    struct THandle< VkSwapchainKHR > : public THandleBase< VkSwapchainKHR > {
        /**
         * Creates swapchain.
         * Handles deleting of the old swapchain.
         **/
        bool Recreate( VkDevice InLogicalDeviceHandle, VkSwapchainCreateInfoKHR const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            auto     PrevHandle  = Handle;
            TDeleter PrevDeleter = Deleter;

            if ( InLogicalDeviceHandle != nullptr ) {
                Deleter.hLogicalDevice = InLogicalDeviceHandle;
                if ( VK_SUCCESS == CheckedResult( vkCreateSwapchainKHR( InLogicalDeviceHandle, &CreateInfo, GetAllocationCallbacks( ), &Handle ) ) ) {

                    /* If we just re-created an existing swapchain, we should destroy the old swapchain at this point.
                     * @note Destroying the swapchain also cleans up all its associated presentable images
                     * once the platform is done with them.
                     **/

                    PrevDeleter( PrevHandle );
                    return true;
                }
            }

            return false;
        }
    };

    template <>
    struct THandle< VkCommandPool > : public THandleBase< VkCommandPool > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkCommandPoolCreateInfo const &CreateInfo ) {
            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedResult( vkCreateCommandPool( InLogicalDeviceHandle, &CreateInfo, GetAllocationCallbacks( ), &Handle ) );
        }

        bool Reset( bool bRecycleResources ) {
            assert( IsNotNull( ) );
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );
            const auto ResetFlags = bRecycleResources ? VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT : 0u;
            return VK_SUCCESS == CheckedResult( vkResetCommandPool( Deleter.hLogicalDevice, Handle, ResetFlags ) );
        }
    };

    template <>
    struct THandle< VkCommandBuffer > : public THandleBase< VkCommandBuffer > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkCommandBufferAllocateInfo const &AllocInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );
            apemode_assert( AllocInfo.commandPool != VK_NULL_HANDLE, "No default pools available." );
            apemode_assert( AllocInfo.commandBufferCount == 1, "This method handles single command list." );

            Deleter( Handle );
            Deleter.CmdPool        = AllocInfo.commandPool;
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedResult( vkAllocateCommandBuffers( InLogicalDeviceHandle, &AllocInfo, &Handle ) );
        }
    };

    template <>
    struct THandle< VkFence > : public THandleBase< VkFence > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkFenceCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedResult( vkCreateFence( InLogicalDeviceHandle, &CreateInfo, GetAllocationCallbacks( ), &Handle ) );
        }

        bool Wait( uint64_t Timeout = UINT64_MAX ) const {
            return VK_SUCCESS == CheckedResult( vkWaitForFences( Deleter.hLogicalDevice, 1, &Handle, true, Timeout ) );
        }

        inline VkResult GetStatus() const       { return vkGetFenceStatus( Deleter.hLogicalDevice, Handle ); }
        inline bool     IsSignalled() const  { return vkGetFenceStatus( Deleter.hLogicalDevice, Handle ) == VK_SUCCESS; }
        inline bool     IsInProgress() const { return vkGetFenceStatus( Deleter.hLogicalDevice, Handle ) == VK_NOT_READY; }
        inline bool     Failed() const       { return vkGetFenceStatus( Deleter.hLogicalDevice, Handle ) < VK_SUCCESS; }
    };

    template <>
    struct THandle< VkEvent > : public THandleBase< VkEvent > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkEventCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedResult( vkCreateEvent( InLogicalDeviceHandle, &CreateInfo, GetAllocationCallbacks( ), &Handle ) );
        }

        bool Set( ) const {
            return VK_SUCCESS == CheckedResult( vkSetEvent( Deleter.hLogicalDevice, Handle ) );
        }

        bool Reset( ) const {
            return VK_SUCCESS == CheckedResult( vkResetEvent( Deleter.hLogicalDevice, Handle ) );
        }

        inline VkResult GetStatus( ) const  { return vkGetEventStatus( Deleter.hLogicalDevice, Handle ); }
        inline bool     IsSet( ) const   { return vkGetEventStatus( Deleter.hLogicalDevice, Handle ) == VK_EVENT_SET; }
        inline bool     IsReset( ) const { return vkGetEventStatus( Deleter.hLogicalDevice, Handle ) == VK_EVENT_RESET; }
        inline bool     Failed( ) const  { return vkGetEventStatus( Deleter.hLogicalDevice, Handle ) < VK_SUCCESS; }
    };

    inline uint32_t ResolveMemoryType( const VkPhysicalDeviceMemoryProperties & physicalDeviceMemoryProperties, VkMemoryPropertyFlags properties, uint32_t type_bits ) {
        for ( uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++ )
            if ( ( physicalDeviceMemoryProperties.memoryTypes[ i ].propertyFlags & properties ) == properties && type_bits & ( 1 << i ) )
                return i;

        // Unable to find memoryType
        // DebugBreak( );
        return uint32_t( -1 );
    }

    inline uint32_t ResolveMemoryType( VkPhysicalDevice gpu, VkMemoryPropertyFlags properties, uint32_t type_bits ) {
        VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
        vkGetPhysicalDeviceMemoryProperties( gpu, &physicalDeviceMemoryProperties );
        return ResolveMemoryType( physicalDeviceMemoryProperties, properties, type_bits );
    }

    template <>
    struct THandle<VkImage> : public THandleBase<VkImage>
    {
        bool Assign( VkDevice         InLogicalDeviceHandle,
                     VkPhysicalDevice InPhysicalDeviceHandle,
                     VkImage          Img,
                     bool             bTakeOwnership ) {
            Deleter( Handle );

            const bool bCaseOk = InLogicalDeviceHandle != VK_NULL_HANDLE && Img != VK_NULL_HANDLE;
            const bool bCaseNull = InLogicalDeviceHandle == VK_NULL_HANDLE && Img == VK_NULL_HANDLE;
            apemode_assert( bCaseOk || bCaseNull, "Both handles should be null or valid." );

            if ( bCaseOk || bCaseNull ) {
                Handle = Img;
                Deleter.PhysicalDeviceHandle = InPhysicalDeviceHandle;
                Deleter.hLogicalDevice = bTakeOwnership ? InLogicalDeviceHandle : nullptr;
                return true;
            }

            return false;
        }

        bool Recreate( VkDevice                 InLogicalDeviceHandle,
                       VkPhysicalDevice         InPhysicalDeviceHandle,
                       VkImageCreateInfo const &CreateInfo ) {
            apemode_assert(InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required.");

            Deleter( Handle );
            Deleter.hLogicalDevice       = InLogicalDeviceHandle;
            Deleter.PhysicalDeviceHandle = InPhysicalDeviceHandle;
            return VK_SUCCESS == CheckedResult( vkCreateImage( InLogicalDeviceHandle, &CreateInfo, GetAllocationCallbacks( ), &Handle ) );
        }

        VkMemoryRequirements GetMemoryRequirements( ) {
            apemode_assert( IsNotNull( ), "Null." );

            VkMemoryRequirements memoryRequirements;
            vkGetImageMemoryRequirements( Deleter.hLogicalDevice, Handle, &memoryRequirements );
            return memoryRequirements;
        }

        VkMemoryAllocateInfo GetMemoryAllocateInfo(VkMemoryPropertyFlags memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {

            VkMemoryRequirements memoryRequirements = GetMemoryRequirements();

            VkMemoryAllocateInfo memoryAllocInfo;
            apemodevk::utils::ZeroMemory(memoryAllocInfo);
            memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memoryAllocInfo.allocationSize = memoryRequirements.size;
            memoryAllocInfo.memoryTypeIndex = ResolveMemoryType(Deleter.PhysicalDeviceHandle, memoryPropertyFlags, memoryRequirements.memoryTypeBits);

            return memoryAllocInfo;
        }

        bool BindMemory( VkDeviceMemory hMemory, uint32_t Offset = 0 ) {
            return VK_SUCCESS == CheckedResult( vkBindImageMemory( Deleter.hLogicalDevice, Handle, hMemory, Offset ) );
        }
    };

    template <>
    struct THandle< VkImageView > : public THandleBase< VkImageView > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkImageViewCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedResult( vkCreateImageView( InLogicalDeviceHandle, &CreateInfo, GetAllocationCallbacks( ), &Handle ) );
        }
    };

    template <>
    struct THandle<VkDeviceMemory> : public THandleBase<VkDeviceMemory>
    {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkMemoryAllocateInfo const &AllocInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedResult( vkAllocateMemory( InLogicalDeviceHandle, &AllocInfo, GetAllocationCallbacks( ), &Handle ) );
        }

        uint8_t *Map( uint32_t mappedMemoryOffset, uint32_t mappedMemorySize, VkMemoryHeapFlags mapFlags ) {
            apemode_assert( Deleter.hLogicalDevice != VK_NULL_HANDLE, "Device is required." );

            uint8_t *mappedData = nullptr;
            if ( VK_SUCCESS == CheckedResult( vkMapMemory( Deleter.hLogicalDevice,
                                                         Handle,
                                                         mappedMemoryOffset,
                                                         mappedMemorySize,
                                                         mapFlags,
                                                         reinterpret_cast< void ** >( &mappedData ) ) ) )
                return mappedData;
            return nullptr;
        }

        void Unmap( ) {
            apemode_assert( Deleter.hLogicalDevice != VK_NULL_HANDLE, "Device is required." );
            vkUnmapMemory( Deleter.hLogicalDevice, Handle );
        }
    };

    template <>
    struct THandle< VkBuffer > : public THandleBase< VkBuffer > {
        bool Recreate( VkDevice                  InLogicalDeviceHandle,
                       VkPhysicalDevice          InPhysicalDeviceHandle,
                       VkBufferCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice       = InLogicalDeviceHandle;
            Deleter.PhysicalDeviceHandle = InPhysicalDeviceHandle;
            return VK_SUCCESS == CheckedResult( vkCreateBuffer( InLogicalDeviceHandle, &CreateInfo, GetAllocationCallbacks( ), &Handle ) );
        }

        VkMemoryRequirements GetMemoryRequirements( ) {
            apemode_assert( IsNotNull( ), "Null." );

            VkMemoryRequirements memoryRequirements;
            vkGetBufferMemoryRequirements( Deleter.hLogicalDevice, Handle, &memoryRequirements );
            return memoryRequirements;
        }

        VkMemoryAllocateInfo GetMemoryAllocateInfo( VkMemoryPropertyFlags memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) {
            VkMemoryRequirements memoryRequirements = GetMemoryRequirements( );

            VkMemoryAllocateInfo memoryAllocInfo;
            apemodevk::utils::ZeroMemory( memoryAllocInfo );
            memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memoryAllocInfo.allocationSize = memoryRequirements.size;
            memoryAllocInfo.memoryTypeIndex = ResolveMemoryType( Deleter.PhysicalDeviceHandle, memoryPropertyFlags, memoryRequirements.memoryTypeBits );

            return memoryAllocInfo;
        }

        bool BindMemory( VkDeviceMemory hMemory, uint32_t Offset = 0 ) {
            return VK_SUCCESS == CheckedResult( vkBindBufferMemory( Deleter.hLogicalDevice, Handle, hMemory, Offset ) );
        }
    };

    template <>
    struct THandle< VkBufferView > : public THandleBase< VkBufferView > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkBufferViewCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedResult( vkCreateBufferView( InLogicalDeviceHandle, &CreateInfo, GetAllocationCallbacks( ), &Handle ) );
        }
    };

    template <>
    struct THandle< VkFramebuffer > : public THandleBase< VkFramebuffer > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkFramebufferCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedResult( vkCreateFramebuffer( InLogicalDeviceHandle, &CreateInfo, GetAllocationCallbacks( ), &Handle ) );
        }
    };

    template <>
    struct THandle< VkRenderPass > : public THandleBase< VkRenderPass > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkRenderPassCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedResult( vkCreateRenderPass( InLogicalDeviceHandle, &CreateInfo, GetAllocationCallbacks( ), &Handle ) );
        }
    };

    template <>
    struct THandle< VkPipelineCache > : public THandleBase< VkPipelineCache > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkPipelineCacheCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedResult( vkCreatePipelineCache( InLogicalDeviceHandle, &CreateInfo, GetAllocationCallbacks( ), &Handle ) );
        }
    };

    template <>
    struct THandle< VkPipelineLayout > : public THandleBase< VkPipelineLayout > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkPipelineLayoutCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedResult( vkCreatePipelineLayout( InLogicalDeviceHandle, &CreateInfo, GetAllocationCallbacks( ), &Handle ) );
        }
    };

    template <>
    struct THandle< VkDescriptorSetLayout > : public THandleBase< VkDescriptorSetLayout > {

        THandle( ) {
            Handle                 = VK_NULL_HANDLE;
            Deleter.hLogicalDevice = VK_NULL_HANDLE;
        }

        THandle( THandle< VkDescriptorSetLayout > &&Other ) {
            Handle                 = Other.Release( );
            Deleter.hLogicalDevice = Other.Deleter.hLogicalDevice;
        }

        THandle( VkDevice InLogicalDeviceHandle, VkDescriptorSetLayout SetLayoutHandle ) {
            Handle                 = SetLayoutHandle;
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
        }

        bool Recreate( VkDevice InLogicalDeviceHandle, VkDescriptorSetLayoutCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedResult( vkCreateDescriptorSetLayout( InLogicalDeviceHandle, &CreateInfo, GetAllocationCallbacks( ), &Handle ) );
        }
    };

    template <>
    struct THandle< VkDescriptorSet > : public THandleBase< VkDescriptorSet > {
        bool Recreate( VkDevice                           InLogicalDeviceHandle,
                       VkDescriptorPool                   InDescPoolHandle,
                       VkDescriptorSetAllocateInfo const &AllocInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedResult( vkAllocateDescriptorSets( InLogicalDeviceHandle, &AllocInfo, &Handle ) );
        }
    };

    template <>
    struct THandle< VkDescriptorPool > : public THandleBase< VkDescriptorPool > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkDescriptorPoolCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedResult( vkCreateDescriptorPool( InLogicalDeviceHandle, &CreateInfo, GetAllocationCallbacks( ), &Handle ) );
        }
    };

    template <>
    struct THandle< VkPipeline > : public THandleBase< VkPipeline > {
        bool Recreate( VkDevice                            InLogicalDeviceHandle,
                       VkPipelineCache                     pCache,
                       VkGraphicsPipelineCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedResult( vkCreateGraphicsPipelines(
                                     InLogicalDeviceHandle, pCache, 1, &CreateInfo, GetAllocationCallbacks( ), &Handle ) );
        }

        bool Recreate( VkDevice InLogicalDeviceHandle, VkPipelineCache pCache, VkComputePipelineCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedResult( vkCreateComputePipelines(
                                     InLogicalDeviceHandle, pCache, 1, &CreateInfo, GetAllocationCallbacks( ), &Handle ) );
        }
    };

    template <>
    struct THandle< VkShaderModule > : public THandleBase< VkShaderModule > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkShaderModuleCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedResult( vkCreateShaderModule( InLogicalDeviceHandle, &CreateInfo, GetAllocationCallbacks( ), &Handle ) );
        }
    };

    template <>
    struct THandle< VkSemaphore > : public THandleBase< VkSemaphore > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkSemaphoreCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedResult( vkCreateSemaphore( InLogicalDeviceHandle, &CreateInfo, GetAllocationCallbacks( ), &Handle ) );
        }
    };

    template <>
    struct THandle< VkSampler > : public THandleBase< VkSampler > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkSamplerCreateInfo const &CreateInfo ) {
            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedResult( vkCreateSampler( InLogicalDeviceHandle, &CreateInfo, GetAllocationCallbacks( ), &Handle ) );
        }
    };

    template <>
    struct THandle< VmaAllocator > : public THandleBase< VmaAllocator > {
        bool Recreate( VmaAllocatorCreateInfo const &CreateInfo ) {
            Deleter( Handle );
            return VK_SUCCESS == CheckedResult( vmaCreateAllocator( &CreateInfo, &Handle ) );
        }
    };
}
