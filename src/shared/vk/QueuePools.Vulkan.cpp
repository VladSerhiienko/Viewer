// Copyright (c) 2018 Vladyslav Serhiienko <vlad.serhiienko@gmail.com>
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

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
                vkWaitForFences( pDevice, 1, &Queues[ queueIndex ].hFence, true, UINT64_MAX );
                vkDestroyFence( pDevice, Queues[ queueIndex ].hFence, GetAllocationCallbacks( ) );
            }
        }
    }
}

apemodevk::QueueFamilyPool::~QueueFamilyPool( ) {
}

const VkQueueFamilyProperties& apemodevk::QueueFamilyPool::GetQueueFamilyProps( ) const {
    return QueueFamilyProps;
}

bool apemodevk::QueueFamilyPool::SupportsPresenting( VkSurfaceKHR pSurface ) const {
    VkBool32 supported = false;

    switch ( vkGetPhysicalDeviceSurfaceSupportKHR( pPhysicalDevice, QueueFamilyId, pSurface, &supported ) ) {
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

bool apemodevk::QueueFamilyBased::SupportsSparseBinding( ) const {
    return apemodevk::HasFlagEq( QueueFamilyProps.queueFlags, VK_QUEUE_SPARSE_BINDING_BIT );
}

bool apemodevk::QueueFamilyBased::SupportsTransfer( ) const {
    return apemodevk::HasFlagEq( QueueFamilyProps.queueFlags, VK_QUEUE_TRANSFER_BIT );
}

apemodevk::AcquiredQueue apemodevk::QueueFamilyPool::Acquire( bool bIgnoreFence ) {
    apemodevk_memory_allocation_scope;

    uint32_t queueIndex = 0;

    /* Loop through queues */
    for ( auto& queue : Queues ) {

        /* If the queue is not used by the other thread and it is not executing cmd lists, it will be returned */
        /* Note that queue can suspend the execution of the commands, or discard "one time" buffers. */
        if ( false == queue.bInUse.exchange( true, std::memory_order_acquire ) ) {

            /* Get device queue, return DEVICE_LOST if failed. */
            if ( VK_NULL_HANDLE == queue.hQueue ) {
                vkGetDeviceQueue( pDevice, QueueFamilyId, queueIndex, &queue.hQueue );
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

                const VkResult eCreateFenceResult = vkCreateFence( pDevice, &fenceCreateInfo, GetAllocationCallbacks( ), &queue.hFence );
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

            const VkResult eFenceStatus = vkGetFenceStatus( pDevice, queue.hFence );

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
                acquiredQueue.QueueId       = queueIndex;

                return acquiredQueue;
            }

            /* Move back to unused state */
            queue.bInUse.exchange( false, std::memory_order_release );
        }

        ++queueIndex;
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
    Pools[ acquireCmdBuffer.QueueFamilyId ].Release( acquireCmdBuffer );
}

bool apemodevk::CommandBufferFamilyPool::Inititalize( VkDevice                       pInDevice,
                                                      VkPhysicalDevice               pInPhysicalDevice,
                                                      uint32_t                       InQueueFamilyIndex,
                                                      VkQueueFamilyProperties const& InQueueFamilyProps ) {
    QueueFamilyId    = InQueueFamilyIndex;
    QueueFamilyProps = InQueueFamilyProps;
    pDevice          = pInDevice;
    pPhysicalDevice = pInPhysicalDevice;
    return true;
}

void apemodevk::CommandBufferFamilyPool::Destroy( ) {
}

apemodevk::CommandBufferFamilyPool::~CommandBufferFamilyPool( ) {
}

VkResult InitializeCommandBufferInPool( VkDevice pDevice, uint32_t queueFamilyId, apemodevk::CommandBufferInPool& cmdBuffer ) {
    using namespace apemodevk;
    apemodevk_memory_allocation_scope;

    VkResult eResult;

    VkCommandPoolCreateInfo commandPoolCreateInfo;
    InitializeStruct( commandPoolCreateInfo );
    commandPoolCreateInfo.queueFamilyIndex = queueFamilyId;
    commandPoolCreateInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    eResult = vkCreateCommandPool( pDevice, &commandPoolCreateInfo, GetAllocationCallbacks( ), &cmdBuffer.pCmdPool );
    if ( VK_SUCCESS != CheckedResult( eResult ) ) {
        return eResult;
    }

    VkCommandBufferAllocateInfo commandBufferAllocateInfo;
    InitializeStruct( commandBufferAllocateInfo );
    commandBufferAllocateInfo.commandBufferCount = 1;
    commandBufferAllocateInfo.commandPool        = cmdBuffer.pCmdPool;
    commandBufferAllocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    return vkAllocateCommandBuffers( pDevice, &commandBufferAllocateInfo, &cmdBuffer.pCmdBuffer );
}

apemodevk::AcquiredCommandBuffer apemodevk::CommandBufferFamilyPool::Acquire( bool bIgnoreFence ) {
    apemodevk_memory_allocation_scope;

    uint32_t              cmdBufferIndex = 0;
    AcquiredCommandBuffer acquiredCommandBuffer;

    /* Loop through command buffers */
    for ( auto& cmdBuffer : CmdBuffers ) {
        /* If the command buffer is not used by other thread and it is not being executed, it will be returned */
        if ( false == cmdBuffer.bInUse.exchange( true, std::memory_order_acquire ) ) {

            /* Fence is not passed when releasing (synchronized). */
            /* Fence won't be checked. */
            /* Check if the fence is signaled. */
            if ( VK_NULL_HANDLE == cmdBuffer.pFence || bIgnoreFence ||
                 VK_SUCCESS == CheckedResult( vkGetFenceStatus( pDevice, cmdBuffer.pFence ) ) ) {

                /* Check if the buffer is allocated. */
                if ( VK_NULL_HANDLE == cmdBuffer.pCmdBuffer ) {
                    cmdBuffer.bInUse.exchange( false, std::memory_order_release );
                    acquiredCommandBuffer.eResult = VK_ERROR_DEVICE_LOST;
                    return acquiredCommandBuffer;
                }

                acquiredCommandBuffer.pCmdBuffer    = cmdBuffer.pCmdBuffer;
                acquiredCommandBuffer.pCmdPool      = cmdBuffer.pCmdPool;
                acquiredCommandBuffer.CmdBufferId   = cmdBufferIndex;
                acquiredCommandBuffer.QueueFamilyId = QueueFamilyId;

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

    /* Allocate buffer. */
    acquiredCommandBuffer.eResult = InitializeCommandBufferInPool( pDevice, QueueFamilyId, cmdBuffer );
    if ( VK_SUCCESS != acquiredCommandBuffer.eResult ) {
        return acquiredCommandBuffer;
    }

    CmdBuffers.emplace_back( cmdBuffer );

    acquiredCommandBuffer.QueueFamilyId = QueueFamilyId;
    acquiredCommandBuffer.pCmdBuffer    = cmdBuffer.pCmdBuffer;
    acquiredCommandBuffer.pCmdPool      = cmdBuffer.pCmdPool;
    acquiredCommandBuffer.CmdBufferId   = cmdBufferIndex;

    return acquiredCommandBuffer;
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
    switch ( err = CheckedResult( vkGetFenceStatus( pDevice, pFence ) ) ) {
        case VK_NOT_READY:
             err = CheckedResult( vkWaitForFences( pDevice, 1, &pFence, true, timeout ) );
            // case VK_ERROR_OUT_OF_HOST_MEMORY:
            // case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            // case VK_ERROR_DEVICE_LOST:
        default:
            break;
    }

    return err;
}