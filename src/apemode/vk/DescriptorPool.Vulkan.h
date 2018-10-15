#pragma once

#include <apemode/vk/GraphicsDevice.Vulkan.h>
#include <apemode/vk/TInfoStruct.Vulkan.h>
#include <apemode/vk/NativeHandles.Vulkan.h>

namespace apemodevk
{
    class CommandBuffer;
    class PipelineLayout;
    class DescriptorPool;

    class DescriptorSetLayout;
    class DescriptorSetLayoutBuilder;

    class APEMODEVK_API DescriptorPool : public NoCopyAssignPolicy {
    public:
        DescriptorPool( );
        ~DescriptorPool( );

        struct InitializeParameters {
            GraphicsDevice* pNode                                                   = nullptr;
            uint32_t        MaxDescriptorSetCount                                   = 0;
            uint32_t        MaxDescriptorPoolSizes[ VK_DESCRIPTOR_TYPE_RANGE_SIZE ] = {0};
        };

        bool
        Initialize( InitializeParameters const& initializeParameters );

    public:
        uint32_t GetAvailableDescriptorSetCount( ) const;
        uint32_t GetAvailableDescriptorPoolSize( VkDescriptorType descriptorType ) const;
        operator VkDescriptorPool( ) const;

    public:
        GraphicsDevice*                        pNode                                                   = nullptr;
        uint32_t                               MaxDescriptorSetCount                                   = 0;
        VkDescriptorPoolSize                   MaxDescriptorPoolSizes[ VK_DESCRIPTOR_TYPE_RANGE_SIZE ] = {};
        apemodevk::THandle< VkDescriptorPool > hDescriptorPool;
    };

    struct APEMODEVK_API DescriptorSetBindingsBase {
        struct APEMODEVK_API Binding {
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

        DescriptorSetBindingsBase( Binding* pBinding, uint32_t BindingCount )
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

    struct APEMODEVK_API DescriptorSetPool {
        VkDevice                                           pLogicalDevice       = VK_NULL_HANDLE;
        VkDescriptorPool                                   pDescriptorPool      = VK_NULL_HANDLE;
        VkDescriptorSetLayout                              pDescriptorSetLayout = VK_NULL_HANDLE;
        apemodevk::vector_map< uint64_t, VkDescriptorSet > Sets;
        apemodevk::vector< VkWriteDescriptorSet >          TempWrites;

        bool            Recreate( VkDevice pInLogicalDevice, VkDescriptorPool pInDescPool, VkDescriptorSetLayout pInLayout );
        VkDescriptorSet GetDescriptorSet( const DescriptorSetBindingsBase * pDescriptorSetBase );
    };
}