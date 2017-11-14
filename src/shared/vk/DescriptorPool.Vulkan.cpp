//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <DescriptorPool.Vulkan.h>

#include <CommandQueue.Vulkan.h>

apemodevk::DescriptorPool::DescriptorPool( ) : pNode( nullptr ) {
}

apemodevk::DescriptorPool::~DescriptorPool( ) {
}

uint32_t apemodevk::DescriptorPool::GetAvailableSetCount( ) const {
    return DescSetCounter;
}

uint32_t apemodevk::DescriptorPool::GetAvailableDescCount( VkDescriptorType DescType ) const {
    apemode_assert( DescPoolCounters[ DescType ].type == DescType, "Desc type mismatch." );

    return DescPoolCounters[ DescType ].descriptorCount;
}

bool apemodevk::DescriptorPool::RecreateResourcesFor( GraphicsDevice& GraphicsNode,
                                                      uint32_t        MaxSets,
                                                      uint32_t        MaxSamplerCount,
                                                      uint32_t        MaxCombinedImgCount,
                                                      uint32_t        MaxSampledImgCount,
                                                      uint32_t        MaxStorageImgCount,
                                                      uint32_t        MaxUniformTexelBufferCount,
                                                      uint32_t        MaxStorageTexelBufferCount,
                                                      uint32_t        MaxUniformBufferCount,
                                                      uint32_t        MaxStorageBufferCount,
                                                      uint32_t        MaxDynamicUniformBufferCount,
                                                      uint32_t        MaxDynamicStorageBufferCount,
                                                      uint32_t        MaxInputAttachmentCount ) {
    DescSetCounter = MaxSets;

    // This array is used for creating descriptor pool.
    VkDescriptorPoolSize LclSubpoolSizes[ VK_DESCRIPTOR_TYPE_RANGE_SIZE ];

    apemodevk::ZeroMemory( DescPoolCounters );
    apemodevk::ZeroMemory( LclSubpoolSizes );

    uint32_t DescTypeCounter       = 0;
    uint32_t SubpoolSizeCounter    = 0;
    uint32_t LclSubpoolSizeCounter = 0;
    for ( const auto MaxDescTypeCount : {MaxSamplerCount,
                                         MaxCombinedImgCount,
                                         MaxSampledImgCount,
                                         MaxStorageImgCount,
                                         MaxUniformTexelBufferCount,
                                         MaxStorageTexelBufferCount,
                                         MaxUniformBufferCount,
                                         MaxStorageBufferCount,
                                         MaxDynamicUniformBufferCount,
                                         MaxDynamicStorageBufferCount,
                                         MaxInputAttachmentCount} ) {
        VkDescriptorType DescType = static_cast< VkDescriptorType >( DescTypeCounter );

        DescPoolCounters[ SubpoolSizeCounter ].descriptorCount = MaxDescTypeCount;
        DescPoolCounters[ SubpoolSizeCounter ].type            = DescType;

        if ( apemode_unlikely( MaxDescTypeCount ) ) {
            LclSubpoolSizes[ LclSubpoolSizeCounter ] = DescPoolCounters[ SubpoolSizeCounter ];
            ++LclSubpoolSizeCounter;
        }

        ++SubpoolSizeCounter;
        ++DescTypeCounter;
    }

    // TOFIX Does it make sense creating empty descriptor pool?
    // TOFIX Is it required by certain API functions just to provide a valid (even empty) pool?
    apemode_assert( LclSubpoolSizeCounter && MaxSets, "Empty descriptor pool." );

    TInfoStruct< VkDescriptorPoolCreateInfo > DescPoolDesc;
    DescPoolDesc->maxSets       = MaxSets;
    DescPoolDesc->pPoolSizes    = LclSubpoolSizes;
    DescPoolDesc->poolSizeCount = LclSubpoolSizeCounter;

    if ( apemode_likely( hDescPool.Recreate( GraphicsNode, DescPoolDesc ) ) ) {
        pNode = &GraphicsNode;
        return true;
    }

    return false;
}

apemodevk::DescriptorPool::operator VkDescriptorPool( ) const {
    return hDescPool;
}

apemodevk::DescriptorSet::DescriptorSet( ) : pDescPool( nullptr ), pNode( nullptr ) {
}

apemodevk::DescriptorSet::~DescriptorSet( ) {
    if ( pDescPool ) {
    }
}

bool apemodevk::DescriptorSet::RecreateResourcesFor( apemodevk::GraphicsDevice& GraphicsNode,
                                                     apemodevk::DescriptorPool& DescPool,
                                                     VkDescriptorSetLayout      DescSetLayout ) {
    if ( DescPool.DescSetCounter >= 1 ) {
        TInfoStruct< VkDescriptorSetAllocateInfo > AllocInfo;
        AllocInfo->pSetLayouts        = &DescSetLayout;
        AllocInfo->descriptorPool     = DescPool;
        AllocInfo->descriptorSetCount = 1;

        if ( apemode_likely( hDescSet.Recreate( GraphicsNode, DescPool, AllocInfo ) ) ) {
            pDescPool = &DescPool;
            // pRootSign     = &RootSign;
            pNode = &GraphicsNode;

            --DescPool.DescSetCounter;
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

void apemodevk::DescriptorSetUpdater::Reset( uint32_t MaxWrites, uint32_t MaxCopies ) {
    pNode = nullptr;

    BufferInfos.clear( );
    ImgInfos.clear( );
    Writes.clear( );
    Copies.clear( );

    BufferInfos.reserve( MaxWrites );
    ImgInfos.reserve( MaxWrites );
    Writes.reserve( MaxWrites );
    Copies.reserve( MaxCopies );
}

bool apemodevk::DescriptorSetUpdater::SetGraphicsNode( GraphicsDevice const& GraphicsNode ) {
    if ( apemode_likely( pNode ) ) {
        apemode_assert( pNode == &GraphicsNode, "Descriptor sets of different devices." );
        return pNode == &GraphicsNode;
    }

    pNode = &GraphicsNode;
    return true;
}

bool apemodevk::DescriptorSetUpdater::WriteUniformBuffer(
    DescriptorSet const& DescSet, VkBuffer Buffer, uint32_t Offset, uint32_t TotalSize, uint32_t Binding, uint32_t Count ) {
    if ( DescSet.pDescPool->GetAvailableDescCount( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ) < Count ) {
        apemode_halt( "Reserve more." );
        return false;
    }

    if ( !SetGraphicsNode( *DescSet.pNode ) ) {
        return false;
    }

    uintptr_t BufferInfoIdx = BufferInfos.size( );
    auto&     Write         = apemodevk::PushBackAndGet( Writes );
    auto&     BufferInfo    = apemodevk::PushBackAndGet( BufferInfos );

    BufferInfo.buffer = Buffer;
    BufferInfo.offset = Offset;
    BufferInfo.range  = TotalSize;

    Write                 = TInfoStruct< VkWriteDescriptorSet >( );
    Write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    Write.descriptorCount = Count;
    Write.dstBinding      = Binding;
    Write.dstSet          = DescSet;
    Write.pBufferInfo     = reinterpret_cast< const VkDescriptorBufferInfo* >( BufferInfoIdx );

    return true;
}

bool apemodevk::DescriptorSetUpdater::WriteCombinedImgSampler( DescriptorSet const& DescSet,
                                                               VkImageView          ImgView,
                                                               VkImageLayout        ImgLayout,
                                                               VkSampler            Sampler,
                                                               uint32_t             Binding,
                                                               uint32_t             Count ) {
    if ( DescSet.pDescPool->GetAvailableDescCount( VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ) < Count ) {
        apemode_halt( "Reserve more." );
        return false;
    }

    if ( !SetGraphicsNode( *DescSet.pNode ) ) {
        return false;
    }

    uintptr_t ImgInfoIdx = ImgInfos.size( );
    auto&     Write      = apemodevk::PushBackAndGet( Writes );
    auto&     ImgInfo    = apemodevk::PushBackAndGet( ImgInfos );

    ImgInfo.sampler     = Sampler;
    ImgInfo.imageView   = ImgView;
    ImgInfo.imageLayout = ImgLayout;

    Write                 = TInfoStruct< VkWriteDescriptorSet >( );
    Write.dstSet          = DescSet;
    Write.dstBinding      = Binding;
    Write.descriptorCount = Count;
    Write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    Write.pImageInfo      = reinterpret_cast< const VkDescriptorImageInfo* >( ImgInfoIdx );

    return true;
}

void apemodevk::DescriptorSetUpdater::Flush( ) {
    for ( auto& Write : Writes ) {
        if ( Write.pBufferInfo ) {
            uintptr_t BufferInfoIdx = reinterpret_cast< const uintptr_t >( Write.pBufferInfo );
            Write.pBufferInfo       = &BufferInfos[ BufferInfoIdx ];
        } else if ( Write.pImageInfo ) {
            uintptr_t ImgInfoIdx = reinterpret_cast< const uintptr_t >( Write.pImageInfo );
            Write.pImageInfo     = &ImgInfos[ ImgInfoIdx ];
        }
    }

    apemode_assert( pNode != nullptr, "No writes or copies were submitted." );

    if ( apemode_likely( pNode != nullptr ) ) {
        vkUpdateDescriptorSets( *pNode,
                                _Get_collection_length_u( Writes ),
                                Writes.empty( ) ? nullptr : Writes.data( ),
                                _Get_collection_length_u( Copies ),
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

    vkUpdateDescriptorSets( pLogicalDevice, TempWrites.size( ), TempWrites.data( ), 0, nullptr );
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
