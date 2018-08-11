// Copyright (c) 2018 Vladyslav Serhiienko <vlad.serhiienko@gmail.com>
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

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

        uint32_t                QueueFamilyId    = kInvalidIndex;
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

    struct AcquiredCommandBuffer {
        VkCommandBuffer pCmdBuffer    = VK_NULL_HANDLE; /* Handle */
        VkCommandPool   pCmdPool      = VK_NULL_HANDLE; /* Command pool handle (associated with the Handle) */
        VkFence         pFence        = VK_NULL_HANDLE; /* Last queue fence */
        uint32_t        QueueFamilyId = kInvalidIndex;  /* Queue family index */
        uint32_t        CmdBufferId   = kInvalidIndex;  /* Queue index in family pool */
        VkResult        eResult       = VK_SUCCESS;     /* The error code that occurred during the queue acquisition */
    };

    /**
     * Currently command buffer "in pool" can be submitted to only one queue at a time (no concurrent executions).
     * Each command pool has only one associated buffer (so external no synchoronization needed).
     * Ensure to release the allocated buffer for reusing.
     **/
    struct CommandBufferInPool {
        THandle< VkCommandBuffer > hCmdBuff; /* Handle */
        THandle< VkCommandPool >   hCmdPool; /* Command pool handle (associated with the Handle) */
        VkFence                    pFence;   /* Last queue fence */
        std::atomic_bool           bInUse;   /* Indicates it is used by other thread */

        CommandBufferInPool( );
        CommandBufferInPool( CommandBufferInPool&& o );
        CommandBufferInPool& operator=( CommandBufferInPool&& o );
    };

    class CommandBufferFamilyPool : public QueueFamilyBased {
        friend class CommandBufferPool;

    public:
        VkDevice            pDevice         = VK_NULL_HANDLE;
        VkPhysicalDevice    pPhysicalDevice = VK_NULL_HANDLE;
        CommandBufferInPool CmdBuffers[ 32 ];

        CommandBufferFamilyPool( );
        CommandBufferFamilyPool( CommandBufferFamilyPool&& o );
        CommandBufferFamilyPool& operator=( CommandBufferFamilyPool&& o );
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
        VkDevice                          pDevice         = VK_NULL_HANDLE;
        VkPhysicalDevice                  pPhysicalDevice = VK_NULL_HANDLE;
        uint32_t                          QueueFamilyId   = 0;
        vector< CommandBufferFamilyPool > Pools;

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
        VkQueue  pQueue        = VK_NULL_HANDLE;          /* Free to use queue. */
        VkFence  pFence        = VK_NULL_HANDLE;          /* Acquire() can optionally ignore fence status. */
        uint32_t QueueFamilyId = kInvalidIndex;           /* Queue family index */
        uint32_t QueueId       = kInvalidIndex;           /* Queue index in family pool */
        VkResult eResult       = VK_SUCCESS;              /* The error code that occurred during the queue acquisition */
    };

    class QueueFamilyPool : public QueueFamilyBased {
    public:
        VkDevice              pDevice         = VK_NULL_HANDLE;
        VkPhysicalDevice      pPhysicalDevice = VK_NULL_HANDLE;
        vector< QueueInPool > Queues;

        QueueFamilyPool( )                              = default;
        QueueFamilyPool( QueueFamilyPool&& other )      = default;
        QueueFamilyPool( const QueueFamilyPool& other ) = default;
        ~QueueFamilyPool( );

        bool Inititalize( VkDevice                       pInDevice,
                          VkPhysicalDevice               pInPhysicalDevice,
                          uint32_t                       queueFamilyIndex,
                          VkQueueFamilyProperties const& queueFamilyProps );

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
        VkDevice                  pDevice         = VK_NULL_HANDLE;
        VkPhysicalDevice          pPhysicalDevice = VK_NULL_HANDLE;
        vector< QueueFamilyPool > Pools;

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
        QueueFamilyPool*       GetPool( uint32_t queueFamilyIndex );       /* @see GetPool( VkQueueFlags , bool ) */
        const QueueFamilyPool* GetPool( uint32_t queueFamilyIndex ) const; /* @see GetPool( VkQueueFlags , bool ) */
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
        AcquiredQueue Acquire( bool bIgnoreFenceStatus, uint32_t queueFamilyIndex );

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

    /**
     * @brief Checks if the returned structure indicates that the operation succeeded.
     * @param acquiredQueue  The returned result structure.
     * @return True of succeeded, false otherwise.
     */
    inline bool IsOk( const AcquiredQueue& acquiredQueue ) {
        return ( VK_SUCCESS == acquiredQueue.eResult ) &&
               acquiredQueue.pQueue &&
               acquiredQueue.pFence &&
               ( kInvalidIndex != acquiredQueue.QueueId ) &&
               ( kInvalidIndex != acquiredQueue.QueueFamilyId );
    }

    /**
     * @brief Checks if the returned structure indicates that the operation succeeded.
     * @param acquiredCmdBuffer  The returned result structure.
     * @return True of succeeded, false otherwise.
     */
    inline bool IsOk( const AcquiredCommandBuffer& acquiredCmdBuffer ) {
        return ( VK_SUCCESS == acquiredCmdBuffer.eResult ) &&
               acquiredCmdBuffer.pCmdBuffer &&
               acquiredCmdBuffer.pCmdPool &&
               ( kInvalidIndex != acquiredCmdBuffer.CmdBufferId ) &&
               ( kInvalidIndex != acquiredCmdBuffer.QueueFamilyId );
    }

} // namespace apemodevk