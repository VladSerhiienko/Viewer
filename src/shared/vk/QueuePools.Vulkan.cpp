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
        if ( pool.queueFamilyProps.queueFlags == queueFlags )
            return &pool;

    /* Try to find something usable. */
    if ( false == match )
        for ( auto& pool : Pools )
            if ( queueFlags == ( pool.queueFamilyProps.queueFlags & queueFlags ) )
                return &pool;

    /* Nothing available for immediate usage */
    return nullptr;
}

bool apemodevk::QueuePool::Inititalize( VkDevice                       pInDevice,
                                        VkPhysicalDevice               pInPhysicalDevice,
                                        const VkQueueFamilyProperties* pQueuePropsIt,
                                        const VkQueueFamilyProperties* pQueuePropsEnd,
                                        const float*                   pQueuePrioritiesIt,
                                        const float*                   pQueuePrioritiesItEnd ) {
    apemodevk_memory_allocation_scope;

    pDevice = pInDevice;
    pPhysicalDevice = pInPhysicalDevice;

    (void) pQueuePrioritiesIt;
    (void) pQueuePrioritiesItEnd;

    Pools.reserve( std::distance( pQueuePropsIt, pQueuePropsEnd ) );

    uint32_t familyIndex = 0;
    std::transform( pQueuePropsIt, pQueuePropsEnd, std::back_inserter( Pools ), [&]( const VkQueueFamilyProperties& InProps ) {
        QueueFamilyPool pool;
        pool.Inititalize( pInDevice, pInPhysicalDevice, familyIndex++, InProps );
        return pool;
    } );

    return true;
}

void apemodevk::QueuePool::Destroy( ) {
}

apemodevk::QueuePool::~QueuePool( ) {
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

bool apemodevk::QueueFamilyPool::Inititalize( VkDevice                       pInDevice,
                                              VkPhysicalDevice               pInPhysicalDevice,
                                              uint32_t                       InQueueFamilyIndex,
                                              VkQueueFamilyProperties const& InQueueFamilyProps ) {
    apemodevk_memory_allocation_scope;

    pDevice          = pInDevice;
    pPhysicalDevice  = pInPhysicalDevice;
    queueFamilyId    = InQueueFamilyIndex;
    queueFamilyProps = InQueueFamilyProps;

    Queues.resize( InQueueFamilyProps.queueCount );
    return true;
}

void apemodevk::QueueFamilyPool::Destroy( ) {
    apemodevk_memory_allocation_scope;

    if ( false == Queues.empty( ) ) {
        uint32_t queueIndex = queueFamilyProps.queueCount;
        while ( queueIndex-- ) {
            if ( VK_NULL_HANDLE != Queues[ queueIndex ].hFence ) {
                vkWaitForFences( pDevice, 1, &Queues[ queueIndex ].hFence, true, UINT64_MAX );
                vkDestroyFence( pDevice, Queues[ queueIndex ].hFence, GetAllocationCallbacks( ) );
            }
        }
    }
}

apemodevk::QueueFamilyPool::~QueueFamilyPool( ) {
}

const VkQueueFamilyProperties& apemodevk::QueueFamilyPool::GetQueueFamilyProps( ) const {
    return queueFamilyProps;
}

bool apemodevk::QueueFamilyPool::SupportsPresenting( VkSurfaceKHR pSurface ) const {
    VkBool32 supported = false;

    switch ( vkGetPhysicalDeviceSurfaceSupportKHR( pPhysicalDevice, queueFamilyId, pSurface, &supported ) ) {
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
    : queueFamilyId( queueFamilyId ), queueFamilyProps( queueFamilyProperties ) {

    /* From the docs: VK_QUEUE_TRANSFER_BIT is enabled if either GRAPHICS or COMPUTE or both are enabled. */
    /* Since we search queues / command buffers according to those flags, we set them here. */
    if ( ( queueFamilyProps.queueFlags & VK_QUEUE_GRAPHICS_BIT ) == VK_QUEUE_GRAPHICS_BIT ||
         ( queueFamilyProps.queueFlags & VK_QUEUE_COMPUTE_BIT ) == VK_QUEUE_COMPUTE_BIT ) {

        queueFamilyProps.queueFlags |= VK_QUEUE_TRANSFER_BIT;
    }
}

bool apemodevk::QueueFamilyBased::SupportsGraphics( ) const {
    return apemodevk::HasFlagEq( queueFamilyProps.queueFlags, VK_QUEUE_GRAPHICS_BIT );
}

bool apemodevk::QueueFamilyBased::SupportsCompute( ) const {
    return apemodevk::HasFlagEq( queueFamilyProps.queueFlags, VK_QUEUE_COMPUTE_BIT );
}

bool apemodevk::QueueFamilyBased::SupportsSparseBinding( ) const {
    return apemodevk::HasFlagEq( queueFamilyProps.queueFlags, VK_QUEUE_SPARSE_BINDING_BIT );
}

bool apemodevk::QueueFamilyBased::SupportsTransfer( ) const {
    return apemodevk::HasFlagEq( queueFamilyProps.queueFlags, VK_QUEUE_TRANSFER_BIT );
}

apemodevk::AcquiredQueue apemodevk::QueueFamilyPool::Acquire( bool bIgnoreFence ) {
    apemodevk_memory_allocation_scope;

    uint32_t queueIndex = 0;

    /* Loop through queues */
    for (auto& queue : Queues) {

        /* If the queue is not used by other thread and it is not executing cmd lists, it will be returned */
        /* Note that queue can suspend the execution of the commands, or discard "one time" buffers. */
        if ( false == queue.bInUse.exchange( true, std::memory_order_acquire ) ) {
            if ( bIgnoreFence || VK_NULL_HANDLE == queue.hQueue || VK_SUCCESS == vkGetFenceStatus( pDevice, queue.hFence ) ) {
                if ( VK_NULL_HANDLE == queue.hQueue ) {
                    VkFenceCreateInfo fenceCreateInfo;
                    InitializeStruct( fenceCreateInfo );

                    /* Since the queue is free to use after creation. */
                    /* @note Must be reset before usage. */
                    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

                    vkGetDeviceQueue( pDevice, queueFamilyId, queueIndex, &queue.hQueue );
                    vkCreateFence( pDevice, &fenceCreateInfo, GetAllocationCallbacks( ), &queue.hFence );

                    assert( queue.hQueue );
                    assert( queue.hFence );
                }

                AcquiredQueue acquiredQueue;
                acquiredQueue.pQueue        = queue.hQueue;
                acquiredQueue.pFence        = queue.hFence;
                acquiredQueue.QueueFamilyId = queueFamilyId;
                acquiredQueue.QueueId       = queueIndex;
                return acquiredQueue;
            }

            /* Move back to unused state */
            queue.bInUse.exchange( false, std::memory_order_release );
        }

        ++queueIndex;
    }

    /* If no free queue found, null is returned */
    return {};
}

bool apemodevk::QueueFamilyPool::Release( const apemodevk::AcquiredQueue& acquiredQueue ) {
    apemodevk_memory_allocation_scope;

    /* Check if the queue was acquired */
    if ( VK_NULL_HANDLE != acquiredQueue.pQueue ) {

        /* No longer used, ok */
        const bool previouslyUsed = Queues[ acquiredQueue.QueueId ].bInUse.exchange( false, std::memory_order_release );

        /* Try to track incorrect usage or atomic mess. */
        if ( false == previouslyUsed )
            apemodevk::platform::DebugBreak( );

        return true;
    }

    apemodevk::platform::DebugBreak( );
    return false;
}

apemodevk::QueueInPool::QueueInPool( )
    : bInUse( false ) {
}

apemodevk::QueueInPool::QueueInPool( const QueueInPool& other )
    : hQueue( other.hQueue )
    , hFence( other.hFence )
    , bInUse( other.bInUse.load( std::memory_order_relaxed ) ) {
}

bool apemodevk::CommandBufferPool::Inititalize( VkDevice                       pInDevice,
                                                VkPhysicalDevice               pInPhysicalDevice,
                                                const VkQueueFamilyProperties* pQueuePropsIt,
                                                const VkQueueFamilyProperties* pQueuePropsEnd ) {
    apemodevk_memory_allocation_scope;

    pDevice         = pInDevice;
    pPhysicalDevice = pInPhysicalDevice;

    Pools.reserve( std::distance( pQueuePropsIt, pQueuePropsEnd ) );

    uint32_t familyIndex = 0;
    std::transform( pQueuePropsIt, pQueuePropsEnd, std::back_inserter( Pools ), [&]( const VkQueueFamilyProperties& InProps ) {
        CommandBufferFamilyPool pool;
        pool.Inititalize( pInDevice, pInPhysicalDevice, familyIndex++, InProps );
        return pool;
    } );

    return true;
}

void apemodevk::CommandBufferPool::Destroy( ) {
}

apemodevk::CommandBufferPool::~CommandBufferPool( ) {
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
    Pools[ acquireCmdBuffer.queueFamilyId ].Release( acquireCmdBuffer );
}

bool apemodevk::CommandBufferFamilyPool::Inititalize( VkDevice                       pInDevice,
                                                      VkPhysicalDevice               pInPhysicalDevice,
                                                      uint32_t                       InQueueFamilyIndex,
                                                      VkQueueFamilyProperties const& InQueueFamilyProps ) {
    queueFamilyId    = InQueueFamilyIndex;
    queueFamilyProps = InQueueFamilyProps;
    pDevice          = pInDevice;
    pPhysicalDevice = pInPhysicalDevice;
    return true;
}

void apemodevk::CommandBufferFamilyPool::Destroy( ) {
}

apemodevk::CommandBufferFamilyPool::~CommandBufferFamilyPool( ) {
}

void InitializeCommandBufferInPool( VkDevice pDevice, uint32_t queueFamilyId, apemodevk::CommandBufferInPool& cmdBuffer ) {
    using namespace apemodevk;
    apemodevk_memory_allocation_scope;

    VkCommandPoolCreateInfo commandPoolCreateInfo;
    InitializeStruct( commandPoolCreateInfo );
    commandPoolCreateInfo.queueFamilyIndex = queueFamilyId;
    commandPoolCreateInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    vkCreateCommandPool( pDevice, &commandPoolCreateInfo, GetAllocationCallbacks( ), &cmdBuffer.pCmdPool );
    assert( cmdBuffer.pCmdPool );

    VkCommandBufferAllocateInfo commandBufferAllocateInfo;
    InitializeStruct( commandBufferAllocateInfo );
    commandBufferAllocateInfo.commandBufferCount = 1;
    commandBufferAllocateInfo.commandPool        = cmdBuffer.pCmdPool;
    commandBufferAllocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    vkAllocateCommandBuffers( pDevice, &commandBufferAllocateInfo, &cmdBuffer.pCmdBuffer );
    assert( cmdBuffer.pCmdBuffer );
}

apemodevk::AcquiredCommandBuffer apemodevk::CommandBufferFamilyPool::Acquire( bool bIgnoreFence ) {
    apemodevk_memory_allocation_scope;

    uint32_t              cmdBufferIndex = 0;
    AcquiredCommandBuffer acquiredCommandBuffer;

    /* Loop through command buffers */
    for (auto& cmdBuffer : CmdBuffers) {
        /* If the command buffer is not used by other thread and it is not being executed, it will be returned */
        if ( false == cmdBuffer.bInUse.exchange( true, std::memory_order_acquire ) ) {
            if ( bIgnoreFence ||                       /* Fence won't be checked */
                 VK_NULL_HANDLE == cmdBuffer.pFence || /* Fence is not passed when releasing (synchronized) */
                 VK_SUCCESS == vkGetFenceStatus( pDevice, cmdBuffer.pFence ) ) {
                if ( VK_NULL_HANDLE == cmdBuffer.pCmdBuffer ) {
                    InitializeCommandBufferInPool( pDevice, queueFamilyId, cmdBuffer );
                }

                acquiredCommandBuffer.pCmdBuffer    = cmdBuffer.pCmdBuffer;
                acquiredCommandBuffer.pCmdPool      = cmdBuffer.pCmdPool;
                acquiredCommandBuffer.cmdBufferId   = cmdBufferIndex;
                acquiredCommandBuffer.queueFamilyId = queueFamilyId;

                return acquiredCommandBuffer;
            }

            /* Move back to unused state */
            cmdBuffer.bInUse.exchange( false, std::memory_order_release );
        }

        ++cmdBufferIndex;
    }

    /* Create new command buffer */

    CommandBufferInPool cmdBuffer;
    cmdBuffer.bInUse.exchange( true, std::memory_order_relaxed );
    InitializeCommandBufferInPool( pDevice, queueFamilyId, cmdBuffer );
    CmdBuffers.emplace_back( cmdBuffer );

    acquiredCommandBuffer.queueFamilyId = queueFamilyId;
    acquiredCommandBuffer.pCmdBuffer    = cmdBuffer.pCmdBuffer;
    acquiredCommandBuffer.pCmdPool      = cmdBuffer.pCmdPool;
    acquiredCommandBuffer.cmdBufferId   = cmdBufferIndex;

    return acquiredCommandBuffer;
}

bool apemodevk::CommandBufferFamilyPool::Release( const AcquiredCommandBuffer& acquiredCmdBuffer ) {
    apemodevk_memory_allocation_scope;

    /* Check if the command buffer was acquired */
    if ( VK_NULL_HANDLE != acquiredCmdBuffer.pCmdBuffer ) {
        auto& cmdBufferInPool  = CmdBuffers[ acquiredCmdBuffer.cmdBufferId ];

        /* Set queue fence */
        cmdBufferInPool.pFence = acquiredCmdBuffer.pFence;

        /* No longer in use */
        const bool previouslyUsed = cmdBufferInPool.bInUse.exchange( false, std::memory_order_release );

        /* Try to track incorrect usage or atomic mess. */
        if ( false == previouslyUsed )
            apemodevk::platform::DebugBreak( );

        return true;
    }

    apemodevk::platform::DebugBreak( );
    return false;
}

apemodevk::CommandBufferInPool::CommandBufferInPool( )
    : bInUse( false ) {
}

apemodevk::CommandBufferInPool::CommandBufferInPool( const CommandBufferInPool& other )
    : pCmdBuffer( other.pCmdBuffer )
    , pCmdPool( other.pCmdPool )
    , pFence( other.pFence )
    , bInUse( other.bInUse.load( std::memory_order_relaxed ) ) {
}

VkResult apemodevk::WaitForFence( VkDevice pDevice, VkFence pFence, uint64_t timeout ) {
    apemodevk_memory_allocation_scope;

    VkResult err;
    switch ( err = CheckedCall( vkGetFenceStatus( pDevice, pFence ) ) ) {
        case VK_NOT_READY:
             err = CheckedCall( vkWaitForFences( pDevice, 1, &pFence, true, timeout ) );
            // case VK_ERROR_OUT_OF_HOST_MEMORY:
            // case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            // case VK_ERROR_DEVICE_LOST:
        default:
            break;
    }

    return err;
}