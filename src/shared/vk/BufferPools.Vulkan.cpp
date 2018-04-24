#include <GraphicsDevice.Vulkan.h>
#include <BufferPools.Vulkan.h>

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
    pNode = pInNode;
    eMemoryPropertyFlags = memoryPropertyFlags;

    TotalChunkCount = bufferRange / alignment;
    TotalSize = TotalChunkCount * alignment;
    Alignment = alignment;

    VkBufferCreateInfo bufferCreateInfo;
    InitializeStruct( bufferCreateInfo );
    bufferCreateInfo.size  = bufferRange;
    bufferCreateInfo.usage = bufferUsageFlags;

    VmaAllocationCreateInfo allocationCreateInfo;
    InitializeStruct( allocationCreateInfo );
    allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

    if ( HasFlagEq( eMemoryPropertyFlags, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) ) {
        allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

#if _apemodevk_HostBufferPool_Page_InvalidateOrFlushAllRanges
        Ranges.reserve( std::min< uint32_t >( 64, TotalChunkCount ) );
#else
        InitializeStruct( Range );
#endif

    } else {

#if _apemodevk_HostBufferPool_Page_InvalidateOrFlushAllRanges
        Ranges = {};
#else
        InitializeStruct( Range );
#endif

    }

    if ( false == hBuffer.Recreate( pNode->hAllocator, bufferCreateInfo, allocationCreateInfo ) ) {
        apemodevk::platform::DebugBreak( );
        return false;
    }

    vmaGetMemoryTypeProperties( pNode->hAllocator, hBuffer.Handle.AllocationInfo.memoryType, &eMemoryPropertyFlags );

    return Reset( );
}

bool apemodevk::HostBufferPool::Page::Reset( ) {
    if ( false == HasFlagEq( eMemoryPropertyFlags, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) ) {

#if _apemodevk_HostBufferPool_Page_InvalidateOrFlushAllRanges
        // assert( pMappedData );

        if ( false == Ranges.empty( ) ) {
            if ( VK_SUCCESS != vkInvalidateMappedMemoryRanges( pNode->hLogicalDevice, (uint32_t) Ranges.size( ), Ranges.data( ) ) ) {
                apemodevk::platform::DebugBreak( );
                return false;
            }

            Ranges.clear( );
        }
#else
        if ( 0 != CurrChunkIndex ) {
            Range.memory = hBuffer.Handle.AllocationInfo.deviceMemory;
            Range.size = CurrChunkIndex * Alignment;

            if ( VK_SUCCESS != vkInvalidateMappedMemoryRanges( pNode->hLogicalDevice, 1, &Range ) ) {
                apemodevk::platform::DebugBreak( );
                return false;
            }
        }
#endif
    }

    /* TODO: Create descriptor set. */
    return true;
}

bool apemodevk::HostBufferPool::Page::Flush( ) {

    if ( false == HasFlagEq( eMemoryPropertyFlags, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) ) {

#if _apemodevk_HostBufferPool_Page_InvalidateOrFlushAllRanges
        if ( false == Ranges.empty( ) ) {
            if ( VK_SUCCESS != vkFlushMappedMemoryRanges( pNode->hLogicalDevice, (uint32_t) Ranges.size( ), Ranges.data( ) ) ) {
                apemodevk::platform::DebugBreak( );
                return false;
            }
        }
#else
        if ( 0 != CurrChunkIndex ) {
            Range.memory = hBuffer.Handle.AllocationInfo.deviceMemory;
            Range.size = CurrChunkIndex * Alignment;

            if ( VK_SUCCESS != vkFlushMappedMemoryRanges( pNode->hLogicalDevice, 1, &Range ) ) {
                apemodevk::platform::DebugBreak( );
                return false;
            }
        }
#endif
    }

    return true;
}

void apemodevk::HostBufferPool::Recreate( GraphicsDevice *pInNode, VkBufferUsageFlags bufferUsageFlags, bool bInHostCoherent ) {
    Destroy( );

    pNode = pInNode;
    // eMemoryPropertyFlags |= bInHostCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0;

    Pages.reserve( 16 );

    if ( 0 != bufferUsageFlags )
        eBufferUsageFlags = bufferUsageFlags;

    auto &deviceLimits = pNode->GetPhysicalDeviceLimits( );

    /* Use defaults otherwise. */
    if ( HasFlagEq( eBufferUsageFlags, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT ) ) {

        MaxPageRange = deviceLimits.maxUniformBufferRange;
        MinAlignment = deviceLimits.minUniformBufferOffsetAlignment;

    } else if ( HasFlagEq( eBufferUsageFlags, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT ) ) {

        if ( 0 == MaxPageRange )
            // TODO: How to use this one?
            // MaxPageRange = deviceLimits.maxTexelBufferElements;
            MaxPageRange = deviceLimits.maxStorageBufferRange;
        MinAlignment = deviceLimits.minTexelBufferOffsetAlignment;

    } else /* VK_BUFFER_USAGE_STORAGE_BUFFER_BIT */ {

        if ( 0 == MaxPageRange )
            MaxPageRange = deviceLimits.maxStorageBufferRange;
        MinAlignment = deviceLimits.minStorageBufferOffsetAlignment;

    }


    /* Catch incorrectly initialized device limits. */
    assert( 0 != MaxPageRange );
    assert( 0 != MinAlignment );
}

apemodevk::HostBufferPool::Page *apemodevk::HostBufferPool::FindPage( uint32_t dataStructureSize ) {
    /* Ensure it is possible to allocate that space. */
    if ( dataStructureSize > MaxPageRange ) {
        apemodevk::platform::DebugBreak( );
        return nullptr;
    }

    /* Calculate how many chunks the data covers. */
    const uint32_t coveredChunkCount = dataStructureSize / MinAlignment + ( uint32_t )( 0 != dataStructureSize % MinAlignment );

    /* Try to find an existing free page. */
    auto pageIt = std::find_if( Pages.begin( ), Pages.end( ), [&]( PagePtr & pPage ) {
        assert( nullptr != pPage );
        const uint32_t availableChunkCount = pPage->TotalChunkCount - pPage->CurrChunkIndex;
        return availableChunkCount >= coveredChunkCount;
    } );

    /* Return free page. */
    if ( pageIt != Pages.end( ) ) {
        return pageIt->get( );
    }

    /* Allocate new page and return. */
    Pages.push_back( apemodevk::make_unique< Page >( ) );
    Pages.back( )->Recreate( pNode, MinAlignment, MaxPageRange, eBufferUsageFlags, eMemoryPropertyFlags );

    return Pages.back( ).get( );
}

void apemodevk::HostBufferPool::Flush( ) {
    std::for_each( Pages.begin( ), Pages.end( ), [&]( PagePtr &pPage ) {
        assert( pPage );
        pPage->Flush( );
    } );
}

void apemodevk::HostBufferPool::Reset( ) {
    std::for_each( Pages.begin( ), Pages.end( ), [&]( PagePtr &pPage ) {
        assert( pPage );
        pPage->Reset( );
    } );
}

void apemodevk::HostBufferPool::Destroy( ) {
    Pages.clear( );
}

uint32_t apemodevk::HostBufferPool::Page::Push( const void *pDataStructure, uint32_t ByteSize ) {
    const uint32_t coveredChunkCount   = ByteSize / Alignment + ( uint32_t )( 0 != ByteSize % Alignment );
    const uint32_t availableChunkCount = TotalChunkCount - CurrChunkIndex;

    // assert( pMappedData );
    assert( coveredChunkCount <= availableChunkCount );

    const uint32_t currentByteOffset = CurrChunkIndex * Alignment;

#if _apemodevk_HostBufferPool_Page_InvalidateOrFlushAllRanges
    if ( false == HasFlagEq( eMemoryPropertyFlags, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) ) {
        /* Fill current range for flush. */
        Ranges.emplace_back( );

        auto &range = Ranges.back( );
        InitializeStruct( range );
        range.offset = hBuffer.Handle.AllocationInfo.offset + currentByteOffset;
        range.size = ByteSize;
        range.memory = hBuffer.Handle.AllocationInfo.deviceMemory;
    }
#endif

    auto pMappedData = hBuffer.Handle.AllocationInfo.pMappedData;
    if ( nullptr == pMappedData ) {
        vmaMapMemory( pNode->hAllocator, hBuffer.Handle.pAllocation, &pMappedData );
    }

    /* Get current memory pointer and copy there. */
    auto pCurrMappedData = (uint8_t **) pMappedData + currentByteOffset;
    memcpy( pCurrMappedData, pDataStructure, ByteSize );
    CurrChunkIndex += coveredChunkCount;

    if ( nullptr ==  hBuffer.Handle.AllocationInfo.pMappedData ) {
        vmaUnmapMemory( pNode->hAllocator, hBuffer.Handle.pAllocation );
    }

    return currentByteOffset;
}

apemodevk::HostBufferPool::SuballocResult apemodevk::HostBufferPool::Suballocate( const void *pDataStructure,
                                                                                  uint32_t    byteSize ) {
    SuballocResult suballocResult;
    InitializeStruct( suballocResult.descBufferInfo );

    if ( auto pPage = FindPage( byteSize ) ) {
        suballocResult.descBufferInfo.buffer = pPage->hBuffer;
        suballocResult.descBufferInfo.range = pPage->TotalSize;
        suballocResult.dynamicOffset = pPage->Push( pDataStructure, byteSize );
    }

    return suballocResult;
}
