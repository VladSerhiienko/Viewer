#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <NativeHandles.Vulkan.h>

namespace apemodevk {

    struct ImageComposite {
        VkImage           pImg        = VK_NULL_HANDLE;
        VmaAllocator      pAllocator  = VK_NULL_HANDLE;
        VmaAllocation     pAllocation = VK_NULL_HANDLE;
        VmaAllocationInfo allocInfo   = {};
    };

    template <>
    struct THandleDeleter< ImageComposite > {

        void operator( )( ImageComposite &imgComposite ) {
            if ( nullptr == imgComposite.pImg )
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
    struct THandle< ImageComposite > : public NoCopyAssignPolicy {

        typedef THandleDeleter< ImageComposite > TDeleter;
        typedef THandle< ImageComposite >        SelfType;

        ImageComposite Handle;
        TDeleter     Deleter;

        bool Recreate( VmaAllocator                    pAllocator,
                       const VkImageCreateInfo  &      createInfo,
                       const VmaAllocationCreateInfo & allocInfo ) {
            apemodevk_memory_allocation_scope;

            Deleter( Handle );
            Handle.pAllocator = pAllocator;
            return VK_SUCCESS == CheckedResult( vmaCreateImage( pAllocator,
                                                              &createInfo,
                                                              &allocInfo,
                                                              &Handle.pImg,
                                                              &Handle.pAllocation,
                                                              &Handle.allocInfo ) );
        }

        inline THandle( ) = default;
        inline THandle( THandle &&Other ) { Handle = Other.Release( ); }
        inline ~THandle( ) { Destroy( ); }

        inline bool IsNull( ) const { return nullptr == Handle.pImg; }
        inline bool IsNotNull( ) const { return nullptr != Handle.pImg; }
        inline void Destroy( ) { Deleter( Handle ); }

        inline operator VkImage ( ) const { return Handle.pImg; }
        inline operator bool( ) const { return nullptr != Handle.pImg; }
        inline VkImage *operator( )( ) { return &Handle.pImg; }
        inline operator VkImage *( ) { return &Handle.pImg; }
        inline operator VkImage const *( ) const { return &Handle.pImg; }

        SelfType & operator=( SelfType &&Other ) {
            Handle = Other.Release( );
            return *this;
        }

        ImageComposite Release( ) {
            ImageComposite ReleasedHandle = Handle;
            Handle.pAllocation           = nullptr;
            Handle.pAllocator            = nullptr;
            Handle.pImg               = nullptr;
            return ReleasedHandle;
        }

        void Swap( SelfType &Other ) {
            std::swap( Handle, Other.Handle );
            std::swap( Deleter, Other.Deleter );
        }
    };

}