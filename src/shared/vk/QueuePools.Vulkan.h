#pragma once

#include <GraphicsManager.Vulkan.h>

#include <atomic>

namespace apemodevk {

    class GraphicsDevice;

    class CommandBufferPool;
    class CommandBufferFamilyPool;

    static const VkQueueFlags sQueueAllBits
        = VK_QUEUE_COMPUTE_BIT
        | VK_QUEUE_GRAPHICS_BIT
        | VK_QUEUE_TRANSFER_BIT;

    /* Basic properties for the objects that depend on queue family id */
    struct QueueFamilyBased {
        /* Queue properties: */

        uint32_t                QueueFamilyId    = VK_QUEUE_FAMILY_IGNORED;
        VkQueueFamilyProperties QueueFamilyProps = {};

        QueueFamilyBased( ) = default;
        QueueFamilyBased( uint32_t queueFamilyId, VkQueueFamilyProperties queueFamilyProps );

        /* Queue features: */

        bool SupportsSparseBinding( ) const;
        bool SupportsTransfer( ) const;
        bool SupportsGraphics( ) const;
        bool SupportsCompute( ) const;

        /* NOTE: Surface support can be checked with QueueFamilyPool. */
    };

    /**
     * Currently command buffer "in pool" can be submitted to only one queue at a time (no concurrent executions).
     * Each command pool has only one associated buffer (so external no synchoronization needed).
     * Ensure to release the allocated buffer for reusing.
     **/
    struct CommandBufferInPool {
        VkCommandBuffer  pCmdBuffer = VK_NULL_HANDLE; /* Handle */
        VkCommandPool    pCmdPool   = VK_NULL_HANDLE; /* Command pool handle (associated with the Handle) */
        VkFence          pFence     = VK_NULL_HANDLE; /* Last queue fence */
        std::atomic_bool bInUse;                      /* Indicates it is used by other thread */

        CommandBufferInPool( );
        CommandBufferInPool( const CommandBufferInPool& other );
    };

    struct AcquiredCommandBuffer {
        VkCommandBuffer pCmdBuffer    = VK_NULL_HANDLE; /* Handle */
        VkCommandPool   pCmdPool      = VK_NULL_HANDLE; /* Command pool handle (associated with the Handle) */
        VkFence         pFence        = VK_NULL_HANDLE; /* Last queue fence */
        uint32_t        QueueFamilyId = VK_QUEUE_FAMILY_IGNORED;
        uint32_t        CmdBufferId   = VK_QUEUE_FAMILY_IGNORED;
    };

    class CommandBufferFamilyPool : public QueueFamilyBased {
        friend class CommandBufferPool;
    public:

        VkDevice                           pDevice         = VK_NULL_HANDLE;
        VkPhysicalDevice                   pPhysicalDevice = VK_NULL_HANDLE;
        std::vector< CommandBufferInPool > CmdBuffers;

        CommandBufferFamilyPool( )                                 = default;
        CommandBufferFamilyPool( CommandBufferFamilyPool&& other ) = default;
        ~CommandBufferFamilyPool( );

        bool Inititalize( VkDevice                       pInDevice,
                          VkPhysicalDevice               pInPhysicalDevice,
                          uint32_t                       InQueueFamilyIndex,
                          VkQueueFamilyProperties const& InQueueFamilyProps );

        void Destroy( );

        AcquiredCommandBuffer Acquire( bool bIgnoreFenceStatus );
        bool                  Release( const AcquiredCommandBuffer& acquireCmdBuffer );
    };

    class CommandBufferPool {
    public:
        VkDevice                               pDevice         = VK_NULL_HANDLE;
        VkPhysicalDevice                       pPhysicalDevice = VK_NULL_HANDLE;
        uint32_t                               QueueFamilyId   = 0;
        std::vector< CommandBufferFamilyPool > Pools;

        CommandBufferPool( ) = default;
        ~CommandBufferPool( );

        bool Inititalize( VkDevice                       pDevice,
                          VkPhysicalDevice               pPhysicalDevice,
                          const VkQueueFamilyProperties* pQueuePropsIt,
                          const VkQueueFamilyProperties* pQueuePropsEnd );

        void Destroy( );

        uint32_t                       GetPoolCount( ) const;
        CommandBufferFamilyPool*       GetPool( uint32_t queueFamilyIndex );
        const CommandBufferFamilyPool* GetPool( uint32_t queueFamilyIndex ) const;
        CommandBufferFamilyPool*       GetPool( VkQueueFlags eRequiredQueueFlags, bool bExactMatchByFlags );
        const CommandBufferFamilyPool* GetPool( VkQueueFlags eRequiredQueueFlags, bool bExactMatchByFlags ) const;

        /**
         * @param bIgnoreFenceStatus If any command buffer submitted to this queue is in the executable state,
         *                           it is moved to the pending state. Note, that VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
         *                           command buffers will be invalidated (unless explicit vkWaitForFences).
         * @param eRequiredQueueFlags Flags that must be enabled (try to pick only the needed bits).
         * @param bExactMatchByFlags Only the flags in eRequiredQueueFlags must be present (for copy queues is important).
         * @return Unused command buffer, or null if none was found.
         * @note Release for reusing, @see Release().
         **/
        AcquiredCommandBuffer Acquire( bool bIgnoreFenceStatus, VkQueueFlags eRequiredQueueFlags, bool bExactMatchByFlags );
        AcquiredCommandBuffer Acquire( bool bIgnoreFenceStatus, uint32_t queueFamilyIndex );

        /**
         * Allows the command buffer to be reused.
         **/
        void Release( const AcquiredCommandBuffer& acquireCmdBuffer );
    };

    class QueuePool;
    class QueueFamilyPool;

    struct QueueInPool {
        VkQueue          hQueue = VK_NULL_HANDLE; /* Handle */
        VkFence          hFence = VK_NULL_HANDLE; /* Indicates that the execution is in progress */
        std::atomic_bool bInUse;                  /* Indicates that it is used by other thread */

        QueueInPool( );
        QueueInPool( const QueueInPool& other );
    };

    struct AcquiredQueue {
        VkQueue  pQueue        = VK_NULL_HANDLE; /* Free to use queue. */
        VkFence  pFence        = VK_NULL_HANDLE; /* Acquire() can optionally ignore fence status, so the user can await. */
        uint32_t QueueFamilyId = VK_QUEUE_FAMILY_IGNORED;              /* Queue family index */
        uint32_t QueueId       = VK_QUEUE_FAMILY_IGNORED;              /* Queue index in family pool */
    };

    class QueueFamilyPool : public QueueFamilyBased {
    public:
        VkDevice                   pDevice         = VK_NULL_HANDLE;
        VkPhysicalDevice           pPhysicalDevice = VK_NULL_HANDLE;
        std::vector< QueueInPool > Queues;

        QueueFamilyPool( )                              = default;
        QueueFamilyPool( QueueFamilyPool&& other )      = default;
        QueueFamilyPool( const QueueFamilyPool& other ) = default;
        ~QueueFamilyPool( );

        bool Inititalize( VkDevice                       pInDevice,
                          VkPhysicalDevice               pInPhysicalDevice,
                          uint32_t                       InQueueFamilyIndex,
                          VkQueueFamilyProperties const& InQueueFamilyProps );

        void Destroy( );

        const VkQueueFamilyProperties& GetQueueFamilyProps( ) const;
        bool SupportsPresenting( VkSurfaceKHR pSurface ) const;

        /**
         * @param bIgnoreFenceStatus If any command buffer submitted to this queue is in the executable state,
         *                           it is moved to the pending state. Note, that VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
         *                           command buffers will be invalidated (unless explicit vkWaitForFences).
         * @return Unused queue, or null if none was found.
         * @note Release for reusing, @see Release().
         */
        AcquiredQueue Acquire( bool bIgnoreFenceStatus );

        /**
         * Allows the queue to be reused.
         */
        bool Release( const AcquiredQueue& pUnneededQueue );
    };

    /**
     * QueuePool is created by device.
     */
    class QueuePool {
    public:
        VkDevice                       pDevice         = VK_NULL_HANDLE;
        VkPhysicalDevice               pPhysicalDevice = VK_NULL_HANDLE;
        std::vector< QueueFamilyPool > Pools;

        QueuePool( ) = default;
        ~QueuePool( );

        bool Inititalize( VkDevice                       pInDevice,
                          VkPhysicalDevice               pInPhysicalDevice,
                          const VkQueueFamilyProperties* pQueuePropsIt,
                          const VkQueueFamilyProperties* pQueuePropsEnd,
                          const float*                   pQueuePrioritiesIt,
                          const float*                   pQueuePrioritiesItEnd );

        void Destroy( );

        uint32_t               GetPoolCount( ) const;                      /* @note Returns the number of queue families */
        QueueFamilyPool*       GetPool( uint32_t QueueFamilyIndex );       /* @see GetPool( VkQueueFlags , bool ) */
        const QueueFamilyPool* GetPool( uint32_t QueueFamilyIndex ) const; /* @see GetPool( VkQueueFlags , bool ) */
        QueueFamilyPool*       GetPool( VkQueueFlags eRequiredQueueFlags, bool bExactMatchByFlags );
        const QueueFamilyPool* GetPool( VkQueueFlags eRequiredQueueFlags, bool bExactMatchByFlags ) const;

        /**
         * @param bIgnoreFenceStatus If any command buffer submitted to this queue is in the executable state,
         *                           it is moved to the pending state. Note, that VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
         *                           command buffers will be invalidated (unless explicit vkWaitForFences).
         * @param RequiredQueueFlags Flags that must be available enabled (try to pick only the needed bits).
         * @param bExactMatchByFlags Only the flags in RequiredQueueFlags must be present (for copy queues is important).
         * @return Unused queue, or null if none was found.
         * @note Release for reusing, @see Release().
         * @see GetPool().
         */
        AcquiredQueue Acquire( bool bIgnoreFenceStatus, VkQueueFlags eRequiredQueueFlags, bool bExactMatchByFlags );
        AcquiredQueue Acquire( bool bIgnoreFenceStatus, uint32_t QueueFamilyIndex );

        /**
         * Allows the queue to be reused.
         **/
        void Release( const AcquiredQueue& acquiredQueue );
    };

    /**
     * Checks the stage of fence
     * Returns VK_SUCCESS if it is signaled.
     * Waits for fence with the provided timeout otherwise.
     **/
    VkResult WaitForFence( VkDevice pDevice, VkFence pFence, uint64_t timeout = ~0ULL );

    struct OneTimeCmdBufferSubmitResult {
        bool     bAcquiredQueue         = true;
        VkResult eResetCmdPoolResult    = VK_SUCCESS;
        VkResult eBeginCmdBuffer        = VK_SUCCESS;
        VkResult eEndCmdBuffer          = VK_SUCCESS;
        VkResult eWaitForFence          = VK_SUCCESS;
        VkResult eResetFence            = VK_SUCCESS;
        VkResult eQueueSubmit           = VK_SUCCESS;
        VkResult eWaitForSubmittedFence = VK_SUCCESS;
        uint32_t QueueFamilyId          = VK_QUEUE_FAMILY_IGNORED;
        uint32_t QueueId                = VK_QUEUE_FAMILY_IGNORED;
    };

    inline bool Succeeded( const OneTimeCmdBufferSubmitResult& result ) {
        return result.bAcquiredQueue && ( result.eResetCmdPoolResult == VK_SUCCESS ) &&
               ( result.eBeginCmdBuffer == VK_SUCCESS ) && ( result.eEndCmdBuffer == VK_SUCCESS ) &&
               ( result.eWaitForFence == VK_SUCCESS ) && ( result.eQueueSubmit == VK_SUCCESS ) &&
               ( ( result.eWaitForSubmittedFence == VK_SUCCESS ) || ( ( result.QueueId != VK_QUEUE_FAMILY_IGNORED ) && ( result.QueueFamilyId != VK_QUEUE_FAMILY_IGNORED ) ) );
    }

    template < typename TPopulateCmdBufferFn >
    OneTimeCmdBufferSubmitResult TOneTimeCmdBufferSubmit( GraphicsDevice*      pNode,
                                                          uint32_t             queueFamilyId,
                                                          bool                 pAwaitQueue,
                                                          TPopulateCmdBufferFn populateCmdBufferFn,
                                                          const VkSemaphore*   pSignalSemaphores    = nullptr,
                                                          uint32_t             signalSemaphoreCount = 0,
                                                          const VkSemaphore*   pWaitSemaphores      = nullptr,
                                                          uint32_t             waitSemaphoreCount   = 0 ) {
        OneTimeCmdBufferSubmitResult result{};

        /* Get command buffer from pool */
        auto pCmdBufferPool = pNode->GetCommandBufferPool( );
        auto acquiredCmdBuffer = pCmdBufferPool->Acquire( false, queueFamilyId );

        /* Reset command pool */

        result.eResetCmdPoolResult = vkResetCommandPool( pNode->hLogicalDevice, acquiredCmdBuffer.pCmdPool, 0 );
        if ( VK_SUCCESS != CheckedCall( result.eResetCmdPoolResult ) ) {
            // VK_ERROR_OUT_OF_HOST_MEMORY
            // VK_ERROR_OUT_OF_DEVICE_MEMORY
            return result;
        }

        /* Begin recording commands to command buffer */
        VkCommandBufferBeginInfo commandBufferBeginInfo;
        InitializeStruct( commandBufferBeginInfo );
        commandBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        result.eBeginCmdBuffer = vkBeginCommandBuffer( acquiredCmdBuffer.pCmdBuffer, &commandBufferBeginInfo );
        if ( VK_SUCCESS != CheckedCall( result.eBeginCmdBuffer ) ) {
            // VK_ERROR_OUT_OF_HOST_MEMORY
            // VK_ERROR_OUT_OF_DEVICE_MEMORY
            return result;
        }

        const bool bPopulated = populateCmdBufferFn( acquiredCmdBuffer.pCmdBuffer );

        result.eEndCmdBuffer = vkEndCommandBuffer( acquiredCmdBuffer.pCmdBuffer );
        if ( VK_SUCCESS != CheckedCall( result.eEndCmdBuffer ) ) {
            // VK_ERROR_OUT_OF_HOST_MEMORY
            // VK_ERROR_OUT_OF_DEVICE_MEMORY
            return result;
        }

        if ( bPopulated ) {

            /* Get queue from pool */
            auto pQueuePool = pNode->GetQueuePool( );
            assert( pQueuePool );

            auto pQueueFamilyPool = pQueuePool->GetPool( queueFamilyId );
            assert( pQueueFamilyPool );

            AcquiredQueue acquiredQueue = pQueueFamilyPool->Acquire( false );
            while ( acquiredQueue.pQueue == nullptr ) {
                acquiredQueue = pQueueFamilyPool->Acquire( true );
            }

            assert( acquiredQueue.pQueue );
            assert( acquiredQueue.pFence );

            result.eWaitForFence = WaitForFence( pNode->hLogicalDevice, acquiredQueue.pFence );
            if ( VK_SUCCESS != result.eWaitForFence ) {
                // case VK_ERROR_OUT_OF_HOST_MEMORY:
                // case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                // case VK_ERROR_DEVICE_LOST:
                return result;
            }

            VkSubmitInfo submitInfo;
            InitializeStruct( submitInfo );

            submitInfo.pSignalSemaphores  = pSignalSemaphores;
            submitInfo.pWaitSemaphores    = pWaitSemaphores;
            submitInfo.waitSemaphoreCount = waitSemaphoreCount;
            submitInfo.pCommandBuffers    = &acquiredCmdBuffer.pCmdBuffer;
            submitInfo.commandBufferCount = 1;

            result.eResetFence = vkResetFences( pNode->hLogicalDevice, 1, &acquiredQueue.pFence );
            if ( VK_SUCCESS != CheckedCall( result.eResetFence ) ) {
                // VK_ERROR_OUT_OF_HOST_MEMORY
                // VK_ERROR_OUT_OF_DEVICE_MEMORY
                return result;
            }

            result.eQueueSubmit = vkQueueSubmit( acquiredQueue.pQueue, 1, &submitInfo, acquiredQueue.pFence );
            if ( VK_SUCCESS != CheckedCall( result.eQueueSubmit ) ) {
                // VK_ERROR_OUT_OF_HOST_MEMORY
                // VK_ERROR_OUT_OF_DEVICE_MEMORY
                // VK_ERROR_DEVICE_LOST
                return result;
            }

            if ( pAwaitQueue ) {
                result.eWaitForSubmittedFence = WaitForFence( pNode->hLogicalDevice, acquiredQueue.pFence );
                if ( VK_SUCCESS != result.eWaitForSubmittedFence ) {
                    // case VK_ERROR_OUT_OF_HOST_MEMORY:
                    // case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    // case VK_ERROR_DEVICE_LOST:
                    return result;
                }

                result.QueueId       = acquiredQueue.QueueId;
                result.QueueFamilyId = acquiredQueue.QueueFamilyId;
            }

            acquiredCmdBuffer.pFence = acquiredQueue.pFence;
            pQueuePool->Release( acquiredQueue );
        }

        pCmdBufferPool->Release( acquiredCmdBuffer );
        return result;
    }

} // namespace apemodevk
