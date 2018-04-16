
#include <GraphicsDevice.Vulkan.h>
#include <NativeHandles.Vulkan.h>

namespace apemodevk {

    struct BufferComposite
    {
        VkBuffer          pBuffer     = VK_NULL_HANDLE;
        VmaAllocator      pAllocator  = VK_NULL_HANDLE;
        VmaAllocation     pAllocation = VK_NULL_HANDLE;
        VmaAllocationInfo allocInfo   = {};
    };

    template <>
    struct THandleDeleter< BufferComposite > : public THandleHandleTypeResolver< BufferComposite > {

        void operator( )( BufferComposite &bufferComposite ) {
            if ( bufferComposite.pBuffer )
                return;

            assert( bufferComposite.pAllocator );
            assert( bufferComposite.pAllocation );

            vmaDestroyBuffer( bufferComposite.pAllocator,
                              bufferComposite.pBuffer,
                              bufferComposite.pAllocation );

            bufferComposite.pBuffer = VK_NULL_HANDLE;
            bufferComposite.pAllocator = VK_NULL_HANDLE;
            bufferComposite.pAllocation = VK_NULL_HANDLE;
        }
    };


    template <>
    struct THandle< BufferComposite > : public THandleBase< BufferComposite > {

        bool Recreate( VmaAllocator                    pAllocator,
                       const VkBufferCreateInfo  &     createInfo,
                       const VmaAllocationCreateInfo & allocInfo ) {

            Deleter( Handle );
            Handle.pAllocator = pAllocator;
            return VK_SUCCESS == CheckedCall( vmaCreateBuffer( pAllocator,
                                                               &createInfo,
                                                               &allocInfo,
                                                               &Handle.pBuffer,
                                                               &Handle.pAllocation,
                                                               &Handle.allocInfo ) );
        }

        VkMemoryRequirements GetMemoryRequirements( ) {
            apemode_assert( IsNotNull( ), "Null." );

            VkMemoryRequirements memoryRequirements;
            vkGetBufferMemoryRequirements( Handle.pAllocator->m_hDevice,
                                           Handle.pAllocator->GetAllocationCallbacks( ),
                                           &memoryRequirements );

            return memoryRequirements;
        }

        VkMemoryAllocateInfo GetMemoryAllocateInfo( VkMemoryPropertyFlags memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) {
            VkMemoryRequirements memoryRequirements = GetMemoryRequirements( );

            VkMemoryAllocateInfo memoryAllocInfo;
            apemodevk::ZeroMemory( memoryAllocInfo );
            memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memoryAllocInfo.allocationSize = memoryRequirements.size;
            memoryAllocInfo.memoryTypeIndex = ResolveMemoryType( Handle.pAllocator->m_MemProps, memoryPropertyFlags, memoryRequirements.memoryTypeBits );

            return memoryAllocInfo;
        }

        bool BindMemory( VkDeviceMemory hMemory, uint32_t Offset = 0 ) {
            return VK_SUCCESS == CheckedCall( vkBindBufferMemory( Handle.pAllocator->m_hDevice, *this, hMemory, Offset ) );
        }
    };

    typedef THandle< BufferComposite > BufferCompositeHandle;

}