// Copyright (c) 2018 Vladyslav Serhiienko <vlad.serhiienko@gmail.com>
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <QueuePools.Vulkan.h>

#include <atomic>

namespace apemodevk {

    /**
     * @brief OneTimeCmdBufferSubmitResult structure contains result values for the buffer creation and submission stages.
     *        Since some of the API functions might return OUT_OF_MEMORY or DEVICE_LOST error codes,
     *        the user must have a way to handle it on time.
     */
    struct OneTimeCmdBufferSubmitResult {
        VkResult eResetCmdPoolResult    = VK_SUCCESS;
        VkResult eBeginCmdBuffer        = VK_SUCCESS;
        VkResult eEndCmdBuffer          = VK_SUCCESS;
        VkResult eWaitForFence          = VK_SUCCESS;
        VkResult eResetFence            = VK_SUCCESS;
        bool     bAcquiredQueue         = true;
        VkResult eQueueSubmit           = VK_SUCCESS;
        VkResult eWaitForSubmittedFence = VK_SUCCESS;
        uint32_t QueueFamilyId          = VK_QUEUE_FAMILY_IGNORED;
        uint32_t QueueId                = VK_QUEUE_FAMILY_IGNORED;
    };

    /**
     * @return Returns true if TOneTimeCmdBufferSubmit() succeeded, false otherwise.
     */
    inline bool Succeeded( const OneTimeCmdBufferSubmitResult& result ) {
        return result.bAcquiredQueue &&
                ( VK_SUCCESS == result.eResetCmdPoolResult ) &&
                ( VK_SUCCESS == result.eBeginCmdBuffer ) &&
                ( VK_SUCCESS == result.eEndCmdBuffer ) &&
                ( VK_SUCCESS == result.eWaitForFence ) &&
                ( VK_SUCCESS == result.eQueueSubmit ) &&
                ( ( VK_SUCCESS == result.eWaitForSubmittedFence ) ||
                    ( ( VK_QUEUE_FAMILY_IGNORED != result.QueueId ) &&
                        ( VK_QUEUE_FAMILY_IGNORED != result.QueueFamilyId ) ) );
    }

    /**
     * @brief The utlitly functon is used for populating and submitting the command buffer.
     * @tparam TPopulateCmdBufferFn The type of the functor, that has the signature 'bool( VkCommandBuffer )'.
     * @param pNode The graphics node.
     * @param queueFamilyId The queue family id. The command buffer and the queue will be allocated from the related pools.
     * @param pAwaitQueue Indicates whether the implementation should wait for the completion of the populated command buffer.
     * @param populateCmdBufferFn The functor where the user populated the allocated command buffer.
     * @param pSignalSemaphores The semaphores that will be signaled when the command buffer is executed.
     * @param signalSemaphoreCount The number of semaphores that will be signaled.
     * @param pWaitSemaphores The semaphores that the queue will wait on before executing the populated command buffer.
     * @param waitSemaphoreCount The number of semaphores that will be awaited.
     * @return The OneTimeCmdBufferSubmitResult instance that will contain the per-stage error codes.
     * @see Please find 'bool Succeeded( const OneTimeCmdBufferSubmitResult& )' utility function for checking the error status of the operation.
     */
    template < typename TPopulateCmdBufferFn /* = std::function< bool( VkCommandBuffer ) > */ >
    OneTimeCmdBufferSubmitResult TOneTimeCmdBufferSubmit( GraphicsDevice*      pNode,
                                                          uint32_t             queueFamilyId, /* = 0 */
                                                          bool                 pAwaitQueue,   /* = true */
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
