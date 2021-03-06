#include "BufferPools.Vulkan.h"
#include <memory.h>

inline uint32_t AlignedOffsetCount( uint32_t offset, uint32_t alignment ) {
    return offset / alignment + ( uint32_t ) !!( offset % alignment );
}

inline uint32_t AlignedOffset( uint32_t offset, uint32_t alignment ) {
    return alignment * ( offset / alignment + ( uint32_t ) !!( offset % alignment ) );
}

bool apemodevk::HostBufferPool::Page::Recreate( GraphicsDevice *      pInNode,
                                                uint32_t              alignment,
                                                uint32_t              bufferRange,
                                                VkBufferUsageFlags    bufferUsageFlags,
                                                VkMemoryPropertyFlags memoryPropertyFlags ) {
    apemodevk_memory_allocation_scope;

    pNode                = pInNode;
    eMemoryPropertyFlags = memoryPropertyFlags;
    TotalOffsetCount     = bufferRange / alignment;
    TotalSize            = TotalOffsetCount * alignment;
    Alignment            = alignment;
    pMapped              = nullptr;

#if _apemodevk_HostBufferPool_Page_InvalidateOrFlushAllRanges
    Ranges.reserve( eastl::min< uint32_t >( 64, TotalOffsetCount ) );
#else
    InitializeStruct( Range );
#endif

    VkBufferCreateInfo bufferCreateInfo;
    InitializeStruct( bufferCreateInfo );
    bufferCreateInfo.size  = TotalSize;
    bufferCreateInfo.usage = bufferUsageFlags;

    VmaAllocationCreateInfo allocationCreateInfo;
    InitializeStruct( allocationCreateInfo );
    allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    if ( false == hBuffer.Recreate( pNode->hAllocator, bufferCreateInfo, allocationCreateInfo ) ) {
        apemodevk::platform::DebugBreak( );
        return false;
    }

    return Reset( );
}

bool apemodevk::HostBufferPool::Page::Reset( ) {
    apemodevk_memory_allocation_scope;

    if ( false == HasFlagEq( eMemoryPropertyFlags, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) ) {
#if _apemodevk_HostBufferPool_Page_InvalidateOrFlushAllRanges
        if ( !Ranges.empty( ) ) {
            CheckedResult( vkInvalidateMappedMemoryRanges( pNode->hLogicalDevice, (uint32_t) Ranges.size( ), Ranges.data( ) ) );
            Ranges.clear( );
        }
#else
        if ( nullptr != pMapped && 0 != CurrentOffsetIndex ) {
            Range.memory = hBuffer.Handle.AllocationInfo.deviceMemory;
            Range.size   = CurrentOffsetIndex * Alignment;

            if ( VK_SUCCESS != vkInvalidateMappedMemoryRanges( pNode->hLogicalDevice, 1, &Range ) ) {
                apemodevk::platform::DebugBreak( );
            }
        }
#endif
    }

    if ( nullptr == pMapped ) {
        CurrentOffsetIndex = 0;

        /* If it failed to map buffer upon the creation, try to map it here. Return false if failed. */
        if ( hBuffer.Handle.AllocationInfo.pMappedData ) {
            pMapped = reinterpret_cast< uint8_t * >( hBuffer.Handle.AllocationInfo.pMappedData );
        } else if ( VK_SUCCESS != CheckedResult( vmaMapMemory( pNode->hAllocator, hBuffer.Handle.pAllocation, (void **) ( &pMapped ) ) ) ) {
            return false;
        }
    }

    assert(pMapped);
    return nullptr != pMapped;
}

bool apemodevk::HostBufferPool::Page::Flush( ) {
    apemodevk_memory_allocation_scope;

    if ( nullptr != pMapped ) {
        if ( false == HasFlagEq( eMemoryPropertyFlags, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) ) {
#if _apemodevk_HostBufferPool_Page_InvalidateOrFlushAllRanges
            if ( !Ranges.empty( ) ) {
                CheckedResult( vkFlushMappedMemoryRanges( pNode->hLogicalDevice, (uint32_t) Ranges.size( ), Ranges.data( ) ) );
                Ranges.clear();
            }
#else
            if ( 0 != CurrentOffsetIndex ) {
                Range.memory = hBuffer.Handle.AllocationInfo.deviceMemory;
                Range.size   = CurrentOffsetIndex * Alignment;

                CheckedResult( vkFlushMappedMemoryRanges( pNode->hLogicalDevice, 1, &Range ) );
            }
#endif
        }

        if ( nullptr == hBuffer.Handle.AllocationInfo.pMappedData ) {
            vmaUnmapMemory( pNode->hAllocator, hBuffer.Handle.pAllocation );
        }

        pMapped = nullptr;
    }

    return true;
}

void apemodevk::HostBufferPool::Recreate( GraphicsDevice *pInNode, VkBufferUsageFlags bufferUsageFlags, bool bInHostCoherent ) {
    apemodevk_memory_allocation_scope;

    Destroy( );

    pNode = pInNode;
    eMemoryPropertyFlags |= bInHostCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0;

    Pages.reserve( 16 );

    if ( 0 != bufferUsageFlags )
        eBufferUsageFlags = bufferUsageFlags;

    const VkPhysicalDeviceLimits & limits = pNode->AdapterProps.limits;

    /* Use defaults otherwise. */
    if ( HasFlagEq( eBufferUsageFlags, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT ) ) {
        MaxPageRange = eastl::min<uint32_t>( 65536, limits.maxUniformBufferRange );
        MinAlignment = (uint32_t) limits.minUniformBufferOffsetAlignment;
    } else if ( HasFlagEq( eBufferUsageFlags, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT ) ) {
        MaxPageRange = limits.maxStorageBufferRange; /* TODO: There is a limit for the number of texel elements */
        MinAlignment = (uint32_t) limits.minTexelBufferOffsetAlignment;
    } else /* VK_BUFFER_USAGE_STORAGE_BUFFER_BIT or other usages */ {
        MaxPageRange = limits.maxStorageBufferRange;
        MinAlignment = (uint32_t) limits.minStorageBufferOffsetAlignment;
    }

    /* Catch incorrectly initialized device limits. */
    assert( 0 != MaxPageRange );
    assert( 0 != MinAlignment );
}

apemodevk::HostBufferPool::Page *apemodevk::HostBufferPool::FindPage( uint32_t dataStructureSize ) {
    apemodevk_memory_allocation_scope;

    /* Ensure it is possible to allocate that space. */
    if ( dataStructureSize > MaxPageRange ) {
        apemodevk::platform::DebugBreak( );
        return nullptr;
    }

    /* Calculate how many chunks the data covers. */
    const uint32_t coveredOffsetCount = dataStructureSize / MinAlignment + ( uint32_t )( 0 != dataStructureSize % MinAlignment );

    /* Try to find an existing free page. */
    auto pageIt = eastl::find_if( Pages.begin( ), Pages.end( ), [&]( PagePtr & pPage ) {
        assert( nullptr != pPage );
        const uint32_t availableOffsets = pPage->TotalOffsetCount - pPage->CurrentOffsetIndex;
        return availableOffsets >= coveredOffsetCount;
    } );

    /* Return free page. */
    if ( pageIt != Pages.end( ) ) {
        return pageIt->get();
    }

    /* Allocate new page and return. */
    Pages.push_back( apemodevk::make_unique< Page >( ) );

    Pages.back( )->Recreate( pNode,
                             MinAlignment,
                             HasFlagEq( eBufferUsageFlags, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT )
                                 ? MaxPageRange
                                 : eastl::min( MaxPageRange, AlignedOffset( dataStructureSize, MinAlignment ) ),
                             eBufferUsageFlags,
                             eMemoryPropertyFlags );

    return Pages.back( ).get( );
}

void apemodevk::HostBufferPool::Flush( ) {
    eastl::for_each( Pages.begin( ), Pages.end( ), [&]( PagePtr & pPage ) {
        assert( nullptr != pPage );
        pPage->Flush( );
    } );
}

void apemodevk::HostBufferPool::Reset( ) {
    eastl::for_each( Pages.begin( ), Pages.end( ), [&]( PagePtr & pPage ) {
        assert( nullptr != pPage );
        pPage->Reset( );
    } );
}

void apemodevk::HostBufferPool::Destroy( ) {
    Pages.clear( );
}

uint32_t apemodevk::HostBufferPool::Page::Push( const void *pDataStructure, uint32_t ByteSize ) {
    apemodevk_memory_allocation_scope;

    const uint32_t coveredOffsetCount   = ByteSize / Alignment + ( uint32_t )( 0 != ByteSize % Alignment );
    const uint32_t availableOffsetCount = TotalOffsetCount - CurrentOffsetIndex;

    assert( nullptr != pMapped );
    assert( coveredOffsetCount <= availableOffsetCount );

    const uint32_t currentMappedOffset = CurrentOffsetIndex * Alignment;

    if ( false == HasFlagEq( eMemoryPropertyFlags, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) ) {
#if _apemodevk_HostBufferPool_Page_InvalidateOrFlushAllRanges
        /* Fill current range for flush. */
        Ranges.emplace_back( );

        auto &range = Ranges.back( );
        InitializeStruct( range );
        range.offset = currentMappedOffset;
        range.size   = ByteSize;
        range.memory = hBuffer.Handle.AllocationInfo.deviceMemory;
#endif
    }

    /* Get current memory pointer and copy there. */
    auto mappedData = pMapped + currentMappedOffset;
    memcpy( mappedData, pDataStructure, ByteSize );
    CurrentOffsetIndex += coveredOffsetCount;

    return currentMappedOffset;
}

apemodevk::HostBufferPool::SuballocResult apemodevk::HostBufferPool::Suballocate( const void *pDataStructure,
                                                                                  uint32_t    ByteSize ) {
    apemodevk_memory_allocation_scope;

    SuballocResult suballocResult;
    InitializeStruct( suballocResult.DescriptorBufferInfo );

    if ( auto pPage = FindPage( ByteSize ) ) {
        suballocResult.DescriptorBufferInfo.buffer = pPage->hBuffer;
        suballocResult.DescriptorBufferInfo.range  = pPage->TotalSize;
        suballocResult.DynamicOffset         = pPage->Push( pDataStructure, ByteSize );
    }

    return suballocResult;
}
