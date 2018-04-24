#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <Buffer.Vulkan.h>

#define _apemodevk_HostBufferPool_Page_InvalidateOrFlushAllRanges 1

namespace apemodevk {

    struct HostBufferPool {
        struct Page {
            THandle< BufferComposite > hBuffer;
            GraphicsDevice *           pNode = nullptr;
            //uint8_t *                  pMappedData = nullptr;
            uint32_t                   Alignment = 0;
            uint32_t                   TotalSize = 0;
            uint32_t                   CurrChunkIndex = 0;
            uint32_t                   TotalChunkCount = 0;
            VkMemoryPropertyFlags      eMemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

#if _apemodevk_HostBufferPool_Page_InvalidateOrFlushAllRanges
            std::vector< VkMappedMemoryRange > Ranges;
#else
            VkMappedMemoryRange Range;
#endif

            bool Recreate( GraphicsDevice *      pNode,
                           uint32_t              alignment,
                           uint32_t              bufferRange,
                           VkBufferUsageFlags    bufferUsageFlags,
                           VkMemoryPropertyFlags eMemoryPropertyFlags );

            bool Reset( );

            /* NOTE: Called from pool. */
            bool Flush( );

            /* NOTE: Does not handle space requirements (aborts in debug mode only). */
            uint32_t Push( const void *pDataStructure, uint32_t ByteSize );

            /* NOTE: Does not handle space requirements (aborts in debug mode only). */
            template < typename TDataStructure >
            uint32_t TPush( const TDataStructure &dataStructure ) {
                return Push( &dataStructure, sizeof( TDataStructure ) );
            }
        };

        using PagePtr = std::unique_ptr< Page >;

        struct SuballocResult {
            VkDescriptorBufferInfo descBufferInfo;
            uint32_t               dynamicOffset;
        };

        GraphicsDevice *      pNode = nullptr;
        VkBufferUsageFlags    eBufferUsageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        VkMemoryPropertyFlags eMemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        uint32_t              MinAlignment = 0;
        uint32_t              MaxPageRange = 0;
        std::vector< PagePtr > Pages;

        /**
         * @param pInLogicalDevice Logical device.
         * @param pInPhysicalDevice Physical device.
         * @param pInDescPool Descriptor pool.
         * @param pInLimits Device limits (null is ok).
         * @param usageFlags Usually UNIFORM.
         **/
        void Recreate( GraphicsDevice *pNode, VkBufferUsageFlags bufferUsageFlags, bool bInHostCoherent );

        Page *FindPage( uint32_t dataStructureSize );

        /* On command buffer submission */
        void Flush( );

        void Reset( );

        void Destroy( );

        SuballocResult Suballocate(const void *pDataStructure, uint32_t ByteSize);

        template < typename TDataStructure >
        SuballocResult TSuballocate( const TDataStructure &dataStructure ) {
            return Suballocate( &dataStructure, sizeof( TDataStructure ) );
        }
    };

    struct PoolComposite {
        VmaPool      pPool      = VK_NULL_HANDLE;
        VmaAllocator pAllocator = VK_NULL_HANDLE;
    };

    template <>
    struct THandleDeleter< VmaPool > {
        VmaAllocator pAllocator = VK_NULL_HANDLE;

        void operator( )( VmaPool &pPool ) {
            if ( nullptr == pPool || nullptr == pAllocator )
                return;

            vmaDestroyPool( pAllocator, pPool );

            pPool      = VK_NULL_HANDLE;
            pAllocator = VK_NULL_HANDLE;
        }
    };

    template <>
    struct THandle< VmaPool > : THandleBase< VmaPool > {

        bool Recreate( VmaAllocator pAllocator, const VmaPoolCreateInfo &poolCreateInfo ) {
            if ( nullptr == pAllocator )
                return false;

            Deleter( Handle );
            Deleter.pAllocator = pAllocator;

            return VK_SUCCESS == CheckedCall( vmaCreatePool( pAllocator, &poolCreateInfo, &Handle ) );
        }

    };

}