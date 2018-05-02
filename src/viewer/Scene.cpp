
#include "Scene.h"
#include <AppState.h>
#include <MemoryManager.h>

#ifndef PI
#define PI 3.14159265358979323846264338327950288
#endif

using namespace apemode;

uint32_t MaterialPropertyGetIndex( uint32_t packed ) {
    const uint32_t valueIndex = ( packed >> 8 ) & 0x0fff;
    return valueIndex;
}

apemodefb::EValueTypeFb MaterialPropertyGetType( uint32_t packed ) {
    const uint32_t valueType = packed & 0x000f;
    return apemodefb::EValueTypeFb( valueType );
}

template < typename T >
bool FlatbuffersTVectorIsNotNullAndNotEmptyAssert( const flatbuffers::Vector< T > *pVector ) {
    assert( pVector && pVector->size( ) );
    return pVector && pVector->size( );
}

template < typename T >
typename flatbuffers::Vector< T >::return_type FlatbuffersTVectorGetAtIndex( const flatbuffers::Vector< T > *pVector,
                                                                             const size_t                    itemIndex ) {
    assert( pVector );
    const size_t vectorSize = pVector->size( );

    assert( vectorSize > itemIndex );
    return pVector->operator[]( static_cast< flatbuffers::uoffset_t >( itemIndex ) );
}

std::string GetStringProperty( const apemodefb::SceneFb *pScene, uint32_t valueId ) {
    assert( pScene );
    assert( apemodefb::EValueTypeFb_String == MaterialPropertyGetType( valueId ) );

    const uint32_t valueIndex = MaterialPropertyGetIndex( valueId );
    return FlatbuffersTVectorGetAtIndex( pScene->string_values( ), valueIndex )->str( );
}

const char *GetCStringProperty( const apemodefb::SceneFb *pScene, uint32_t valueId ) {
    assert( pScene );
    assert( apemodefb::EValueTypeFb_String == MaterialPropertyGetType( valueId ) );

    const uint32_t valueIndex = MaterialPropertyGetIndex( valueId );
    return FlatbuffersTVectorGetAtIndex( pScene->string_values( ), valueIndex )->c_str( );
}

bool GetBoolProperty( const apemodefb::SceneFb *pScene, uint32_t valueId ) {
    assert( pScene );
    assert( apemodefb::EValueTypeFb_Bool == MaterialPropertyGetType( valueId ) );

    const uint32_t valueIndex = MaterialPropertyGetIndex( valueId );
    return FlatbuffersTVectorGetAtIndex( pScene->bool_values( ), valueIndex );
}

float GetScalarProperty( const apemodefb::SceneFb *pScene, uint32_t valueId ) {
    assert( pScene );
    assert( apemodefb::EValueTypeFb_Float == MaterialPropertyGetType( valueId ) );

    const uint32_t valueIndex = MaterialPropertyGetIndex( valueId );
    return FlatbuffersTVectorGetAtIndex( pScene->float_values( ), valueIndex );
}

XMFLOAT2 GetVec2Property( const apemodefb::SceneFb *pScene, uint32_t valueId ) {
    assert( pScene );

    const auto valueType = MaterialPropertyGetType( valueId );
    assert( apemodefb::EValueTypeFb_Float2 == valueType );

    const uint32_t valueIndex = MaterialPropertyGetIndex( valueId );
    return XMFLOAT2( FlatbuffersTVectorGetAtIndex( pScene->float_values( ), valueIndex ),
                     FlatbuffersTVectorGetAtIndex( pScene->float_values( ), valueIndex + 1 ) );
}

XMFLOAT3 GetVec3Property( const apemodefb::SceneFb *pScene, uint32_t valueId ) {
    assert( pScene );

    const auto valueType = MaterialPropertyGetType( valueId );
    assert( apemodefb::EValueTypeFb_Float3 == valueType );

    const uint32_t valueIndex = MaterialPropertyGetIndex( valueId );
    return XMFLOAT3( FlatbuffersTVectorGetAtIndex( pScene->float_values( ), valueIndex ),
                     FlatbuffersTVectorGetAtIndex( pScene->float_values( ), valueIndex + 1 ),
                     FlatbuffersTVectorGetAtIndex( pScene->float_values( ), valueIndex + 2 ) );
}

XMFLOAT4 GetVec4Property( const apemodefb::SceneFb *pScene, uint32_t valueId, float defaultW = 1.0f ) {
    assert( pScene );

    const auto valueType = MaterialPropertyGetType( valueId );
    assert( apemodefb::EValueTypeFb_Float3 == valueType || apemodefb::EValueTypeFb_Float4 == valueType );

    const uint32_t valueIndex = MaterialPropertyGetIndex( valueId );
    return XMFLOAT4( FlatbuffersTVectorGetAtIndex( pScene->float_values( ), valueIndex ),
                     FlatbuffersTVectorGetAtIndex( pScene->float_values( ), valueIndex + 1 ),
                     FlatbuffersTVectorGetAtIndex( pScene->float_values( ), valueIndex + 2 ),
                     apemodefb::EValueTypeFb_Float4 == valueType
                         ? FlatbuffersTVectorGetAtIndex( pScene->float_values( ), valueIndex + 3 )
                         : defaultW );
}

bool apemode::SceneNodeTransform::Validate( ) const {
    return false ==
           ( isnan( Translation.x ) || isnan( Translation.y ) || isnan( Translation.z ) || isnan( RotationOffset.x ) ||
             isnan( RotationOffset.y ) || isnan( RotationOffset.z ) || isnan( RotationPivot.x ) || isnan( RotationPivot.y ) ||
             isnan( RotationPivot.z ) || isnan( PreRotation.x ) || isnan( PreRotation.y ) || isnan( PreRotation.z ) ||
             isnan( PostRotation.x ) || isnan( PostRotation.y ) || isnan( PostRotation.z ) || isnan( Rotation.x ) ||
             isnan( Rotation.y ) || isnan( Rotation.z ) || isnan( ScalingOffset.x ) || isnan( ScalingOffset.y ) ||
             isnan( ScalingOffset.z ) || isnan( ScalingPivot.x ) || isnan( ScalingPivot.y ) || isnan( ScalingPivot.z ) ||
             isnan( Scaling.x ) || isnan( Scaling.y ) || isnan( Scaling.z ) || isnan( GeometricTranslation.x ) ||
             isnan( GeometricTranslation.y ) || isnan( GeometricTranslation.z ) || isnan( GeometricRotation.x ) ||
             isnan( GeometricRotation.y ) || isnan( GeometricRotation.z ) || isnan( GeometricScaling.x ) ||
             isnan( GeometricScaling.y ) || isnan( GeometricScaling.z ) || Scaling.x == 0 || Scaling.y == 0 || Scaling.z == 0 ||
             GeometricScaling.x == 0 || GeometricScaling.y == 0 || GeometricScaling.z == 0 );
}

XMMATRIX XMMatrixRotationZYX( const XMFLOAT3 *v ) {
    return XMMatrixRotationX( v->x ) *
           XMMatrixRotationY( v->y ) *
           XMMatrixRotationZ( v->z );
}

XMMATRIX apemode::SceneNodeTransform::CalculateLocalMatrix( ) const {
    return XMMatrixTranslationFromVector( XMVectorNegate( XMLoadFloat3( &ScalingPivot ) ) ) *
           XMMatrixScalingFromVector( XMLoadFloat3( &Scaling ) ) *
           XMMatrixTranslationFromVector( XMLoadFloat3( &ScalingPivot ) ) *
           XMMatrixTranslationFromVector( XMLoadFloat3( &ScalingOffset ) ) *
           XMMatrixTranslationFromVector( XMVectorNegate( XMLoadFloat3( &RotationPivot ) ) ) *
           XMMatrixRotationZYX( &PostRotation ) *
           XMMatrixRotationZYX( &Rotation ) *
           XMMatrixRotationZYX( &PreRotation ) *
           XMMatrixTranslationFromVector( XMLoadFloat3( &RotationPivot ) ) *
           XMMatrixTranslationFromVector( XMLoadFloat3( &RotationOffset ) ) *
           XMMatrixTranslationFromVector( XMLoadFloat3( &Translation ) );
}

XMMATRIX apemode::SceneNodeTransform::CalculateGeometricMatrix( ) const {
    return XMMatrixScalingFromVector( XMLoadFloat3( &GeometricScaling ) ) *
           XMMatrixRotationZYX( &GeometricRotation ) *
           XMMatrixTranslationFromVector( XMLoadFloat3( &GeometricTranslation ) );
}

void apemode::Scene::UpdateChildWorldMatrices( const uint32_t nodeId ) {

    for ( const auto childId : Nodes[ nodeId ].ChildIds ) {

        LocalMatrices[ childId ]        = Transforms[ childId ].CalculateLocalMatrix( );
        GeometricalMatrices[ childId ]  = Transforms[ childId ].CalculateGeometricMatrix( );
        HierarchicalMatrices[ childId ] = LocalMatrices[ childId ] * HierarchicalMatrices[ Nodes[ childId ].ParentId ];
        WorldMatrices[ childId ]        = GeometricalMatrices[ childId ] * HierarchicalMatrices[ childId ];

        if ( false == Nodes[ childId ].ChildIds.empty( ) )
            UpdateChildWorldMatrices( childId );
    }
}

void apemode::Scene::UpdateMatrices( ) {
    if ( Transforms.empty( ) || Nodes.empty( ) )
        return;

    //
    // Resize matrices storage if needed
    //

    if ( LocalMatrices.size( ) < Transforms.size( ) ) {
        LocalMatrices.resize( Transforms.size( ) );
        WorldMatrices.resize( Transforms.size( ) );
        GeometricalMatrices.resize( Transforms.size( ) );
        HierarchicalMatrices.resize( Transforms.size( ) );
    }

    //
    // Implicit world calculations for the root node.
    // Start recursive updates from the root node.
    //

    LocalMatrices[ 0 ]        = Transforms[ 0 ].CalculateLocalMatrix( );
    GeometricalMatrices[ 0 ]  = Transforms[ 0 ].CalculateGeometricMatrix( );
    HierarchicalMatrices[ 0 ] = LocalMatrices[ 0 ];
    WorldMatrices[ 0 ]        = GeometricalMatrices[ 0 ] * LocalMatrices[ 0 ];

    UpdateChildWorldMatrices( 0 );
}

apemode::UniqueScenePtrPair apemode::LoadSceneFromBin( const uint8_t *pData, size_t dataSize ) {

    const apemodefb::SceneFb * pSrcScene = apemodefb::GetSceneFb( pData );
    if ( nullptr == pSrcScene ) {
        LogError( "Failed to reinterpret the buffer to scene." );

        return UniqueScenePtrPair( nullptr, nullptr );
    }

    if ( apemodefb::EVersionFb_Value != pSrcScene->version( ) ) {
        LogError( "Library version \"{}\" does not match to file version \"{}\"",
                  apemodefb::EVersionFb_Value,
                  pSrcScene->version( ) );

        return UniqueScenePtrPair( nullptr, nullptr );
    }

    std::unique_ptr< Scene > pScene( new Scene( ) );
    std::set< uint32_t >     meshIds;
    std::set< uint32_t >     materialIds;

    if ( auto nodesFb = pSrcScene->nodes( ) ) {
        pScene->Nodes.resize( nodesFb->size( ) );
        pScene->Transforms.resize( nodesFb->size( ) );

        const float toRadsFactor = float( PI ) / 180.0f;

        for ( auto nodeFb : *nodesFb ) {
            assert( nodeFb );

            auto &node = pScene->Nodes[ nodeFb->id( ) ];

            node.Id = nodeFb->id( );
            node.MeshId = nodeFb->mesh_id( );

            LogInfo( "Processing node: {}", GetStringProperty( pSrcScene, nodeFb->name_id( ) ).c_str( ) );

            if ( nodeFb->child_ids( ) && nodeFb->child_ids( )->size( ) )
                node.ChildIds.reserve( nodeFb->child_ids( )->size( ) );

            auto childIdsIt = nodeFb->child_ids( )->data( );
            auto childIdsEndIt = childIdsIt + nodeFb->child_ids( )->size( );
            std::transform( childIdsIt, childIdsEndIt, std::back_inserter( node.ChildIds ), [&]( uint32_t id ) {
                auto &childNode = pScene->Nodes[ id ];
                childNode.ParentId = node.Id;
                return id;
            } );

            if ( nodeFb->mesh_id( ) != uint32_t( -1 ) ) {
                meshIds.insert( nodeFb->mesh_id( ) );
            }

            auto &transform = pScene->Transforms[ nodeFb->id( ) ];
            auto transformFb = ( *pSrcScene->transforms( ) )[ nodeFb->id( ) ];

            transform.Translation.x          = ( transformFb->translation( ).x( ) );
            transform.Translation.y          = ( transformFb->translation( ).y( ) );
            transform.Translation.z          = ( transformFb->translation( ).z( ) );
            transform.RotationOffset.x       = ( transformFb->rotation_offset( ).x( ) );
            transform.RotationOffset.y       = ( transformFb->rotation_offset( ).y( ) );
            transform.RotationOffset.z       = ( transformFb->rotation_offset( ).z( ) );
            transform.RotationPivot.x        = ( transformFb->rotation_pivot( ).x( ) );
            transform.RotationPivot.y        = ( transformFb->rotation_pivot( ).y( ) );
            transform.RotationPivot.z        = ( transformFb->rotation_pivot( ).z( ) );
            transform.PreRotation.x          = ( transformFb->pre_rotation( ).x( ) * toRadsFactor );
            transform.PreRotation.y          = ( transformFb->pre_rotation( ).y( ) * toRadsFactor );
            transform.PreRotation.z          = ( transformFb->pre_rotation( ).z( ) * toRadsFactor );
            transform.Rotation.x             = ( transformFb->rotation( ).x( ) * toRadsFactor );
            transform.Rotation.y             = ( transformFb->rotation( ).y( ) * toRadsFactor );
            transform.Rotation.z             = ( transformFb->rotation( ).z( ) * toRadsFactor );
            transform.PostRotation.x         = ( transformFb->post_rotation( ).x( ) * toRadsFactor );
            transform.PostRotation.y         = ( transformFb->post_rotation( ).y( ) * toRadsFactor );
            transform.PostRotation.z         = ( transformFb->post_rotation( ).z( ) * toRadsFactor );
            transform.ScalingOffset.x        = ( transformFb->scaling_offset( ).x( ) );
            transform.ScalingOffset.y        = ( transformFb->scaling_offset( ).y( ) );
            transform.ScalingOffset.z        = ( transformFb->scaling_offset( ).z( ) );
            transform.ScalingPivot.x         = ( transformFb->scaling_pivot( ).x( ) );
            transform.ScalingPivot.y         = ( transformFb->scaling_pivot( ).y( ) );
            transform.ScalingPivot.z         = ( transformFb->scaling_pivot( ).z( ) );
            transform.Scaling.x              = ( transformFb->scaling( ).x( ) );
            transform.Scaling.y              = ( transformFb->scaling( ).y( ) );
            transform.Scaling.z              = ( transformFb->scaling( ).z( ) );
            transform.GeometricTranslation.x = ( transformFb->geometric_translation( ).x( ) );
            transform.GeometricTranslation.y = ( transformFb->geometric_translation( ).y( ) );
            transform.GeometricTranslation.z = ( transformFb->geometric_translation( ).z( ) );
            transform.GeometricRotation.x    = ( transformFb->geometric_rotation( ).x( ) * toRadsFactor );
            transform.GeometricRotation.y    = ( transformFb->geometric_rotation( ).y( ) * toRadsFactor );
            transform.GeometricRotation.z    = ( transformFb->geometric_rotation( ).z( ) * toRadsFactor );
            transform.GeometricScaling.x     = ( transformFb->geometric_scaling( ).x( ) );
            transform.GeometricScaling.y     = ( transformFb->geometric_scaling( ).y( ) );
            transform.GeometricScaling.z     = ( transformFb->geometric_scaling( ).z( ) );

            const bool bValidTransform = transform.Validate( );
            if ( !bValidTransform ) {
                LogError( "Found invalid transform, node id {}", nodeFb->id( ) );
            }
        }

        pScene->UpdateMatrices( );
    }

    if ( auto pMeshesFb = pSrcScene->meshes( ) ) {
        pScene->Meshes.reserve( meshIds.size( ) );

        {   /* All the subsets are stored in Scene instance, and can be referenced
             * by the BaseSubset and SubsetCount values in SceneMeshSubset struct.
             */

            size_t totalSubsetCount = 0;
            for ( auto meshId : meshIds ) {
                auto pMeshFb = FlatbuffersTVectorGetAtIndex( pMeshesFb, meshId );
                totalSubsetCount += pMeshFb->subsets( ) ? pMeshFb->subsets( )->size( ) : 0;
            }

            pScene->Subsets.reserve( totalSubsetCount );
        }

        for ( auto meshId : meshIds ) {
            auto pMeshFb = FlatbuffersTVectorGetAtIndex( pMeshesFb, meshId );

            assert( pMeshFb );
            if ( pMeshFb &&
                 FlatbuffersTVectorIsNotNullAndNotEmptyAssert( pMeshFb->vertices( ) ) &&
                 FlatbuffersTVectorIsNotNullAndNotEmptyAssert( pMeshFb->indices( ) ) &&
                 FlatbuffersTVectorIsNotNullAndNotEmptyAssert( pMeshFb->subsets( ) ) &&
                 FlatbuffersTVectorIsNotNullAndNotEmptyAssert( pMeshFb->submeshes( ) ) ) {

                pScene->Meshes.emplace_back( );
                auto &mesh = pScene->Meshes.back( );

                auto submeshesFb = pMeshFb->submeshes( );
                auto submeshFb   = submeshesFb->Get( 0 );

                mesh.PositionOffset.x = ( submeshFb->position_offset( ).x( ) );
                mesh.PositionOffset.y = ( submeshFb->position_offset( ).y( ) );
                mesh.PositionOffset.z = ( submeshFb->position_offset( ).z( ) );
                mesh.PositionScale.x  = ( submeshFb->position_scale( ).x( ) );
                mesh.PositionScale.y  = ( submeshFb->position_scale( ).y( ) );
                mesh.PositionScale.z  = ( submeshFb->position_scale( ).z( ) );
                mesh.TexcoordOffset.x = ( submeshFb->uv_offset( ).x( ) );
                mesh.TexcoordOffset.y = ( submeshFb->uv_offset( ).y( ) );
                mesh.TexcoordScale.x  = ( submeshFb->uv_scale( ).x( ) );
                mesh.TexcoordScale.y  = ( submeshFb->uv_scale( ).y( ) );

                mesh.SubsetCount = static_cast< uint32_t >( pMeshFb->subsets( )->size( ) );
                mesh.BaseSubset  = static_cast< uint32_t >( pScene->Subsets.size( ) );

                // clang-format off
                std::for_each( pMeshFb->subsets( )->begin( ),
                               pMeshFb->subsets( )->end( ),
                               [&]( const apemodefb::SubsetFb *pSubsetFb ) {
                                   if ( pSubsetFb && ( uint32_t( -1 ) != pSubsetFb->material_id( ) ) ) {
                                       materialIds.insert( pSubsetFb->material_id( ) );
                                   }
                               } );
                // clang-format on

                std::transform( pMeshFb->subsets( )->begin( ),
                                pMeshFb->subsets( )->end( ),
                                std::back_inserter( pScene->Subsets ),
                                [&]( const apemodefb::SubsetFb *pSubsetFb ) {

                                    SceneMeshSubset subset;
                                    subset.MaterialId = pSubsetFb->material_id( );
                                    subset.BaseIndex  = pSubsetFb->base_index( );
                                    subset.IndexCount = pSubsetFb->index_count( );

                                    return subset;
                                } );
            }
        }
    }

    if ( auto pMaterialsFb = pSrcScene->materials( ) ) {
        pScene->Materials.reserve( materialIds.size( ) );

        for ( auto materialId : materialIds ) {

            auto pMaterialFb = FlatbuffersTVectorGetAtIndex( pMaterialsFb, materialId );
            assert( pMaterialFb );

            LogInfo( "Processing material \"{}\": properties: {}, textures: {}",
                     GetCStringProperty( pSrcScene, pMaterialFb->name_id( ) ),
                     pMaterialFb->properties( ) ? pMaterialFb->properties( )->size( ) : 0,
                     pMaterialFb->texture_properties( ) ? pMaterialFb->texture_properties( )->size( ) : 0 );

            pScene->Materials.emplace_back( );
            auto &material = pScene->Materials.back( );

            if ( auto pPropertiesFb = pMaterialFb->properties( ) ) {
                for ( auto pMaterialPropFb : *pPropertiesFb ) {

                    auto pszMaterialPropName = GetCStringProperty( pSrcScene, pMaterialPropFb->name_id( ) );
                    LogInfo( "Loading property: \"{}\"", pszMaterialPropName );

                    if ( strcmp( "baseColorFactor", GetCStringProperty( pSrcScene, pMaterialPropFb->name_id( ) ) ) == 0 ) {
                        material.BaseColorFactor = GetVec4Property( pSrcScene, pMaterialPropFb->value_id( ) );
                    } else if ( strcmp( "metallicFactor", GetCStringProperty( pSrcScene, pMaterialPropFb->name_id( ) ) ) == 0 ) {
                        material.MetallicFactor = GetScalarProperty( pSrcScene, pMaterialPropFb->value_id( ) );
                    } else if ( strcmp( "emissiveFactor", GetCStringProperty( pSrcScene, pMaterialPropFb->name_id( ) ) ) == 0 ) {
                        material.EmissiveFactor = GetVec3Property( pSrcScene, pMaterialPropFb->value_id( ) );
                    } else if ( strcmp( "roughnessFactor", GetCStringProperty( pSrcScene, pMaterialPropFb->name_id( ) ) ) == 0 ) {
                        material.RoughnessFactor = GetScalarProperty( pSrcScene, pMaterialPropFb->value_id( ) );
                    } else if ( strcmp( "doubleSided", GetCStringProperty( pSrcScene, pMaterialPropFb->name_id( ) ) ) == 0 ) {
                        material.bDoubleSided = GetBoolProperty( pSrcScene, pMaterialPropFb->value_id( ) );
                    } else {
                        LogError( "Failed to map the property to the available slot" );
                    }

                } /* pMaterialPropFb */
            }     /* pPropertiesFb */

            if ( auto pTexturesFb = pSrcScene->textures( ) ) {
                if ( auto pFilesFb = pSrcScene->files( ) ) {
                    if ( auto pTexturePropertiesFb = pMaterialFb->texture_properties( ) ) {
                        for ( auto pTexturePropFb : *pTexturePropertiesFb ) {
                            if ( auto pTextureFb = FlatbuffersTVectorGetAtIndex( pTexturesFb, pTexturePropFb->value_id( ) ) ) {
                                auto pFileFb = FlatbuffersTVectorGetAtIndex( pFilesFb, pTextureFb->file_id( ) );
                                assert( pFileFb );

                                auto pszFileName        = GetCStringProperty( pSrcScene, pFileFb->name_id( ) );
                                auto pszTextureName     = GetCStringProperty( pSrcScene, pTextureFb->name_id( ) );
                                auto pszTexturePropName = GetCStringProperty( pSrcScene, pTexturePropFb->name_id( ) );

                                LogInfo( "Loading texture: \"{}\", name: {}", pszTexturePropName, pszTextureName );

                                if ( strcmp( "baseColorTexture", pszTexturePropName ) == 0 ) {
                                    material.BaseColorImgAsset.AssetId     = pTextureFb->file_id( );
                                    material.BaseColorImgAsset.pBufferData = pFileFb->buffer( )->data( );
                                    material.BaseColorImgAsset.BufferSize  = pFileFb->buffer( )->size( );
                                } else if ( strcmp( "normalTexture", pszTexturePropName ) == 0 ) {
                                    material.NormalImgAsset.AssetId     = pTextureFb->file_id( );
                                    material.NormalImgAsset.pBufferData = pFileFb->buffer( )->data( );
                                    material.NormalImgAsset.BufferSize  = pFileFb->buffer( )->size( );
                                } else if ( strcmp( "occlusionTexture", pszTexturePropName ) == 0 ) {
                                    material.OcclusionImgAsset.AssetId     = pTextureFb->file_id( );
                                    material.OcclusionImgAsset.pBufferData = pFileFb->buffer( )->data( );
                                    material.OcclusionImgAsset.BufferSize  = pFileFb->buffer( )->size( );
                                } else if ( strcmp( "metallicRoughnessTexture", pszTexturePropName ) == 0 ) {
                                    material.MetallicRoughnessImgAsset.AssetId     = pTextureFb->file_id( );
                                    material.MetallicRoughnessImgAsset.pBufferData = pFileFb->buffer( )->data( );
                                    material.MetallicRoughnessImgAsset.BufferSize  = pFileFb->buffer( )->size( );
                                } else if ( strcmp( "emissiveTexture", pszTexturePropName ) == 0 ) {
                                    material.EmissiveImgAsset.AssetId     = pTextureFb->file_id( );
                                    material.EmissiveImgAsset.pBufferData = pFileFb->buffer( )->data( );
                                    material.EmissiveImgAsset.BufferSize  = pFileFb->buffer( )->size( );
                                } else {
                                    LogError( "Failed to map the texture to the available slot" );
                                }

                            } /* pTextureFb */
                        }     /* pTexturePropFb */
                    }         /* pTexturePropertiesFb */
                }             /* pFilesFb */
            }                 /* pTexturesFb */
        }                     /* pMaterialFb */
    }                         /* pMaterialsFb */

    return UniqueScenePtrPair( std::move( pScene ), pSrcScene );
}
