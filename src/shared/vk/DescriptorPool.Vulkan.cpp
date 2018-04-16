#include <DescriptorPool.Vulkan.h>

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
    DescSetCounter = maxSets;

    // This array is used for creating descriptor pool.
    VkDescriptorPoolSize descriptorPoolSizes[ VK_DESCRIPTOR_TYPE_RANGE_SIZE ];

    apemodevk::ZeroMemory( DescPoolCounters );
    apemodevk::ZeroMemory( descriptorPoolSizes );

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

apemodevk::DescriptorSet::DescriptorSet( ) : pDescriptorPool( nullptr ), pNode( nullptr ) {
}

apemodevk::DescriptorSet::~DescriptorSet( ) {
    if ( pDescriptorPool ) {
    }
}

bool apemodevk::DescriptorSet::RecreateResourcesFor( apemodevk::GraphicsDevice& node,
                                                     apemodevk::DescriptorPool& descriptorPool,
                                                     VkDescriptorSetLayout      descriptorSetLayout ) {
    if ( descriptorPool.DescSetCounter >= 1 ) {
        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
        InitializeStruct( descriptorSetAllocateInfo );

        descriptorSetAllocateInfo.pSetLayouts        = &descriptorSetLayout;
        descriptorSetAllocateInfo.descriptorPool     = descriptorPool;
        descriptorSetAllocateInfo.descriptorSetCount = 1;

        if ( apemode_likely( hDescSet.Recreate( node, descriptorPool, descriptorSetAllocateInfo ) ) ) {
            pDescriptorPool = &descriptorPool;
            pNode           = &node;

            --descriptorPool.DescSetCounter;
            return true;
        }
    }

    return false;
}

void apemodevk::DescriptorSet::BindTo( apemodevk::CommandBuffer& CmdBuffer,
                                       VkPipelineBindPoint       PipelineBindPoint,
                                       uint32_t                  DynamicOffsetCount,
                                       const uint32_t*           DynamicOffsets ) {
    /*apemode_assert (pRootSign != nullptr && hDescSet.IsNotNull (), "Not initialized.");
    if (apemode_likely (pRootSign != nullptr && hDescSet.IsNotNull ()))
    {
        vkCmdBindDescriptorSets (CmdBuffer,
                                 PipelineBindPoint,
                                 *pRootSign,
                                 0,
                                 1,
                                 hDescSet,
                                 DynamicOffsetCount,
                                 DynamicOffsets);
    }*/
}

apemodevk::DescriptorSet::operator VkDescriptorSet( ) const {
    return hDescSet;
}

apemodevk::DescriptorSetUpdater::DescriptorSetUpdater( ) {
}

void apemodevk::DescriptorSetUpdater::Reset( uint32_t maxWrites, uint32_t maxCopies ) {
    pNode = nullptr;

    BufferInfos.clear( );
    ImgInfos.clear( );
    Writes.clear( );
    Copies.clear( );

    BufferInfos.reserve( maxWrites );
    ImgInfos.reserve( maxWrites );
    Writes.reserve( maxWrites );
    Copies.reserve( maxCopies );
}

bool apemodevk::DescriptorSetUpdater::SetGraphicsNode( GraphicsDevice const& node ) {
    if ( apemode_likely( pNode ) ) {
        apemode_assert( pNode == &GraphicsNode, "Descriptor sets of different devices." );
        return pNode == &node;
    }

    pNode = &node;
    return true;
}

bool apemodevk::DescriptorSetUpdater::WriteUniformBuffer( DescriptorSet const& descriptorSet,
                                                          VkBuffer             pBuffer,
                                                          uint32_t             offset,
                                                          uint32_t             totalSize,
                                                          uint32_t             binding,
                                                          uint32_t             count ) {
    if ( descriptorSet.pDescriptorPool->GetAvailableDescCount( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ) < count ) {
        apemode_halt( "Reserve more." );
        return false;
    }

    if ( !SetGraphicsNode( *descriptorSet.pNode ) ) {
        return false;
    }

    uintptr_t BufferInfoIdx = BufferInfos.size( );
    auto&     Write         = apemodevk::PushBackAndGet( Writes );
    auto&     BufferInfo    = apemodevk::PushBackAndGet( BufferInfos );

    BufferInfo.buffer = pBuffer;
    BufferInfo.offset = offset;
    BufferInfo.range  = totalSize;

    Write                 = TNewInitializedStruct< VkWriteDescriptorSet >( );
    Write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    Write.descriptorCount = count;
    Write.dstBinding      = binding;
    Write.dstSet          = descriptorSet;
    Write.pBufferInfo     = reinterpret_cast< const VkDescriptorBufferInfo* >( BufferInfoIdx );

    return true;
}

bool apemodevk::DescriptorSetUpdater::WriteCombinedImgSampler( DescriptorSet const& descriptorSet,
                                                               VkImageView          pImageView,
                                                               VkImageLayout        pImageLayout,
                                                               VkSampler            pSampler,
                                                               uint32_t             binding,
                                                               uint32_t             count ) {
    if ( descriptorSet.pDescriptorPool->GetAvailableDescCount( VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ) < count ) {
        apemode_halt( "Reserve more." );
        return false;
    }

    if ( !SetGraphicsNode( *descriptorSet.pNode ) ) {
        return false;
    }

    uintptr_t ImgInfoIdx = ImgInfos.size( );
    auto&     Write      = apemodevk::PushBackAndGet( Writes );
    auto&     ImgInfo    = apemodevk::PushBackAndGet( ImgInfos );

    ImgInfo.sampler     = pSampler;
    ImgInfo.imageView   = pImageView;
    ImgInfo.imageLayout = pImageLayout;

    Write                 = TNewInitializedStruct< VkWriteDescriptorSet >( );
    Write.dstSet          = descriptorSet;
    Write.dstBinding      = binding;
    Write.descriptorCount = count;
    Write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    Write.pImageInfo      = reinterpret_cast< const VkDescriptorImageInfo* >( ImgInfoIdx );

    return true;
}

void apemodevk::DescriptorSetUpdater::Flush( ) {
    for ( auto& Write : Writes ) {
        if ( Write.pBufferInfo ) {
            uintptr_t bufferInfoIdx = reinterpret_cast< const uintptr_t >( Write.pBufferInfo );
            Write.pBufferInfo       = &BufferInfos[ bufferInfoIdx ];
        } else if ( Write.pImageInfo ) {
            uintptr_t imgInfoIdx = reinterpret_cast< const uintptr_t >( Write.pImageInfo );
            Write.pImageInfo     = &ImgInfos[ imgInfoIdx ];
        }
    }

    apemode_assert( pNode != nullptr, "No writes or copies were submitted." );

    if ( apemode_likely( pNode != nullptr ) ) {
        vkUpdateDescriptorSets( *pNode,
                                GetSizeU( Writes ),
                                Writes.empty( ) ? nullptr : Writes.data( ),
                                GetSizeU( Copies ),
                                Copies.empty( ) ? nullptr : Copies.data( ) );
    }
}

bool apemodevk::DescriptorSetPool::Recreate( VkDevice              pInLogicalDevice,
                                             VkDescriptorPool      pInDescPool,
                                             VkDescriptorSetLayout pInLayout ) {
    pLogicalDevice       = pInLogicalDevice;
    pDescriptorPool      = pInDescPool;
    pDescriptorSetLayout = pInLayout;

    TempWrites.reserve( 16 ); /* TOFIX */
    return true;
}

//VkDescriptorSet apemodevk::DescriptorSetPool::GetDescSet( const VkDescriptorBufferInfo& BufferInfo,
//                                                          VkDescriptorType              eInType,
//                                                          uint32_t                      DstBinding ) {
//    auto descSetIt = std::find_if( BufferSets.begin( ), BufferSets.end( ), [&]( const DescriptorBufferInfo& allocatedSet ) {
//        return allocatedSet.BufferInfo.buffer == BufferInfo.buffer && /* Buffer handles must match */
//               allocatedSet.BufferInfo.offset == BufferInfo.offset && /* Offsets must match */
//               allocatedSet.BufferInfo.range >= BufferInfo.range &&   /* Range can be greater */
//               allocatedSet.DstBinding == DstBinding &&               /* Bindign must match */
//               allocatedSet.eType == eInType;                         /* Descriptor type must match */
//    } );
//
//    if ( descSetIt != BufferSets.end( ) )
//        return descSetIt->pDescriptorSet;
//
//    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
//    InitializeStruct( descriptorSetAllocateInfo );
//    descriptorSetAllocateInfo.descriptorSetCount = 1;
//    descriptorSetAllocateInfo.descriptorPool     = pDescriptorPool;
//    descriptorSetAllocateInfo.pSetLayouts        = &pDescriptorSetLayout;
//
//    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
//    if ( VK_SUCCESS != vkAllocateDescriptorSets( pLogicalDevice, &descriptorSetAllocateInfo, &descriptorSet ) ) {
//        DebugBreak( );
//        return nullptr;
//    }
//
//    BufferSets.emplace_back( DescriptorBufferInfo{descriptorSet, BufferInfo, eInType, DstBinding} );
//
//    VkWriteDescriptorSet writeDescriptorSet;
//    InitializeStruct( writeDescriptorSet );
//    writeDescriptorSet.descriptorCount = 1;
//    writeDescriptorSet.descriptorType  = eInType;
//    writeDescriptorSet.pBufferInfo     = &BufferInfo;
//    writeDescriptorSet.dstSet          = descriptorSet;
//    writeDescriptorSet.dstBinding      = DstBinding;
//    vkUpdateDescriptorSets( pLogicalDevice, 1, &writeDescriptorSet, 0, nullptr );
//
//    return descriptorSet;
//}
//
//VkDescriptorSet apemodevk::DescriptorSetPool::GetDescSet( const VkDescriptorImageInfo& ImageInfo,
//                                                          VkDescriptorType             eInType,
//                                                          uint32_t                     DstBinding ) {
//    auto descSetIt = std::find_if( ImgSets.begin( ), ImgSets.end( ), [&]( const DescriptorImageInfo& allocatedSet ) {
//        return allocatedSet.ImageInfo.imageLayout == ImageInfo.imageLayout && /* Buffer handles must match */
//               allocatedSet.ImageInfo.imageView == ImageInfo.imageView &&     /* Descriptor type must match */
//               allocatedSet.ImageInfo.sampler == ImageInfo.sampler &&         /* Offsets must match */
//               allocatedSet.DstBinding == DstBinding &&                       /* Bindign must match */
//               allocatedSet.eType == eInType;                                 /* Descriptor type must match */
//    } );
//
//    if ( descSetIt != ImgSets.end( ) )
//        return descSetIt->pDescriptorSet;
//
//    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
//    InitializeStruct( descriptorSetAllocateInfo );
//    descriptorSetAllocateInfo.descriptorSetCount = 1;
//    descriptorSetAllocateInfo.descriptorPool     = pDescriptorPool;
//    descriptorSetAllocateInfo.pSetLayouts        = &pDescriptorSetLayout;
//
//    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
//    if ( VK_SUCCESS != vkAllocateDescriptorSets( pLogicalDevice, &descriptorSetAllocateInfo, &descriptorSet ) ) {
//        DebugBreak( );
//        return nullptr;
//    }
//
//    ImgSets.emplace_back( DescriptorImageInfo{descriptorSet, ImageInfo, eInType, DstBinding} );
//
//    VkWriteDescriptorSet writeDescriptorSet;
//    InitializeStruct( writeDescriptorSet );
//    writeDescriptorSet.descriptorCount = 1;
//    writeDescriptorSet.descriptorType  = eInType;
//    writeDescriptorSet.pImageInfo      = &ImageInfo;
//    writeDescriptorSet.dstSet          = descriptorSet;
//    writeDescriptorSet.dstBinding      = DstBinding;
//    vkUpdateDescriptorSets( pLogicalDevice, 1, &writeDescriptorSet, 0, nullptr );
//
//    return descriptorSet;
//}

VkDescriptorSet apemodevk::DescriptorSetPool::GetDescSet( const DescriptorSetBase* pDescriptorSetBase ) {

    apemode::CityHash64Wrapper cityHashBuilder;
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

//apemodevk::DescriptorSetPool::DescriptorBufferInfo::DescriptorBufferInfo( ) {
//}
//
//apemodevk::DescriptorSetPool::DescriptorBufferInfo::DescriptorBufferInfo( VkDescriptorSet        pDescriptorSet,
//                                                                          VkDescriptorBufferInfo bufferInfo,
//                                                                          VkDescriptorType       eType,
//                                                                          uint32_t               DstBinding )
//    : pDescriptorSet( pDescriptorSet ), BufferInfo( bufferInfo ), eType( eType ), DstBinding( DstBinding ) {
//}
//
//apemodevk::DescriptorSetPool::DescriptorImageInfo::DescriptorImageInfo( ) {
//}
//
//apemodevk::DescriptorSetPool::DescriptorImageInfo::DescriptorImageInfo( VkDescriptorSet       pDescriptorSet,
//                                                                        VkDescriptorImageInfo imageInfo,
//                                                                        VkDescriptorType      eType,
//                                                                        uint32_t              DstBinding )
//    : pDescriptorSet( pDescriptorSet ), ImageInfo( imageInfo ), eType( eType ), DstBinding( DstBinding ) {
//}
