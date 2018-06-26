#include <DescriptorPool.Vulkan.h>
#include <CityHash.Vulkan.h>

apemodevk::DescriptorPool::DescriptorPool( ) : pNode( nullptr ) {
}

apemodevk::DescriptorPool::~DescriptorPool( ) {
}

uint32_t apemodevk::DescriptorPool::GetAvailableSetCount( ) const {
    return DescSetCounter;
}

uint32_t apemodevk::DescriptorPool::GetAvailableDescCount( VkDescriptorType descriptorType ) const {
    apemode_assert( DescPoolCounters[ descriptorType ].type == DescType, "Desc type mismatch." );

    return DescPoolCounters[ descriptorType ].descriptorCount;
}

bool apemodevk::DescriptorPool::RecreateResourcesFor( GraphicsDevice& graphicsNode,
                                                      uint32_t        maxSets,
                                                      uint32_t        maxSamplerCount,
                                                      uint32_t        maxCombinedImgCount,
                                                      uint32_t        maxSampledImgCount,
                                                      uint32_t        maxStorageImgCount,
                                                      uint32_t        maxUniformTexelBufferCount,
                                                      uint32_t        maxStorageTexelBufferCount,
                                                      uint32_t        maxUniformBufferCount,
                                                      uint32_t        maxStorageBufferCount,
                                                      uint32_t        maxDynamicUniformBufferCount,
                                                      uint32_t        maxDynamicStorageBufferCount,
                                                      uint32_t        maxInputAttachmentCount ) {
    apemodevk_memory_allocation_scope;
    DescSetCounter = maxSets;

    // This array is used for creating descriptor pool.
    VkDescriptorPoolSize descriptorPoolSizes[ VK_DESCRIPTOR_TYPE_RANGE_SIZE ];

    apemodevk::utils::ZeroMemory( DescPoolCounters );
    apemodevk::utils::ZeroMemory( descriptorPoolSizes );

    uint32_t descriptorTypeCounter     = 0;
    uint32_t sizeCounter               = 0;
    uint32_t descriptorPoolSizeCounter = 0;
    for ( const auto maxDescTypeCount : {maxSamplerCount,
                                         maxCombinedImgCount,
                                         maxSampledImgCount,
                                         maxStorageImgCount,
                                         maxUniformTexelBufferCount,
                                         maxStorageTexelBufferCount,
                                         maxUniformBufferCount,
                                         maxStorageBufferCount,
                                         maxDynamicUniformBufferCount,
                                         maxDynamicStorageBufferCount,
                                         maxInputAttachmentCount} ) {
        VkDescriptorType descriptorType = static_cast< VkDescriptorType >( descriptorTypeCounter );

        DescPoolCounters[ sizeCounter ].descriptorCount = maxDescTypeCount;
        DescPoolCounters[ sizeCounter ].type            = descriptorType;

        if ( apemode_unlikely( maxDescTypeCount ) ) {
            descriptorPoolSizes[ descriptorPoolSizeCounter ] = DescPoolCounters[ sizeCounter ];
            ++descriptorPoolSizeCounter;
        }

        ++sizeCounter;
        ++descriptorTypeCounter;
    }

    // TOFIX Does it make sense creating empty descriptor pool?
    // TOFIX Is it required by certain API functions just to provide a valid (even empty) pool?
    apemode_assert( descriptorPoolSizeCounter && maxSets, "Empty descriptor pool." );

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
    InitializeStruct( descriptorPoolCreateInfo );

    descriptorPoolCreateInfo.maxSets       = maxSets;
    descriptorPoolCreateInfo.pPoolSizes    = descriptorPoolSizes;
    descriptorPoolCreateInfo.poolSizeCount = descriptorPoolSizeCounter;

    if ( apemode_likely( hDescPool.Recreate( graphicsNode, descriptorPoolCreateInfo ) ) ) {
        pNode = &graphicsNode;
        return true;
    }

    return false;
}

apemodevk::DescriptorPool::operator VkDescriptorPool( ) const {
    return hDescPool;
}

bool apemodevk::DescriptorSetPool::Recreate( VkDevice              pInLogicalDevice,
                                             VkDescriptorPool      pInDescPool,
                                             VkDescriptorSetLayout pInLayout ) {
    apemodevk_memory_allocation_scope;

    pLogicalDevice       = pInLogicalDevice;
    pDescriptorPool      = pInDescPool;
    pDescriptorSetLayout = pInLayout;

    TempWrites.reserve( 16 ); /* TOFIX */
    return true;
}

VkDescriptorSet apemodevk::DescriptorSetPool::GetDescSet( const DescriptorSetBase* pDescriptorSetBase ) {
    apemodevk_memory_allocation_scope;

    apemodevk::CityHash64Wrapper cityHashBuilder;
    for ( uint32_t i = 0; i < pDescriptorSetBase->BindingCount; ++i ) {
        cityHashBuilder.CombineWith( pDescriptorSetBase->pBinding[ i ] );
    }

    auto descriptorSetHash = cityHashBuilder.Value;
    auto descriptorSetIt = std::find_if( Sets.begin( ), Sets.end( ), [&]( const DescriptorSetItem& allocatedSet ) {
        return allocatedSet.Hash == descriptorSetHash;
    } );

    if ( descriptorSetIt != Sets.end( ) )
        return descriptorSetIt->pDescriptorSet;

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
    InitializeStruct( descriptorSetAllocateInfo );
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.descriptorPool     = pDescriptorPool;
    descriptorSetAllocateInfo.pSetLayouts        = &pDescriptorSetLayout;

    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    if ( VK_SUCCESS != vkAllocateDescriptorSets( pLogicalDevice, &descriptorSetAllocateInfo, &descriptorSet ) ) {
        platform::DebugBreak( );
        return nullptr;
    }

    DescriptorSetItem item;
    item.Hash = descriptorSetHash;
    item.pDescriptorSet = descriptorSet;
    Sets.emplace_back( item );

    TempWrites.clear( );
    TempWrites.reserve( pDescriptorSetBase->BindingCount );

    std::for_each( pDescriptorSetBase->pBinding,
                   pDescriptorSetBase->pBinding + pDescriptorSetBase->BindingCount,
                   [&]( const DescriptorSetBase::Binding& descriptorSetBinding ) {
                       apemodevk_memory_allocation_scope;

                       TempWrites.emplace_back( );

                       auto& writeDescriptorSet = TempWrites.back( );
                       InitializeStruct( writeDescriptorSet );
                       writeDescriptorSet.descriptorCount = 1;
                       writeDescriptorSet.dstBinding      = descriptorSetBinding.DstBinding;
                       writeDescriptorSet.descriptorType  = descriptorSetBinding.eDescriptorType;
                       writeDescriptorSet.dstSet          = descriptorSet;

                       switch ( descriptorSetBinding.eDescriptorType ) {
                           // case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                           // case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                           case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                           case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                           case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                           case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                               writeDescriptorSet.pBufferInfo = &descriptorSetBinding.BufferInfo;
                               break;

                           case VK_DESCRIPTOR_TYPE_SAMPLER:
                           case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                           case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                           case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                           case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                               writeDescriptorSet.pImageInfo = &descriptorSetBinding.ImageInfo;
                               break;

                           default:
                               platform::DebugBreak( );
                       }
                   } );

    vkUpdateDescriptorSets( pLogicalDevice, static_cast< uint32_t >( TempWrites.size( ) ), TempWrites.data( ), 0, nullptr );
    return descriptorSet;
}
