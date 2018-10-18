// Copyright (c) 2018 Vladyslav Serhiienko <vlad.serhiienko@gmail.com>
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <QueuePools.Vulkan.h>

#include <atomic>

namespace apemodevk {

    /** @brief Contains the error code of the failed stage or the SUCCESS if the buffer was populated and submitted. */
    struct OneTimeCmdBufferSubmitResult {
        VkResult eResult = VK_SUCCESS;              /* The error code of the failed stage. */
        uint32_t QueueId = VK_QUEUE_FAMILY_IGNORED; /* The queue id that executes the command buffer. */
    };

    /**
     * @brief Checks if the returned structure indicates that the operation succeeded.
     * @param oneTimeCmdBufferSubmitResult  The returned result structure.
     * @return True of succeeded, false otherwise.
     */
    inline bool IsOk( const OneTimeCmdBufferSubmitResult& oneTimeCmdBufferSubmitResult ) {
        return ( VK_SUCCESS == oneTimeCmdBufferSubmitResult.eResult ) &&
               ( VK_QUEUE_FAMILY_IGNORED != oneTimeCmdBufferSubmitResult.QueueId );
    }

    constexpr uint64_t kDefaultQueueAcquireTimeoutNanos = 16 * 1000000; /* 16 ms */
    constexpr uint64_t kDefaultQueueAwaitTimeoutNanos = ( ~0ULL ); /* too many ms */

    /**
     * @brief The utlitly functon is used for populating and submitting command buffers.
     * @tparam TPopulateCmdBufferFn The type of the functor, that has the signature 'bool( VkCommandBuffer )'.
     * @param pNode The graphics node.
     * @param queueFamilyId The queue family id. The command buffer and the queue will be allocated from the related pools.
     * @param pAwaitQueue Indicates whether the implementation should wait for the completion of the populated command buffer.
     * @param populateCmdBufferFn The functor where the user populated the allocated command buffer.
     * @param pSignalSemaphores The semaphores that will be signaled when the command buffer is executed.
     * @param signalSemaphoreCount The number of semaphores that will be signaled.
     * @param pWaitSemaphores The semaphores that the queue will wait on before executing the populated command buffer.
     * @param waitSemaphoreCount The number of semaphores that will be awaited.
     * @return VK_SUCCESS if succeeded, VK_ERROR_OUT_OF_HOST_MEMORY,
     * @see Please find 'bool Succeeded( const OneTimeCmdBufferSubmitResult& )' utility function for checking the error status
     * of the operation.
     */
    template < typename TPopulateCmdBufferFn /* = bool( VkCommandBuffer ) { ... } */ >
    inline OneTimeCmdBufferSubmitResult TOneTimeCmdBufferSubmit(
        GraphicsDevice*             pNode,
        uint32_t                    queueFamilyId, /* = 0 */
        bool                        bAwaitQueue,   /* = true */
        TPopulateCmdBufferFn        populateCmdBufferFn,
        uint64_t                    queueAcquireTimeout  = kDefaultQueueAcquireTimeoutNanos,
        uint64_t                    queueAwaitTimeout    = kDefaultQueueAwaitTimeoutNanos,
        const VkSemaphore*          pSignalSemaphores    = nullptr,
        uint32_t                    signalSemaphoreCount = 0,
        const VkPipelineStageFlags* pWaitDstStageMask    = nullptr,
        const VkSemaphore*          pWaitSemaphores      = nullptr,
        uint32_t                    waitSemaphoreCount   = 0 )
    {
        OneTimeCmdBufferSubmitResult result {};
        /* Get command buffer from pool */
        auto pCmdBufferPool = pNode ? pNode->GetCommandBufferPool( ) : nullptr;
        if ( !pCmdBufferPool ) {
            result.eResult = VK_NOT_READY;
            return result;
        }

        auto acquiredCmdBuffer = pCmdBufferPool->Acquire( false, queueFamilyId );
        if ( !acquiredCmdBuffer.pCmdBuffer || !acquiredCmdBuffer.pCmdPool ) {
            result.eResult = VK_NOT_READY;
            return result;
        }

        apemodevk::platform::LogFmt( apemodevk::platform::LogLevel::Trace,
                                     "TOneTimeCmdBufferSubmit: Acq C %u, Q %u",
                                     acquiredCmdBuffer.CmdBufferId,
                                     acquiredCmdBuffer.QueueFamilyId );

        /* Reset command pool */

        result.eResult = vkResetCommandPool( pNode->hLogicalDevice, acquiredCmdBuffer.pCmdPool, 0 );
        if ( VK_SUCCESS != CheckedResult( result.eResult ) ) {
            // VK_ERROR_OUT_OF_HOST_MEMORY
            // VK_ERROR_OUT_OF_DEVICE_MEMORY
            return result;
        }

        /* Begin recording commands to command buffer */
        VkCommandBufferBeginInfo commandBufferBeginInfo;
        InitializeStruct( commandBufferBeginInfo );
        commandBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        result.eResult = vkBeginCommandBuffer( acquiredCmdBuffer.pCmdBuffer, &commandBufferBeginInfo );
        if ( VK_SUCCESS != CheckedResult( result.eResult ) ) {
            // VK_ERROR_OUT_OF_HOST_MEMORY
            // VK_ERROR_OUT_OF_DEVICE_MEMORY
            return result;
        }

        const bool bPopulated = populateCmdBufferFn( acquiredCmdBuffer.pCmdBuffer );

        result.eResult = vkEndCommandBuffer( acquiredCmdBuffer.pCmdBuffer );
        if ( VK_SUCCESS != CheckedResult( result.eResult ) ) {
            // VK_ERROR_OUT_OF_HOST_MEMORY
            // VK_ERROR_OUT_OF_DEVICE_MEMORY
            return result;
        }

        if ( bPopulated ) {
            assert( pNode );

            /* Get queue from pool */
            auto pQueuePool       = pNode->GetQueuePool( );
            auto pQueueFamilyPool = pQueuePool ? pQueuePool->GetPool( queueFamilyId ) : nullptr;

            if ( !pQueueFamilyPool ) {
                result.eResult = VK_ERROR_DEVICE_LOST;
                return result;
            }

            AcquiredQueue acquiredQueue = pQueueFamilyPool->Acquire( false );
            const uint64_t queueTimeStart = GetPerformanceCounter( );

            apemodevk::platform::LogFmt( apemodevk::platform::LogLevel::Trace,
                                         "TOneTimeCmdBufferSubmit: Acq Q @%p",
                                         acquiredQueue.pQueue );

            while ( acquiredQueue.pQueue == nullptr ) {

                const uint64_t queueTimeDelta = GetPerformanceCounter( ) - queueTimeStart;
                if ( queueAcquireTimeout < GetNanoseconds( queueTimeDelta ) ) {
                    result.eResult = VK_TIMEOUT;
                    return result;
                }

                switch ( acquiredQueue.eResult ) {
                    case VK_NOT_READY:
                        acquiredQueue = pQueueFamilyPool->Acquire( true );
                        apemodevk::platform::LogFmt( apemodevk::platform::LogLevel::Trace,
                                                     "TOneTimeCmdBufferSubmit: Awt Q @%p",
                                                     acquiredQueue.pQueue );
                        break;

                    default:
                        // VK_ERROR_DEVICE_LOST
                        // VK_ERROR_OUT_OF_HOST_MEMORY
                        // VK_ERROR_OUT_OF_DEVICE_MEMORY
                        result.eResult = acquiredQueue.eResult;
                        return result;
                }
            }

            result.eResult = WaitForFence( pNode->hLogicalDevice, acquiredQueue.pFence, queueAcquireTimeout );
            if ( VK_SUCCESS != result.eResult ) {
                // VK_ERROR_OUT_OF_HOST_MEMORY
                // VK_ERROR_OUT_OF_DEVICE_MEMORY
                // VK_ERROR_DEVICE_LOST
                return result;
            }

            result.eResult = vkResetFences( pNode->hLogicalDevice, 1, &acquiredQueue.pFence );
            if ( VK_SUCCESS != CheckedResult( result.eResult ) ) {
                // VK_ERROR_OUT_OF_HOST_MEMORY
                // VK_ERROR_OUT_OF_DEVICE_MEMORY
                return result;
            }

            VkSubmitInfo submitInfo;
            InitializeStruct( submitInfo );

            submitInfo.pSignalSemaphores    = pSignalSemaphores;
            submitInfo.signalSemaphoreCount = signalSemaphoreCount;
            submitInfo.pWaitSemaphores      = pWaitSemaphores;
            submitInfo.pWaitDstStageMask    = pWaitDstStageMask;
            submitInfo.waitSemaphoreCount   = waitSemaphoreCount;
            submitInfo.pCommandBuffers      = &acquiredCmdBuffer.pCmdBuffer;
            submitInfo.commandBufferCount   = 1;

            apemodevk::platform::LogFmt( apemodevk::platform::LogLevel::Trace,
                                         "TOneTimeCmdBufferSubmit: Sbm Q %u %u",
                                         acquiredQueue.QueueId,
                                         acquiredQueue.QueueFamilyId );

            result.eResult = vkQueueSubmit( acquiredQueue.pQueue, 1, &submitInfo, acquiredQueue.pFence );
            if ( VK_SUCCESS != CheckedResult( result.eResult ) ) {
                // VK_ERROR_OUT_OF_HOST_MEMORY
                // VK_ERROR_OUT_OF_DEVICE_MEMORY
                // VK_ERROR_DEVICE_LOST
                return result;
            }

            result.QueueId = acquiredQueue.QueueId;

            if ( bAwaitQueue ) {

                result.eResult = WaitForFence( pNode->hLogicalDevice, acquiredQueue.pFence, queueAwaitTimeout );
                if ( VK_SUCCESS > result.eResult ) {
                    // VK_ERROR_OUT_OF_HOST_MEMORY
                    // VK_ERROR_OUT_OF_DEVICE_MEMORY
                    // VK_ERROR_DEVICE_LOST
                    return result;
                }
            }

            acquiredCmdBuffer.pFence = acquiredQueue.pFence;
            pQueuePool->Release( acquiredQueue );
        }

        pCmdBufferPool->Release( acquiredCmdBuffer );
        return result;
    }

} // namespace apemodevk
