#pragma once

#include <ResultHandle.Vulkan.h>
#include <type_traits>

namespace apemodevk {

const VkAllocationCallbacks *GetAllocationCallbacks( );

template < typename TNativeHandle >
struct THandleDeleter {
    void operator( )( TNativeHandle *&Handle ) {
        apemode_error( "Please, specialize the proprietary deleter for your class." );
        Handle = VK_NULL_HANDLE;
    }
};

//  Dispatchable handle types are a pointer to an opaque type. This pointer may be used by layers as part of
//  intercepting API commands, and thus each API command takes a dispatchable type as its first parameter. Each
//  object of a dispatchable type has a unique handle value.
template < typename TNativeHandle_, typename TDeleter_ = THandleDeleter< TNativeHandle_ > >
struct THandleBase : public NoCopyAssignPolicy {
    typedef TDeleter_                              TDeleter;
    typedef TNativeHandle_                         TNativeHandle;
    typedef THandleBase< TNativeHandle, TDeleter > SelfType;

    TNativeHandle Handle;
    TDeleter      Deleter;

    void Swap( SelfType &Other ) {
        eastl::swap( Handle, Other.Handle );
        eastl::swap( Deleter, Other.Deleter );
    }

    inline THandleBase( ) : Handle( VK_NULL_HANDLE ) {
    }

    inline THandleBase( SelfType &&Other ) {
        Swap( Other );
    }

    inline ~THandleBase( ) {
        Destroy( );
    }

    inline bool IsNull( ) const {
        return Handle == nullptr;
    }

    inline bool IsNotNull( ) const {
        return Handle != nullptr;
    }

    inline void Destroy( ) {
        Deleter( Handle );
    }

    inline operator TNativeHandle( ) const {
        return Handle;
    }

    inline operator bool( ) const {
        return nullptr != Handle;
    }

    inline TNativeHandle const *GetAddressOf( ) const {
        return &Handle;
    }

    inline TNativeHandle *GetAddressOf( ) {
        return &Handle;
    }

    SelfType &operator=( SelfType &&Other ) {
        Swap( Other );
        return *this;
    }

    TNativeHandle Release( ) {
        TNativeHandle ReleasedHandle = *this;
        Handle                       = VK_NULL_HANDLE;
        Deleter                      = TDeleter{};
        return ReleasedHandle;
    }
};

} // namespace apemodevk