#pragma once

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
    struct THandleDeleter< BufferComposite > {

        void operator( )( BufferComposite &bufferComposite ) {
            if ( nullptr == bufferComposite.pBuffer )
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
    struct THandle< BufferComposite > : public NoCopyAssignPolicy {
        typedef THandleDeleter< BufferComposite > TDeleter;
        typedef THandle< BufferComposite >        SelfType;

        BufferComposite Handle;
        TDeleter        Deleter;

        bool Recreate( VmaAllocator                   pAllocator,
                       const VkBufferCreateInfo &     createInfo,
                       const VmaAllocationCreateInfo &allocInfo ) {
            apemodevk_memory_allocation_scope;

            Deleter( Handle );
            Handle.pAllocator = pAllocator;

            return VK_SUCCESS == CheckedCall( vmaCreateBuffer( pAllocator,
                                                               &createInfo,
                                                               &allocInfo,
                                                               &Handle.pBuffer,
                                                               &Handle.pAllocation,
                                                               &Handle.allocInfo ) );
        }

        inline THandle( ) = default;
        inline THandle( THandle &&Other ) { Handle = Other.Release( ); }
        inline ~THandle( ) { Destroy( ); }

        inline bool IsNull( ) const { return nullptr == Handle.pBuffer; }
        inline bool IsNotNull( ) const { return nullptr != Handle.pBuffer; }
        inline void Destroy( ) { Deleter( Handle ); }

        inline operator VkBuffer ( ) const { return Handle.pBuffer; }
        inline operator bool( ) const { return nullptr != Handle.pBuffer; }
        inline VkBuffer *operator( )( ) { return &Handle.pBuffer; }
        inline operator VkBuffer *( ) { return &Handle.pBuffer; }
        inline operator VkBuffer const *( ) const { return &Handle.pBuffer; }

        SelfType & operator=( SelfType &&Other ) {
            Handle = Other.Release( );
            return *this;
        }

        BufferComposite Release( ) {
            BufferComposite ReleasedHandle = Handle;
            Handle.pAllocation           = nullptr;
            Handle.pAllocator            = nullptr;
            Handle.pBuffer               = nullptr;
            return ReleasedHandle;
        }

        void Swap( SelfType &Other ) {
            std::swap( Handle, Other.Handle );
            std::swap( Deleter, Other.Deleter );
        }
    };

    typedef THandle< BufferComposite > BufferCompositeHandle;

}