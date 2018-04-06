#pragma once

#include <SystemAllocationCallbacks.Vulkan.h>
#include <type_traits>

namespace apemodevk
{
    template <typename TNativeHandle>
    struct TDispatchableHandleHandleTypeResolver
    {
        typedef typename std::remove_pointer<TNativeHandle>::type HandleType;
        operator VkAllocationCallbacks const * ()
        {
            return nullptr;
            //return TbbAux::AllocationCallbacks ();
        }
    };

    template <typename TNativeHandle>
    struct TDispatchableHandleDeleter : public TDispatchableHandleHandleTypeResolver<TNativeHandle>,
                                        public apemodevk::ScalableAllocPolicy
    {
        void operator()(TNativeHandle *& Handle)
        // void operator()(HandleType *& Handle)
        {
            apemode_error("Please, specialize the proprietary deleter for your class.");
            Handle = VK_NULL_HANDLE;
        }
    };

    //  Dispatchable handle types are a pointer to an opaque type. This pointer may be used by layers as part of
    //  intercepting API commands, and thus each API command takes a dispatchable type as its first parameter. Each
    //  object of a dispatchable type has a unique handle value.
    template < typename TNativeHandle_, typename TDeleter_ = TDispatchableHandleDeleter< TNativeHandle_ > >
    struct TDispatchableHandleBase : public apemodevk::ScalableAllocPolicy, public apemodevk::NoCopyAssignPolicy {
        typedef TDeleter_ TDeleter;
        typedef TNativeHandle_ TNativeHandle;
        typedef TDispatchableHandleBase<TNativeHandle, TDeleter> SelfType;

        TNativeHandle Handle;
        TDeleter Deleter;

        inline TDispatchableHandleBase() : Handle(VK_NULL_HANDLE) {}
        inline TDispatchableHandleBase(SelfType && Other) : Handle(Other.Release()) {}
        inline ~TDispatchableHandleBase() { Destroy(); }

        inline TNativeHandle * operator()() const { return &Handle; }
        inline operator TNativeHandle *() { return &Handle; }
        inline operator TNativeHandle const *() const { return &Handle; }
        inline operator TNativeHandle() const { return Handle; }
        inline operator bool() const { return Handle != nullptr; }
        inline bool IsNull() const { return Handle == nullptr; }
        inline bool IsNotNull() const { return Handle != nullptr; }
        inline void Destroy() { Deleter(Handle); }

        SelfType operator=(SelfType && Other)
        {
            Handle = Other.Release();
            return *this;
        }

        TNativeHandle Release()
        {
            TNativeHandle ReleasedHandle = *this;
            Handle = VK_NULL_HANDLE;
            return ReleasedHandle;
        }

        operator VkAllocationCallbacks const *()
        {
            return nullptr;
            //TbbAux::AllocationCallbacks AllocCallbacks;
            //return AllocCallbacks;
        }

        void Swap(SelfType & Other)
        {
            std::swap(Handle, Other.Handle);
            std::swap(Deleter, Other.Deleter);
        }

    };
}