#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <TInfoStruct.Vulkan.h>
#include <NativeHandles.Vulkan.h>

namespace apemodevk
{
    class CommandBuffer;
    class PipelineLayout;
    class DescriptorPool;

    class DescriptorSetLayout;
    class DescriptorSetLayoutBuilder;

    template < uint32_t TCount >
    class TDescriptorSets {
    public:
        VkDevice              hNode              = VK_NULL_HANDLE;
        VkDescriptorPool      hPool              = VK_NULL_HANDLE;
        VkDescriptorSet       hSets[ TCount ]    = {VK_NULL_HANDLE};
        VkDescriptorSetLayout hLayouts[ TCount ] = {VK_NULL_HANDLE};
        uint32_t              Offsets[ TCount ]  = {0};
        uint32_t              Counts[ TCount ]   = {0};

        TDescriptorSets( ) {
        }

        ~TDescriptorSets( ) {
            if ( hNode && hPool ) {
                vkFreeDescriptorSets( hNode, hPool, TCount, hSets );
            }
        }

        bool RecreateResourcesFor( VkDevice         InNode,
                                   VkDescriptorPool InDescPool,
                                   VkDescriptorSetLayout ( &InLayouts )[ TCount ] ) {
            hNode = InNode;
            hPool = InDescPool;
            memcpy( hLayouts, InLayouts, TCount * sizeof( VkDescriptorSetLayout ) );

            VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
            InitializeStruct( descriptorSetAllocateInfo );

            descriptorSetAllocateInfo.pSetLayouts        = hLayouts;
            descriptorSetAllocateInfo.descriptorPool     = hPool;
            descriptorSetAllocateInfo.descriptorSetCount = TCount;

            //TODO: Only for debugging.
            if ( false == eastl::is_sorted( &hLayouts[ 0 ], &hLayouts[ TCount ] ) ) {
                platform::DebugBreak( );
                return false;
            }

            apemodevk::utils::ZeroMemory( Offsets );
            apemodevk::utils::ZeroMemory( Counts );

            uint32_t layoutIndex   = 0;
            uint32_t descSetOffset = 0;
            VkDescriptorSetLayout currentLayout = hLayouts[ 0 ];

            uint32_t i = 1;
            for ( ; i < TCount; ++i ) {
                if ( currentLayout != hLayouts[ i ] ) {
                    Offsets[ layoutIndex ] = descSetOffset;
                    Counts[ layoutIndex ]  = i - descSetOffset;
                    descSetOffset          = i;
                    ++layoutIndex;
                }
            }

            Offsets[ layoutIndex ] = descSetOffset;
            Counts[ layoutIndex ]  = i - descSetOffset;
            descSetOffset          = i;

            //TODO: Decrement descriptor set counter for the descriptor pool.
            //TODO: Checks

            return VK_SUCCESS == CheckedResult( vkAllocateDescriptorSets( hNode, &descriptorSetAllocateInfo, hSets ) );
        }

        void BindTo( CommandBuffer& CmdBuffer, VkPipelineBindPoint InBindPoint, const uint32_t ( &DynamicOffsets )[ TCount ] ) {
            for ( uint32_t i = 0; i < TCount; ++i ) {
                vkCmdBindDescriptorSets( CmdBuffer, InBindPoint, hLayouts[ i ], 0, 1, &hSets[ i ], 1, DynamicOffsets[ i ] );
            }
        }
    };

    class DescriptorPool : public NoCopyAssignPolicy {
    public:
        DescriptorPool( );
        ~DescriptorPool( );

        bool RecreateResourcesFor( apemodevk::GraphicsDevice& GraphicsNode,
                                   uint32_t MaxSets,
                                   uint32_t MaxSamplerCount,                 // VK_DESCRIPTOR_TYPE_SAMPLER
                                   uint32_t MaxCombinedImgSamplerCount,      // VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                                   uint32_t MaxSampledImgCount,              // VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
                                   uint32_t MaxStorageImgCount,              // VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
                                   uint32_t MaxUniformTexelBufferCount,      // VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
                                   uint32_t MaxStorageTexelBufferCount,      // VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
                                   uint32_t MaxUniformBufferCount,           // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                                   uint32_t MaxStorageBufferCount,           // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
                                   uint32_t MaxDynamicUniformBufferCount,    // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
                                   uint32_t MaxDynamicStorageBufferCount,    // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
                                   uint32_t MaxInputAttachmentCount          // VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
        );

    public:
        uint32_t GetAvailableSetCount( ) const;
        uint32_t GetAvailableDescCount( VkDescriptorType DescType ) const;

        operator VkDescriptorPool( ) const;

    public:
        apemodevk::GraphicsDevice const*       pNode;
        apemodevk::THandle< VkDescriptorPool > hDescPool;

        /** Helps to track descriptor suballocations. */
        VkDescriptorPoolSize DescPoolCounters[ VK_DESCRIPTOR_TYPE_RANGE_SIZE ];
        uint32_t             DescSetCounter;
    };

    struct DescriptorSetBindingsBase {
        struct Binding {
            uint32_t         DstBinding      = uint32_t( -1 );
            VkDescriptorType eDescriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;

            union {
                VkDescriptorImageInfo  ImageInfo;
                VkDescriptorBufferInfo BufferInfo;
            };
        };

        Binding* pBinding     = nullptr;
        uint32_t BindingCount = 0;

        DescriptorSetBindingsBase( ) {
        }

        DescriptorSetBindingsBase( Binding* pBinding = nullptr, uint32_t BindingCount = 0 )
            : pBinding( pBinding ), BindingCount( BindingCount ) {
        }
    };

    enum ETDescriptorSetNoInit {
        eTDescriptorSetNoInit = 0,
    };

    template < uint32_t TMaxBindingCount >
    struct TDescriptorSetBindings : DescriptorSetBindingsBase {
        DescriptorSetBindingsBase::Binding Bindings[ TMaxBindingCount ];

        TDescriptorSetBindings( ETDescriptorSetNoInit ) : DescriptorSetBindingsBase( Bindings, 0 ) {
            InitializeStruct( Bindings );
        }

        TDescriptorSetBindings( ) : DescriptorSetBindingsBase( Bindings, TMaxBindingCount ) {
            uint32_t DstBinding = 0;
            for ( auto& binding : Bindings ) {
                binding.DstBinding = DstBinding++;
            }
        }
    };

    struct DescriptorSetPool {
        VkDevice                                           pLogicalDevice       = VK_NULL_HANDLE;
        VkDescriptorPool                                   pDescriptorPool      = VK_NULL_HANDLE;
        VkDescriptorSetLayout                              pDescriptorSetLayout = VK_NULL_HANDLE;
        apemodevk::vector_map< uint64_t, VkDescriptorSet > Sets;
        apemodevk::vector< VkWriteDescriptorSet >          TempWrites;

        bool            Recreate( VkDevice pInLogicalDevice, VkDescriptorPool pInDescPool, VkDescriptorSetLayout pInLayout );
        VkDescriptorSet GetDescriptorSet( const DescriptorSetBindingsBase * pDescriptorSetBase );
    };
}