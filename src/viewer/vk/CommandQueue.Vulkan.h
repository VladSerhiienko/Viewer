#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <NativeDispatchableHandles.Vulkan.h>

namespace apemodevk
{
    class CommandBuffer;
    class CommandQueue;
    class RenderPass;
    class Framebuffer;
    class PipelineLayout;
    class PipelineState;

    class CommandBuffer : public apemodevk::ScalableAllocPolicy,
                                                    public apemodevk::NoCopyAssignPolicy
    {
    public:
        enum CommandListType
        {
            kCommandListType_Direct = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            kCommandListType_Bundle = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
            kCommandListType_Invalid = -1
        };

    public:
        struct BeginEndScope;
        friend BeginEndScope;

    public:
        using BarrierVector       = std::vector<VkMemoryBarrier>;
        using ImageBarrierVector  = std::vector<VkImageMemoryBarrier>;
        using BufferBarrierVector = std::vector<VkBufferMemoryBarrier>;
        using PtrVector           = std::vector<CommandBuffer *>;

    public:
        static std::unique_ptr<CommandBuffer> MakeNewUnique ();
        static std::shared_ptr<CommandBuffer> MakeNewLinked ();

    public:
        struct BeginEndScope
        {
            CommandBuffer & AssociatedCmdList;
            BeginEndScope (CommandBuffer & CmdBuffer, bool bOneTimeSubmit);
            BeginEndScope (CommandBuffer &                          CmdBuffer,
                           VkCommandBufferInheritanceInfo const & Inheritance,
                           bool                                   bOneTimeSubmit);
            ~BeginEndScope ();
        };

    public:
        CommandBuffer();

        /**
         * Creates command buffer and command pool objects.
         * @returns True if succeeded, false otherwise.
         */
        bool RecreateResourcesFor(GraphicsDevice & GraphicsNode,
                                  uint32_t         queueFamilyId,
                                  bool             bIsDirect,
                                  bool             bIsTransient);

        bool Reset(bool bReleaseResources);

        void InsertBarrier(VkPipelineStageFlags    SrcPipelineStageFlags,
                           VkPipelineStageFlags    DstPipelineStageFlags,
                           VkMemoryBarrier const & Barrier);

        void InsertBarrier(VkPipelineStageFlags         SrcPipelineStageFlags,
                           VkPipelineStageFlags         DstPipelineStageFlags,
                           VkImageMemoryBarrier const & Barrier);

        void InsertBarrier(VkPipelineStageFlags          SrcPipelineStageFlags,
                           VkPipelineStageFlags          DstPipelineStageFlags,
                           VkBufferMemoryBarrier const & Barrier);

        void FlushStagedBarriers();

    public:
        bool IsDirect () const;

    public:
        operator VkCommandBuffer() const;

    protected:
        struct StagedBarrier : public apemodevk::ScalableAllocPolicy
        {
            enum EType
            {
                kType_Global,
                kType_Img,
                kType_Buffer,
            };

            union
            {
                struct
                {
                    VkPipelineStageFlags SrcStage;
                    VkPipelineStageFlags DstStage;
                };

                static_assert (sizeof (VkPipelineStageFlags) == sizeof (uint32_t),
                               "Reconsider hashing.");

                uint64_t StageHash;
            };

            union
            {
                VkStructureType       BarrierType;
                VkMemoryBarrier       Barrier;
                VkImageMemoryBarrier  ImgBarrier;
                VkBufferMemoryBarrier BufferBarrier;
            };

            StagedBarrier (VkPipelineStageFlags    SrcStage,
                           VkPipelineStageFlags    DstStage,
                           VkMemoryBarrier const & Barrier);

            StagedBarrier (VkPipelineStageFlags         SrcStage,
                           VkPipelineStageFlags         DstStage,
                           VkImageMemoryBarrier const & Barrier);

            StagedBarrier (VkPipelineStageFlags          SrcStage,
                           VkPipelineStageFlags          DstStage,
                           VkBufferMemoryBarrier const & Barrier);

            using Key       = uint64_t;
            using KeyLessOp = std::less<uint64_t>;
            using Multimap  = std::multimap<const Key, const StagedBarrier, KeyLessOp>;
            using It        = Multimap::iterator;
            using ItRange   = std::pair<It, It>;
            using Pair      = std::pair<Key, StagedBarrier>;
        };

    public:
        CommandListType eType;
        bool            bIsInBeginEndScope;

        StagedBarrier::Multimap StagedBarriers;
        BarrierVector           Barriers;
        ImageBarrierVector      ImgBarriers;
        BufferBarrierVector     BufferBarriers;
        uint32_t                BarrierCount;
        uint32_t                ImgBarrierCount;
        uint32_t                BufferBarrierCount;

        apemodevk::RenderPass const *                   pRenderPass;
        apemodevk::Framebuffer const *                  pFramebuffer;
        apemodevk::PipelineLayout const *                pPipelineLayout;
        apemodevk::PipelineState const *                pPipelineState;
        apemodevk::TDispatchableHandle<VkCommandBuffer> hCmdList;
        apemodevk::TDispatchableHandle<VkCommandPool>   hCmdAlloc;
    };

    /**
     * Stores reserved command queues of devices. This class is used by queues,
     * but can also be potentially used by the graphics devices.
     */
    class CommandQueueReserver : public apemodevk::ScalableAllocPolicy,
                                                             public apemodevk::NoCopyAssignPolicy
    {
        friend CommandQueue;
        friend GraphicsDevice;

        struct Key
        {
            uint64_t GraphicsNodeHash;

            union 
            {
                struct
                {
                    uint32_t queueId;
                    uint32_t queueFamilyId;
                };

                uint64_t QueueHash;
            };

            Key();
            struct Hasher { size_t operator () (Key const &) const; };
            struct CmpOpLess { bool operator () (Key const &, Key const &) const; };
            struct CmpOpEqual { bool operator () (Key const &, Key const &) const; };
            static Key NewKeyFor (GraphicsDevice const & GraphicsNode, uint32_t queueFamilyId, uint32_t queueId);
        };

        struct Reservation {
            CommandQueue *pQueue;
            uint32_t      queueId;
            uint32_t      queueFamilyId;

            Reservation( );
            bool IsValid( ) const;
            void Release( );

            using CmpOpLess       = Key::CmpOpLess;
            using LookupContainer = std::map< Key, Reservation, CmpOpLess >;
        };

        Reservation::LookupContainer ReservationStorage;

        CommandQueueReserver( );
        /** Returns True if this queue was not created previously, otherwise false. */
        bool TryReserve( GraphicsDevice const &GraphicsNode, uint32_t queueFamilyId, uint32_t queueId );
        /** Must be called when the queue gets destructed to avoid resources leaks. */
        void Unreserve( GraphicsDevice const &GraphicsNode, uint32_t queueFamilyId, uint32_t queueId );
        /** Returns queue reserver instance. */
        static CommandQueueReserver &Get( );
    };

    class CommandQueue : public apemodevk::NoCopyAssignPolicy {
    public:
        CommandQueue( );
        ~CommandQueue( );

    public:
        bool RecreateResourcesFor( GraphicsDevice &GraphicsNode, uint32_t queueFamilyId, uint32_t queueId );

        bool Execute( CommandBuffer &CmdBuffer, VkSemaphore *hWaitSemaphore, uint32_t WaitSemaphoreCount, VkFence hFence );
        bool Execute( CommandBuffer &CmdBuffer, VkFence hFence );
        bool Execute( CommandBuffer *pCmdLists, uint32_t CmdListCount, VkFence Fence );

        /** 
         * Waits on the completion of all work within a single queue.
         * It will block until all command buffers and sparse binding 
         * operations in the queue have completed.
         */
        bool Await( );

    public:
        operator VkQueue( ) const;

    public:
        GraphicsDevice *               pNode;
        TDispatchableHandle< VkQueue > hCmdQueue;
        uint32_t                       queueFamilyId;
        uint32_t                       queueId;
    };
}