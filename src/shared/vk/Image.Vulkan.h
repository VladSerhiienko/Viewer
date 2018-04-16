
#include <GraphicsDevice.Vulkan.h>
#include <NativeHandles.Vulkan.h>

namespace apemodevk {

    struct ImgComposite {
        VkImage           pImg        = VK_NULL_HANDLE;
        VmaAllocator      pAllocator  = VK_NULL_HANDLE;
        VmaAllocation     pAllocation = VK_NULL_HANDLE;
        VmaAllocationInfo allocInfo   = {};
    };

    template <>
    struct THandleDeleter< ImgComposite > : THandleHandleTypeResolver< ImgComposite > {

        void operator( )( ImgComposite &imgComposite ) {
            if ( imgComposite.pBuffer )
                return;

            assert( imgComposite.pAllocator );
            assert( imgComposite.pAllocation );

            vmaDestroyImage( imgComposite.pAllocator,
                             imgComposite.pImg,
                             imgComposite.pAllocation );

            imgComposite.pImg        = VK_NULL_HANDLE;
            imgComposite.pAllocator  = VK_NULL_HANDLE;
            imgComposite.pAllocation = VK_NULL_HANDLE;
        }
    };


    template <>
    struct THandle< ImgComposite > : THandleBase< ImgComposite > {

        bool Recreate( VmaAllocator                    pAllocator,
                       const VkImageCreateInfo  &      createInfo,
                       const VmaAllocationCreateInfo & allocInfo ) {

            Deleter( Handle );
            Handle.pAllocator = pAllocator;
            return VK_SUCCESS == CheckedCall( vmaCreateImage( pAllocator,
                                                              &createInfo,
                                                              &allocInfo,
                                                              &Handle.pImg,
                                                              &Handle.pAllocation,
                                                              &Handle.allocInfo ) );
        }

        VkMemoryRequirements GetMemoryRequirements( ) {
            apemode_assert( IsNotNull( ), "Null." );

            VkMemoryRequirements memoryRequirements;
            vkGetImageMemoryRequirements( Handle.pAllocator->m_hDevice,
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
            return VK_SUCCESS == CheckedCall( vkBindImageMemory( Handle.pAllocator->m_hDevice, *this, hMemory, Offset ) );
        }
    };

}