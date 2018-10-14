#pragma once

#include <apemode/vk/GraphicsDevice.Vulkan.h>
#include <apemode/vk/NativeHandles.Vulkan.h>

namespace apemodevk {

struct BufferComposite {
    VkBuffer          pBuffer        = VK_NULL_HANDLE;
    VmaAllocator      pAllocator     = VK_NULL_HANDLE;
    VmaAllocation     pAllocation    = VK_NULL_HANDLE;
    VmaAllocationInfo AllocationInfo = {};
};

template <>
struct THandleDeleter< BufferComposite > {
    void operator( )( BufferComposite &bufferComposite ) {
        if ( nullptr == bufferComposite.pBuffer )
            return;

        assert( bufferComposite.pAllocator );
        assert( bufferComposite.pAllocation );

        vmaDestroyBuffer( bufferComposite.pAllocator, bufferComposite.pBuffer, bufferComposite.pAllocation );

        bufferComposite.pBuffer     = VK_NULL_HANDLE;
        bufferComposite.pAllocator  = VK_NULL_HANDLE;
        bufferComposite.pAllocation = VK_NULL_HANDLE;
    }
};

template <>
struct THandle< BufferComposite > : public NoCopyAssignPolicy {
    typedef THandleDeleter< BufferComposite > TDeleter;
    typedef THandle< BufferComposite >        SelfType;

    BufferComposite Handle;
    TDeleter        Deleter;

    inline bool Recreate( VmaAllocator                   pAllocator,
                          const VkBufferCreateInfo &     bufferCreateInfo,
                          const VmaAllocationCreateInfo &allocationCreateInfo ) {
        apemodevk_memory_allocation_scope;

        Deleter( Handle );
        Handle.pAllocator = pAllocator;

        return VK_SUCCESS == CheckedResult( vmaCreateBuffer( pAllocator,
                                                             &bufferCreateInfo,
                                                             &allocationCreateInfo,
                                                             &Handle.pBuffer,
                                                             &Handle.pAllocation,
                                                             &Handle.AllocationInfo ) );
    }

    inline void Swap( SelfType &Other ) {
        eastl::swap( Handle, Other.Handle );
        eastl::swap( Deleter, Other.Deleter );
    }

    inline BufferComposite Release( ) {
        BufferComposite ReleasedHandle = Handle;
        Handle.pAllocation             = nullptr;
        Handle.pAllocator              = nullptr;
        Handle.pBuffer                 = nullptr;
        return ReleasedHandle;
    }

    inline THandle( ) : Handle( ), Deleter( ) {
    }

    inline THandle( THandle &&Other ) {
        Swap( Other );
    }

    inline SelfType &operator=( SelfType &&Other ) {
        Swap( Other );
        return *this;
    }

    inline ~THandle( ) {
        Destroy( );
    }

    inline bool IsNull( ) const {
        return nullptr == Handle.pBuffer;
    }

    inline bool IsNotNull( ) const {
        return nullptr != Handle.pBuffer;
    }

    inline void Destroy( ) {
        Deleter( Handle );
    }

    inline operator VkBuffer( ) const {
        return Handle.pBuffer;
    }

    inline operator bool( ) const {
        return nullptr != Handle.pBuffer;
    }

    inline VkBuffer const *GetAddressOf( ) const {
        return &Handle.pBuffer;
    }

    inline VkBuffer *GetAddressOf( ) {
        return &Handle.pBuffer;
    }
};

typedef THandle< BufferComposite > BufferCompositeHandle;

inline uint8_t *MapStagingBuffer( GraphicsDevice *pNode, THandle< BufferComposite > &hBuffer ) {
    if ( hBuffer.Handle.AllocationInfo.pMappedData ) {
        return reinterpret_cast< uint8_t * >( hBuffer.Handle.AllocationInfo.pMappedData );
    }

    uint8_t *pMapped = nullptr;
    if ( VK_SUCCESS ==
         CheckedResult( vmaMapMemory( pNode->hAllocator, hBuffer.Handle.pAllocation, (void **) ( &pMapped ) ) ) ) {
        return pMapped;
    }

    return nullptr;
}

inline void UnmapStagingBuffer( GraphicsDevice *pNode, THandle< BufferComposite > &hBuffer ) {
    if ( !hBuffer.Handle.AllocationInfo.pMappedData ) {
        vmaUnmapMemory( pNode->hAllocator, hBuffer.Handle.pAllocation );
    }
}

} // namespace apemodevk