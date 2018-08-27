#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <NativeHandles.Vulkan.h>

namespace apemodevk {

struct ImageComposite {
    VkImage           pImg           = VK_NULL_HANDLE;
    VmaAllocator      pAllocator     = VK_NULL_HANDLE;
    VmaAllocation     pAllocation    = VK_NULL_HANDLE;
    VmaAllocationInfo AllocationInfo = {};
};

template <>
struct THandleDeleter< ImageComposite > {
    void operator( )( ImageComposite &imgComposite ) {
        if ( nullptr == imgComposite.pImg )
            return;

        assert( imgComposite.pAllocator );
        assert( imgComposite.pAllocation );

        vmaDestroyImage( imgComposite.pAllocator, imgComposite.pImg, imgComposite.pAllocation );

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
    TDeleter       Deleter;

    inline bool Recreate( VmaAllocator                   pAllocator,
                          const VkImageCreateInfo &      imageCreateInfo,
                          const VmaAllocationCreateInfo &allocationCreateInfo ) {
        apemodevk_memory_allocation_scope;

        Deleter( Handle );
        Handle.pAllocator = pAllocator;

        return VK_SUCCESS == CheckedResult( vmaCreateImage( pAllocator,
                                                            &imageCreateInfo,
                                                            &allocationCreateInfo,
                                                            &Handle.pImg,
                                                            &Handle.pAllocation,
                                                            &Handle.AllocationInfo ) );
    }

    inline ImageComposite Release( ) {
        ImageComposite ReleasedHandle = Handle;
        Handle.pAllocation            = nullptr;
        Handle.pAllocator             = nullptr;
        Handle.pImg                   = nullptr;
        return ReleasedHandle;
    }

    inline void Swap( SelfType &Other ) {
        std::swap( Handle, Other.Handle );
        std::swap( Deleter, Other.Deleter );
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
        return nullptr == Handle.pImg;
    }

    inline bool IsNotNull( ) const {
        return nullptr != Handle.pImg;
    }

    inline void Destroy( ) {
        Deleter( Handle );
    }

    inline operator VkImage( ) const {
        return Handle.pImg;
    }

    inline operator bool( ) const {
        return nullptr != Handle.pImg;
    }

    inline VkImage const *GetAddressOf( ) const {
        return &Handle.pImg;
    }

    inline VkImage *GetAddressOf( ) {
        return &Handle.pImg;
    }
};

} // namespace apemodevk