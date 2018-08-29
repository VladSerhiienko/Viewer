#include <DescriptorPool.Vulkan.h>
#include <CityHash.Vulkan.h>

apemodevk::DescriptorPool::DescriptorPool( ) : pNode( nullptr ) {
}

apemodevk::DescriptorPool::~DescriptorPool( ) {
}

uint32_t apemodevk::DescriptorPool::GetAvailableDescriptorSetCount( ) const {
    return MaxDescriptorSetCount;
}

uint32_t apemodevk::DescriptorPool::GetAvailableDescriptorPoolSize( VkDescriptorType descriptorType ) const {
    apemode_assert( MaxDescriptorPoolSizes[ descriptorType ].type == DescType, "Desc type mismatch." );
    return MaxDescriptorPoolSizes[ descriptorType ].descriptorCount;
}

bool apemodevk::DescriptorPool::Initialize( InitializeParameters const& initializeParameters ) {
    apemodevk_memory_allocation_scope;

    pNode                 = initializeParameters.pNode;
    MaxDescriptorSetCount = initializeParameters.MaxDescriptorSetCount;

    for ( uint32_t typeIndex = 0; typeIndex < VK_DESCRIPTOR_TYPE_RANGE_SIZE; ++typeIndex ) {
        MaxDescriptorPoolSizes[ typeIndex ].type            = static_cast< VkDescriptorType >( typeIndex );
        MaxDescriptorPoolSizes[ typeIndex ].descriptorCount = initializeParameters.MaxDescriptorPoolSizes[ typeIndex ];
    }

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
    InitializeStruct( descriptorPoolCreateInfo );

    descriptorPoolCreateInfo.maxSets       = MaxDescriptorSetCount;
    descriptorPoolCreateInfo.pPoolSizes    = MaxDescriptorPoolSizes;
    descriptorPoolCreateInfo.poolSizeCount = VK_DESCRIPTOR_TYPE_RANGE_SIZE;

    return hDescriptorPool.Recreate( *pNode, descriptorPoolCreateInfo );
}

apemodevk::DescriptorPool::operator VkDescriptorPool( ) const {
    return hDescriptorPool;
}

bool apemodevk::DescriptorSetPool::Recreate( VkDevice              pInLogicalDevice,
                                             VkDescriptorPool      pInDescriptorPool,
                                             VkDescriptorSetLayout pInDescriptorSetLayout ) {
    apemodevk_memory_allocation_scope;

    pLogicalDevice       = pInLogicalDevice;
    pDescriptorPool      = pInDescriptorPool;
    pDescriptorSetLayout = pInDescriptorSetLayout;

    return true;
}

VkDescriptorSet apemodevk::DescriptorSetPool::GetDescriptorSet( const DescriptorSetBindingsBase* pDescriptorSetBase ) {
    apemodevk_memory_allocation_scope;

    apemodevk::CityHash64Wrapper cityHashBuilder;
    for ( uint32_t i = 0; i < pDescriptorSetBase->BindingCount; ++i ) {
        cityHashBuilder.CombineWith( pDescriptorSetBase->pBinding[ i ] );
    }

    const uint64_t descriptorSetHash = cityHashBuilder.Value;

    auto descriptorSetIt = Sets.find( descriptorSetHash );
    if ( descriptorSetIt != Sets.end( ) )
        return descriptorSetIt->second;

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
    InitializeStruct( descriptorSetAllocateInfo );
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.descriptorPool     = pDescriptorPool;
    descriptorSetAllocateInfo.pSetLayouts        = &pDescriptorSetLayout;

    VkDescriptorSet pDescriptorSet = VK_NULL_HANDLE;
    if ( VK_SUCCESS != vkAllocateDescriptorSets( pLogicalDevice, &descriptorSetAllocateInfo, &pDescriptorSet ) ) {
        platform::DebugBreak( );
        return nullptr;
    }

    Sets[ descriptorSetHash ] = pDescriptorSet;

    TempWrites.clear( );
    TempWrites.reserve( pDescriptorSetBase->BindingCount );

    auto pBindingIt    = pDescriptorSetBase->pBinding;
    auto pBindingEndIt = pBindingIt + pDescriptorSetBase->BindingCount;

    eastl::for_each( pBindingIt, pBindingEndIt, [&]( const DescriptorSetBindingsBase::Binding& descriptorSetBinding ) {
        apemodevk_memory_allocation_scope;

        auto& writeDescriptorSet = TempWrites.emplace_back( );
        InitializeStruct( writeDescriptorSet );

        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.dstBinding      = descriptorSetBinding.DstBinding;
        writeDescriptorSet.descriptorType  = descriptorSetBinding.eDescriptorType;
        writeDescriptorSet.dstSet          = pDescriptorSet;

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
                break;
        }
    } );

    vkUpdateDescriptorSets( pLogicalDevice, static_cast< uint32_t >( TempWrites.size( ) ), TempWrites.data( ), 0, nullptr );
    return pDescriptorSet;
}
