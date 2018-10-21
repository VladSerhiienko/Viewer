#include <QueuePools.Vulkan.h>
#include <GraphicsDevice.Vulkan.h>

/**
 * Helper function to avoid functionality duplication.
 * The idea is to try to get the best queue first.
 * If there is no correspondence, get the one that will work for case.
 * TODO: Rewrite with iterator based approach.
 **/
template < typename TQueueFamilyBasedElement /* must be set */, typename TQueueFamilyBasedCollection /* auto detection */ >
TQueueFamilyBasedElement* TGetPool( TQueueFamilyBasedCollection& Pools, VkQueueFlags queueFlags, bool match ) {
    /* First, try to find the exact match. */
    /* This can be crucial for only trasfer or only compute queues. */
    for ( auto& pool : Pools )
        if ( pool.QueueFamilyProps.queueFlags == queueFlags )
            return &pool;

    /* Try to find something usable. */
    if ( false == match )
        for ( auto& pool : Pools )
            if ( queueFlags == ( pool.QueueFamilyProps.queueFlags & queueFlags ) )
                return &pool;

    /* Nothing available for immediate usage */
    return nullptr;
}

bool apemodevk::QueuePool::Inititalize( GraphicsDevice*                pInNode,
                                        const VkQueueFamilyProperties* pQueuePropsIt,
                                        const VkQueueFamilyProperties* pQueuePropsEnd,
                                        const float*                   pQueuePrioritiesIt,
                                        const float*                   pQueuePrioritiesItEnd ) {
    apemodevk_memory_allocation_scope;

    (void) pQueuePrioritiesIt;
    (void) pQueuePrioritiesItEnd;

    const size_t queueFamilyCount = size_t( std::distance( pQueuePropsIt, pQueuePropsEnd ) );
    if ( !queueFamilyCount )
        return false;

    pNode = pInNode;
    Pools.reserve( queueFamilyCount );
    QueueFamilyIds.reserve( queueFamilyCount );

    eastl::for_each( pQueuePropsIt, pQueuePropsEnd, [&]( const VkQueueFamilyProperties& queueFamilyProperties ) {
        const uint32_t queueFamilyIndex = uint32_t( eastl::distance( pQueuePropsIt, &queueFamilyProperties ) );

        QueueFamilyPool& queueFamilyPool = Pools.emplace_back( );
        queueFamilyPool.Inititalize( pNode, queueFamilyIndex, queueFamilyProperties );

        if ( ( queueFamilyPool.QueueFamilyProps.queueFlags & VK_QUEUE_GRAPHICS_BIT ) == VK_QUEUE_GRAPHICS_BIT ) {
            QueueFamilyIds.insert( eastl::make_pair( VK_QUEUE_GRAPHICS_BIT, queueFamilyIndex ) );
            platform::LogFmt( platform::LogLevel::Info, "QueueFamily #%u: GRAPHICS", queueFamilyIndex );
        }

        if ( ( queueFamilyPool.QueueFamilyProps.queueFlags & VK_QUEUE_COMPUTE_BIT ) == VK_QUEUE_COMPUTE_BIT &&
             ( queueFamilyPool.QueueFamilyProps.queueFlags & VK_QUEUE_GRAPHICS_BIT ) == VK_QUEUE_GRAPHICS_BIT ) {
            QueueFamilyIds.insert( eastl::make_pair( VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT, queueFamilyIndex ) );
            platform::LogFmt( platform::LogLevel::Info, "QueueFamily #%u: GRAPHICS+COMPUTE", queueFamilyIndex );
        }

        if ( ( queueFamilyPool.QueueFamilyProps.queueFlags & VK_QUEUE_COMPUTE_BIT ) == VK_QUEUE_COMPUTE_BIT &&
             ( queueFamilyPool.QueueFamilyProps.queueFlags & VK_QUEUE_GRAPHICS_BIT ) == 0 ) {
            QueueFamilyIds.insert( eastl::make_pair( VK_QUEUE_COMPUTE_BIT, queueFamilyIndex ) );
            platform::LogFmt( platform::LogLevel::Info, "QueueFamily #%u: COMPUTE", queueFamilyIndex );
        }

        if ( ( queueFamilyPool.QueueFamilyProps.queueFlags & VK_QUEUE_TRANSFER_BIT ) == VK_QUEUE_TRANSFER_BIT &&
             ( queueFamilyPool.QueueFamilyProps.queueFlags & VK_QUEUE_GRAPHICS_BIT ) == 0 &&
             ( queueFamilyPool.QueueFamilyProps.queueFlags & VK_QUEUE_COMPUTE_BIT ) == 0 ) {
            QueueFamilyIds.insert( eastl::make_pair( VK_QUEUE_TRANSFER_BIT, queueFamilyIndex ) );
            platform::LogFmt( platform::LogLevel::Info, "QueueFamily #%u: TRANSFER", queueFamilyIndex );
        }
    } );

    return true;
}

void apemodevk::QueuePool::Destroy( ) {
    if ( pNode ) {
        pNode = nullptr;
        vector< QueueFamilyPool >( ).swap( Pools );
        vector_multimap< VkQueueFlags, uint32_t >( ).swap( QueueFamilyIds );
    }
}

apemodevk::QueuePool::QueuePool( ) : pNode( nullptr ) {
}

apemodevk::QueuePool::QueuePool( QueuePool&& other )
    : pNode( other.pNode )
    , Pools( eastl::move( other.Pools ) )
    , QueueFamilyIds( eastl::move( other.QueueFamilyIds ) ) {
}

apemodevk::QueuePool::~QueuePool( ) {
    Destroy( );
}

apemodevk::QueueFamilyPool* apemodevk::QueuePool::GetPool( uint32_t queueFamilyIndex ) {
    if ( queueFamilyIndex < Pools.size( ) )
        return &Pools[ queueFamilyIndex ];
    return nullptr;
}

const apemodevk::QueueFamilyPool* apemodevk::QueuePool::GetPool( uint32_t queueFamilyIndex ) const {
    if ( queueFamilyIndex < Pools.size( ) )
        return &Pools[ queueFamilyIndex ];
    return nullptr;
}

uint32_t apemodevk::QueuePool::GetPoolCount( ) const {
    return (uint32_t) Pools.size( );
}

apemodevk::QueueFamilyPool* apemodevk::QueuePool::GetPool( VkQueueFlags queueFlags, bool match ) {
    return TGetPool< QueueFamilyPool >( Pools, queueFlags, match );
}

const apemodevk::QueueFamilyPool* apemodevk::QueuePool::GetPool( VkQueueFlags queueFlags, bool match ) const {
    return TGetPool< const QueueFamilyPool >( Pools, queueFlags, match );
}

apemodevk::AcquiredQueue apemodevk::QueuePool::Acquire( bool bIgnoreFenceStatus, VkQueueFlags queueFlags, bool match ) {
    apemodevk_memory_allocation_scope;

    /* Try to find something usable. */
    if ( auto pool = GetPool( queueFlags, match ) ) {
        auto acquiredQueue = pool->Acquire( bIgnoreFenceStatus );
        if ( VK_NULL_HANDLE != acquiredQueue.pQueue )
            return acquiredQueue;
    }

    /* Nothing available for immediate usage */
    return {};
}

apemodevk::AcquiredQueue apemodevk::QueuePool::Acquire( bool bIgnoreFenceStatus, uint32_t queueFamilyIndex ) {
    apemodevk_memory_allocation_scope;

    /* Try to find something usable. */
    if ( auto pool = GetPool( queueFamilyIndex ) ) {
        auto acquiredQueue = pool->Acquire( bIgnoreFenceStatus );
        if ( VK_NULL_HANDLE != acquiredQueue.pQueue )
            return acquiredQueue;
    }

    /* Nothing available for immediate usage */
    return {};
}

void apemodevk::QueuePool::Release( const apemodevk::AcquiredQueue& acquiredQueue ) {
    Pools[ acquiredQueue.QueueFamilyId ].Release( acquiredQueue );
}

bool apemodevk::QueueFamilyPool::Inititalize( GraphicsDevice*                pInNode,
                                              uint32_t                       InQueueFamilyIndex,
                                              VkQueueFamilyProperties const& InQueueFamilyProps ) {
    apemodevk_memory_allocation_scope;

    pNode = pInNode;
    QueueFamilyId    = InQueueFamilyIndex;
    QueueFamilyProps = InQueueFamilyProps;

    Queues.resize( InQueueFamilyProps.queueCount );
    return true;
}

void apemodevk::QueueFamilyPool::Destroy( ) {
    apemodevk_memory_allocation_scope;

    if ( false == Queues.empty( ) ) {
        uint32_t queueIndex = QueueFamilyProps.queueCount;
        while ( queueIndex-- ) {
            if ( VK_NULL_HANDLE != Queues[ queueIndex ].hFence ) {
                vkWaitForFences( *pNode, 1, &Queues[ queueIndex ].hFence, true, UINT64_MAX );
                vkDestroyFence( *pNode, Queues[ queueIndex ].hFence, GetAllocationCallbacks( ) );
            }
        }
    }
}

apemodevk::QueueFamilyPool::QueueFamilyPool( ) : pNode( nullptr ) {
}

apemodevk::QueueFamilyPool::QueueFamilyPool( QueueFamilyPool&& other )
    : pNode( other.pNode ), Queues( eastl::move( other.Queues ) ) {
}

apemodevk::QueueFamilyPool::~QueueFamilyPool( ) {
}

const VkQueueFamilyProperties& apemodevk::QueueFamilyPool::GetQueueFamilyProps( ) const {
    return QueueFamilyProps;
}

bool apemodevk::QueueFamilyPool::SupportsPresenting( VkSurfaceKHR pSurface ) const {
    VkBool32 supported = false;

    switch ( vkGetPhysicalDeviceSurfaceSupportKHR( *pNode, QueueFamilyId, pSurface, &supported ) ) {
        case VK_SUCCESS:
            /* Function succeeded, see the assigned pSucceeded value. */
            return VK_TRUE == supported;

        case VK_ERROR_OUT_OF_HOST_MEMORY:
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        case VK_ERROR_SURFACE_LOST_KHR:
            /* TODO: Cases we expect to happen, need to handle */
            apemodevk::platform::DebugBreak( );
            break;

        default:
            break;
    }

    return false;
}

apemodevk::QueueFamilyBased::QueueFamilyBased( uint32_t queueFamilyId, VkQueueFamilyProperties queueFamilyProperties )
    : QueueFamilyId( queueFamilyId ), QueueFamilyProps( queueFamilyProperties ) {

    /* From the docs: VK_QUEUE_TRANSFER_BIT is enabled if either GRAPHICS or COMPUTE or both are enabled. */
    /* Since we search queues / command buffers according to those flags, we set them here. */
    if ( ( QueueFamilyProps.queueFlags & VK_QUEUE_GRAPHICS_BIT ) == VK_QUEUE_GRAPHICS_BIT ||
         ( QueueFamilyProps.queueFlags & VK_QUEUE_COMPUTE_BIT ) == VK_QUEUE_COMPUTE_BIT ) {

        QueueFamilyProps.queueFlags |= VK_QUEUE_TRANSFER_BIT;
    }
}

bool apemodevk::QueueFamilyBased::SupportsGraphics( ) const {
    return apemodevk::HasFlagEq( QueueFamilyProps.queueFlags, VK_QUEUE_GRAPHICS_BIT );
}

bool apemodevk::QueueFamilyBased::SupportsCompute( ) const {
    return apemodevk::HasFlagEq( QueueFamilyProps.queueFlags, VK_QUEUE_COMPUTE_BIT );
}

bool apemodevk::QueueFamilyBased::IsProtected( ) const {
    return apemodevk::HasFlagEq( QueueFamilyProps.queueFlags, VK_QUEUE_PROTECTED_BIT );
}

bool apemodevk::QueueFamilyBased::SupportsSparseBinding( ) const {
    return apemodevk::HasFlagEq( QueueFamilyProps.queueFlags, VK_QUEUE_SPARSE_BINDING_BIT );
}

bool apemodevk::QueueFamilyBased::SupportsTransfer( ) const {
    return apemodevk::HasFlagEq( QueueFamilyProps.queueFlags, VK_QUEUE_TRANSFER_BIT );
}

apemodevk::AcquiredQueue apemodevk::QueueFamilyPool::Acquire( bool bIgnoreFence ) {
    apemodevk_memory_allocation_scope;

    /* Loop through queues */
    for ( uint32_t i = 0; i < Queues.size( ); ++i ) {
        auto& queue = Queues[ i ];

        /* If the queue is not used by the other thread and it is not executing cmd lists, it will be returned */
        /* Note that queue can suspend the execution of the commands, or discard "one time" buffers. */
        if ( false == queue.bInUse.exchange( true, std::memory_order_acquire ) ) {

            /* Get device queue, return DEVICE_LOST if failed. */
            if ( VK_NULL_HANDLE == queue.hQueue ) {
                vkGetDeviceQueue( *pNode, QueueFamilyId, i, &queue.hQueue );
                assert( queue.hQueue );

                if ( !queue.hQueue ) {
                    /* Move back to unused state */
                    queue.bInUse.exchange( false, std::memory_order_release );

                    AcquiredQueue deviceLostQueue;
                    deviceLostQueue.eResult = VK_ERROR_DEVICE_LOST;
                    return deviceLostQueue;
                }
            }

            /* Create related fence, return false the error code if failed. */
            if ( !queue.hFence ) {
                VkFenceCreateInfo fenceCreateInfo;
                InitializeStruct( fenceCreateInfo );

                /* Since the queue is free to use after creation. */
                /* @note Must be reset before usage. */
                fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

                const VkResult eCreateFenceResult = vkCreateFence( *pNode, &fenceCreateInfo, GetAllocationCallbacks( ), &queue.hFence );
                if ( ( VK_SUCCESS != CheckedResult( eCreateFenceResult ) ) || !queue.hFence ) {
                    /* Move back to unused state */
                    queue.bInUse.exchange( false, std::memory_order_release );

                    // VK_ERROR_OUT_OF_HOST_MEMORY
                    // VK_ERROR_OUT_OF_DEVICE_MEMORY
                    AcquiredQueue outOfMemoryQueue;
                    outOfMemoryQueue.eResult = eCreateFenceResult;
                    return outOfMemoryQueue;
                }
            }

            /* vkGetFenceStatus -> VK_SUCCESS: The fence is signaled. */
            /* vkGetFenceStatus -> VK_NOT_READY: The fence is unsignaled. */
            /* vkGetFenceStatus -> VK_ERROR_DEVICE_LOST: The device is lost. */

            const VkResult eFenceStatus = GetFenceStatus( pNode, queue.hFence );

            if ( VK_SUCCESS > CheckedResult( eFenceStatus ) ) {
                /* Move back to unused state */
                queue.bInUse.exchange( false, std::memory_order_release );

                // VK_ERROR_DEVICE_LOST
                AcquiredQueue deviceLostQueue;
                deviceLostQueue.eResult = eFenceStatus;
                return deviceLostQueue;
            }

            if ( bIgnoreFence || ( VK_SUCCESS == eFenceStatus ) ) {
                AcquiredQueue acquiredQueue;

                acquiredQueue.pQueue        = queue.hQueue;
                acquiredQueue.pFence        = queue.hFence;
                acquiredQueue.QueueFamilyId = QueueFamilyId;
                acquiredQueue.QueueId       = i;

                return acquiredQueue;
            }

            /* Move back to unused state */
            queue.bInUse.exchange( false, std::memory_order_release );
        }
    }

    /* If no free queue found, null is returned */
    AcquiredQueue errorQueue;
    errorQueue.eResult = VK_NOT_READY;
    return errorQueue;
}

bool apemodevk::QueueFamilyPool::Release( const apemodevk::AcquiredQueue& acquiredQueue ) {
    apemodevk_memory_allocation_scope;

    /* Check if the queue was acquired */
    if ( VK_NULL_HANDLE != acquiredQueue.pQueue ) {

        /* No longer used, ok */
        const bool previouslyUsed = Queues[ acquiredQueue.QueueId ].bInUse.exchange( false, std::memory_order_release );
        assert( previouslyUsed && "Trying to release unused queue." );

        return true;
    }

    apemodevk::platform::DebugBreak( );
    return false;
}

apemodevk::QueueInPool::QueueInPool( ) : bInUse( false ) {
}

apemodevk::QueueInPool::QueueInPool( QueueInPool && other )
    : hQueue( other.hQueue )
    , hFence( other.hFence )
    , bInUse( other.bInUse.load( std::memory_order_relaxed ) )
{
}

apemodevk::QueueInPool::QueueInPool( const QueueInPool& other )
    : hQueue( other.hQueue )
    , hFence( other.hFence )
    , bInUse( other.bInUse.load( std::memory_order_relaxed ) ) {
}

bool apemodevk::CommandBufferPool::Inititalize( GraphicsDevice*                pInNode,
                                                const VkQueueFamilyProperties* pQueuePropsIt,
                                                const VkQueueFamilyProperties* pQueuePropsEnd ) {
    apemodevk_memory_allocation_scope;

    pNode = pInNode;
    Pools.reserve( std::distance( pQueuePropsIt, pQueuePropsEnd ) );

    eastl::for_each( pQueuePropsIt, pQueuePropsEnd, [&]( const VkQueueFamilyProperties& InProps ) {
        CommandBufferFamilyPool& pool = Pools.emplace_back( );
        pool.Inititalize( pNode, uint32_t( eastl::distance( pQueuePropsIt, &InProps ) ), InProps );
    } );

    return true;
}

void apemodevk::CommandBufferPool::Destroy( ) {
    if ( pNode ) {
        pNode = nullptr;

        for ( auto& pool : Pools ) {
            pool.Destroy( );
        }

        Pools.clear( );
        vector< CommandBufferFamilyPool >( ).swap( Pools );
    }
}

apemodevk::CommandBufferPool::~CommandBufferPool( ) {
    Destroy( );
}

apemodevk::CommandBufferFamilyPool* apemodevk::CommandBufferPool::GetPool( uint32_t queueFamilyIndex ) {
    if ( queueFamilyIndex < Pools.size( ) )
        return &Pools[ queueFamilyIndex ];
    return nullptr;
}

const apemodevk::CommandBufferFamilyPool* apemodevk::CommandBufferPool::GetPool( uint32_t queueFamilyIndex ) const {
    if ( queueFamilyIndex < Pools.size( ) )
        return &Pools[ queueFamilyIndex ];
    return nullptr;
}

uint32_t apemodevk::CommandBufferPool::GetPoolCount( ) const {
    return (uint32_t) Pools.size( );
}

apemodevk::CommandBufferFamilyPool* apemodevk::CommandBufferPool::GetPool( VkQueueFlags queueFlags, bool match ) {
    return TGetPool< CommandBufferFamilyPool >( Pools, queueFlags, match );
}

const apemodevk::CommandBufferFamilyPool* apemodevk::CommandBufferPool::GetPool( VkQueueFlags queueFlags, bool match ) const {
    return TGetPool< const CommandBufferFamilyPool >( Pools, queueFlags, match );
}

apemodevk::AcquiredCommandBuffer apemodevk::CommandBufferPool::Acquire( bool         bIgnoreFenceStatus,
                                                                        VkQueueFlags eRequiredQueueFlags,
                                                                        bool         bExactMatchByFlags ) {
    apemodevk_memory_allocation_scope;

    /* Try to find something usable. */
    if ( auto pool = GetPool( eRequiredQueueFlags, bExactMatchByFlags ) ) {
        auto acquiredQueue = pool->Acquire( bIgnoreFenceStatus );
        if ( VK_NULL_HANDLE != acquiredQueue.pCmdBuffer )
            return acquiredQueue;
    }

    /* Nothing available for immediate usage */
    return {};
}

apemodevk::AcquiredCommandBuffer apemodevk::CommandBufferPool::Acquire( bool bIgnoreFenceStatus, uint32_t queueFamilyIndex ) {
    /* Try to find something usable. */
    if ( auto pool = GetPool( queueFamilyIndex ) ) {
        auto acquiredQueue = pool->Acquire( bIgnoreFenceStatus );
        if ( VK_NULL_HANDLE != acquiredQueue.pCmdBuffer )
            return acquiredQueue;
    }

    /* Nothing available for immediate usage */
    return {};
}

void apemodevk::CommandBufferPool::Release( const AcquiredCommandBuffer& acquireCmdBuffer ) {
    Pools[ acquireCmdBuffer.QueueFamilyId ].Release( acquireCmdBuffer );
}

bool apemodevk::CommandBufferFamilyPool::Inititalize( GraphicsDevice*                pInNode,
                                                      uint32_t                       InQueueFamilyIndex,
                                                      VkQueueFamilyProperties const& InQueueFamilyProps ) {
    pNode            = pInNode;
    QueueFamilyId    = InQueueFamilyIndex;
    QueueFamilyProps = InQueueFamilyProps;
    return true;
}

void apemodevk::CommandBufferFamilyPool::Destroy( ) {

    if ( pNode ) {
        pNode->Await( );
        for ( auto& cmdBuffer : CmdBuffers ) {
            platform::LogFmt( platform::LogLevel::Info, "Destroying command buffer." );
            cmdBuffer.hCmdBuff.Destroy( );
            cmdBuffer.hCmdPool.Destroy( );
        }
    }
}

apemodevk::CommandBufferFamilyPool::CommandBufferFamilyPool() {
}

apemodevk::CommandBufferFamilyPool::CommandBufferFamilyPool( CommandBufferFamilyPool&& o ) : pNode( o.pNode ) {

    for ( size_t i = 0; i < utils::GetArraySize( CmdBuffers ); ++i ) {
        CmdBuffers[ i ] = eastl::move( o.CmdBuffers[ i ] );
    }
}

apemodevk::CommandBufferFamilyPool& apemodevk::CommandBufferFamilyPool::operator=( CommandBufferFamilyPool&& o ) {

    pNode = o.pNode;
    for ( size_t i = 0; i < utils::GetArraySize( CmdBuffers ); ++i )
        CmdBuffers[ i ] = eastl::move( o.CmdBuffers[ i ] );

    return *this;
}

apemodevk::CommandBufferFamilyPool::~CommandBufferFamilyPool( ) {
    Destroy( );
}

bool InitializeCommandBufferInPool( VkDevice pDevice, uint32_t queueFamilyId, apemodevk::CommandBufferInPool& cmdBuffer ) {
    using namespace apemodevk;
    apemodevk_memory_allocation_scope;

    VkCommandPoolCreateInfo commandPoolCreateInfo;
    InitializeStruct( commandPoolCreateInfo );
    commandPoolCreateInfo.queueFamilyIndex = queueFamilyId;
    commandPoolCreateInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if ( !cmdBuffer.hCmdPool.Recreate( pDevice, commandPoolCreateInfo ) ) {
        return false;
    }

    VkCommandBufferAllocateInfo commandBufferAllocateInfo;
    InitializeStruct( commandBufferAllocateInfo );
    commandBufferAllocateInfo.commandBufferCount = 1;
    commandBufferAllocateInfo.commandPool        = cmdBuffer.hCmdPool;
    commandBufferAllocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    if ( !cmdBuffer.hCmdBuff.Recreate( pDevice, commandBufferAllocateInfo ) ) {
        return false;
    }

    return true;
}

apemodevk::AcquiredCommandBuffer apemodevk::CommandBufferFamilyPool::Acquire( bool bIgnoreFence ) {
    apemodevk_memory_allocation_scope;

    /* Loop through command buffers */

    for ( uint32_t i = 0; i < utils::GetArraySizeU( CmdBuffers ); i++ ) {
        auto& cmdBuffer = CmdBuffers[ i ];

        /* If the command buffer is not used by other thread and it is not being executed, it will be returned */
        if ( false == cmdBuffer.bInUse.exchange( true, std::memory_order_acquire ) ) {
            if ( cmdBuffer.hCmdBuff.IsNull() ) {
                if ( !InitializeCommandBufferInPool( *pNode, QueueFamilyId, cmdBuffer ) || !cmdBuffer.hCmdBuff ) {
                    AcquiredCommandBuffer errorCmdBuffer;
                    errorCmdBuffer.eResult = VK_ERROR_DEVICE_LOST;
                    return errorCmdBuffer;
                }
            }

            /* Fence is not passed when releasing (synchronized).
             * Fence won't be checked.
             * Check if the fence is signaled. */
            if ( !cmdBuffer.pFence || bIgnoreFence ||
                 VK_SUCCESS == CheckedResult( GetFenceStatus( pNode, cmdBuffer.pFence ) ) ) {
                AcquiredCommandBuffer acquiredCommandBuffer;
                acquiredCommandBuffer.QueueFamilyId = QueueFamilyId;
                acquiredCommandBuffer.pCmdBuffer    = cmdBuffer.hCmdBuff;
                acquiredCommandBuffer.pCmdPool      = cmdBuffer.hCmdPool;
                acquiredCommandBuffer.CmdBufferId   = i;

                return acquiredCommandBuffer;
            }

            /* Move back to unused state */
            cmdBuffer.bInUse.exchange( false, std::memory_order_release );
        }
    }

    /* If no free buffer found, null is returned */
    AcquiredCommandBuffer errorCmdBuffer;
    errorCmdBuffer.eResult = VK_NOT_READY;
    return errorCmdBuffer;
}

bool apemodevk::CommandBufferFamilyPool::Release( const AcquiredCommandBuffer& acquiredCmdBuffer ) {
    apemodevk_memory_allocation_scope;

    /* Check if the command buffer was acquired */
    if ( VK_NULL_HANDLE != acquiredCmdBuffer.pCmdBuffer ) {
        auto& cmdBufferInPool  = CmdBuffers[ acquiredCmdBuffer.CmdBufferId ];

        /* Set queue fence */
        cmdBufferInPool.pFence = acquiredCmdBuffer.pFence;

        /* No longer in use */
        const bool previouslyUsed = cmdBufferInPool.bInUse.exchange( false, std::memory_order_release );
        /* Try to track incorrect usage. */
        assert( previouslyUsed );
        return previouslyUsed;
    }

    assert( false );
    return false;
}

//apemodevk::CommandBufferInPool::CommandBufferInPool( CommandBufferInPool&& other )
//    : hCmdBuff( eastl::move( other.hCmdBuff ) )
//    , hCmdPool( eastl::move( other.hCmdPool ) )
//    , pFence( other.pFence )
//    , bInUse( other.bInUse.load( std::memory_order_relaxed ) ) {
//}

namespace {
// TODO: CommandPool interface.
void OnFenceSucceeded( apemodevk::GraphicsDevice* pNode, VkFence pFence ) {
    assert( pNode && pFence );
    apemodevk::CommandBufferPool* pCmdBufferPool = pNode->GetCommandBufferPool( );
    for ( auto& familyPool : pCmdBufferPool->Pools ) {
        for ( auto& cmdBuffer : familyPool.CmdBuffers ) {
            // The command buffer is initialized and submitted with the same fence, and not in use.
            if ( cmdBuffer.hCmdBuff.IsNotNull( ) && ( cmdBuffer.pFence == pFence ) &&
                 ( false == cmdBuffer.bInUse.exchange( true, std::memory_order_acquire ) ) ) {
                cmdBuffer.pFence = nullptr;
                cmdBuffer.bInUse.exchange( false, std::memory_order_release );
            }
        }
    }
}
} // namespace

VkResult apemodevk::GetFenceStatus( GraphicsDevice* pNode, VkFence pFence ) {
    apemodevk_memory_allocation_scope;

    VkResult err;
    switch ( err = CheckedResult( vkGetFenceStatus( pNode->hLogicalDevice, pFence ) ) ) {
        case VK_SUCCESS: {
            OnFenceSucceeded( pNode, pFence );
        } break;
        default:
            break;
    }

    return err;
}

VkResult apemodevk::WaitForFence( GraphicsDevice* pNode, VkFence pFence, uint64_t timeout ) {
    apemodevk_memory_allocation_scope;

    VkResult err;
    switch ( err = CheckedResult( vkGetFenceStatus( pNode->hLogicalDevice, pFence ) ) ) {
        case VK_NOT_READY:
            err = CheckedResult( vkWaitForFences( pNode->hLogicalDevice, 1, &pFence, true, timeout ) );
            // case VK_ERROR_OUT_OF_HOST_MEMORY:
            // case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            // case VK_ERROR_DEVICE_LOST:
        default:
            break;
    }

    if ( VK_SUCCESS == err ) {
        OnFenceSucceeded( pNode, pFence );
    }
    return err;
}

apemodevk::CommandBufferInPool::CommandBufferInPool( ) : pFence( VK_NULL_HANDLE ), bInUse( false ) {
}

apemodevk::CommandBufferInPool::~CommandBufferInPool( ) {
}

apemodevk::CommandBufferInPool::CommandBufferInPool( CommandBufferInPool&& o )
    : hCmdBuff( eastl::move( o.hCmdBuff ) )
    , hCmdPool( eastl::move( o.hCmdPool ) )
    , pFence( o.pFence )
    , bInUse( o.bInUse.load( ) ) {
}

apemodevk::CommandBufferInPool& apemodevk::CommandBufferInPool::operator=( CommandBufferInPool&& o ) {
    hCmdBuff = eastl::move( o.hCmdBuff );
    hCmdPool = eastl::move( o.hCmdPool );
    pFence   = o.pFence;
    bInUse   = o.bInUse.load( );
    return *this;
}
