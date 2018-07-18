
#include "Scene.h"
#include <AppState.h>
#include <MemoryManager.h>

#ifndef PI
#define PI 3.14159265358979323846264338327950288
#endif

using namespace apemode;

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

    apemode::unique_ptr< Scene > pScene( apemode_new Scene( ) );
    std::set< uint32_t >         meshIds;
    std::set< uint32_t >         materialIds;

    auto pNodesFb = pSrcScene->nodes( );
    if ( FlatbuffersTVectorIsNotNullAndNotEmptyAssert( pNodesFb ) ) {

        pScene->Nodes.resize( pNodesFb->size( ) );
        pScene->Transforms.resize( pNodesFb->size( ) );

        for ( auto pNodeFb : *pNodesFb ) {
            assert( pNodeFb );

            auto &node = pScene->Nodes[ pNodeFb->id( ) ];
            node.Id = pNodeFb->id( );

            if ( pNodeFb->mesh_id( ) != kInvalidId ) {
                node.MeshId = pNodeFb->mesh_id( );
                meshIds.insert( pNodeFb->mesh_id( ) );
            }

            LogInfo( "Processing node: {}", GetStringProperty( pSrcScene, pNodeFb->name_id( ) ).c_str( ) );

            if ( pNodeFb->child_ids( ) && pNodeFb->child_ids( )->size( ) )
                node.ChildIds.reserve( pNodeFb->child_ids( )->size( ) );

            auto childIdsIt = pNodeFb->child_ids( )->data( );
            auto childIdsEndIt = childIdsIt + pNodeFb->child_ids( )->size( );
            std::transform( childIdsIt, childIdsEndIt, std::back_inserter( node.ChildIds ), [&]( uint32_t id ) {
                auto &childNode = pScene->Nodes[ id ];
                childNode.ParentId = node.Id;
                return id;
            } );

            auto & transform = pScene->Transforms[ pNodeFb->id( ) ];
            auto transformFb = ( *pSrcScene->transforms( ) )[ pNodeFb->id( ) ];

            constexpr float toRadsFactor = float( PI ) / 180.0f;

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
                LogError( "Found invalid transform, node id {}", pNodeFb->id( ) );
            }

            auto pAnimCurveIdsFb = pNodeFb->anim_curve_ids( );
            if ( FlatbuffersTVectorIsNotNullAndNotEmptyAssert( pAnimCurveIdsFb ) ) {
                node.AnimCurveIds.resize( pAnimCurveIdsFb->size( ) );
                const size_t byteSize = pAnimCurveIdsFb->size( ) * sizeof( uint32_t );
                memcpy( node.AnimCurveIds.data( ), pAnimCurveIdsFb->data( ), byteSize );
            }
        }

        pScene->UpdateMatrices( );
    }

    auto pAnimCurvesFb = pSrcScene->anim_curves( );
    if ( FlatbuffersTVectorIsNotNullAndNotEmptyAssert( pAnimCurvesFb ) ) {
        pScene->AnimCurves.reserve( pAnimCurvesFb->size( ) );

        for ( auto pAnimCurveFb : *pAnimCurvesFb ) {
            assert( pAnimCurveFb );

            LogInfo( "Processing curve: #{} -> {} {}",
                     pAnimCurveFb->id( ),
                     GetStringProperty( pSrcScene, pAnimCurveFb->name_id( ) ).c_str( ),
                     pAnimCurveFb->keys( )->size( ) );

            pScene->AnimCurves.emplace_back( );
            auto &animCurve = pScene->AnimCurves.back( );

            animCurve.AnimLayerId = pAnimCurveFb->anim_stack_id( );
            animCurve.AnimStackId = pAnimCurveFb->anim_stack_id( );
            animCurve.eChannel    = SceneAnimCurve::EChannel( pAnimCurveFb->channel( ) );
            animCurve.eProp       = SceneAnimCurve::EProperty( pAnimCurveFb->property( ) );

            assert( FlatbuffersTVectorIsNotNullAndNotEmptyAssert( pAnimCurveFb->keys( ) ) );
            animCurve.Keys.reserve( pAnimCurveFb->keys( )->size( ) );

            std::transform( pAnimCurveFb->keys( )->begin( ),
                            pAnimCurveFb->keys( )->end( ),
                            std::back_inserter( animCurve.Keys ),
                            [&]( const apemodefb::AnimCurveKeyFb *pKeyFb ) {
                                assert( pKeyFb );

                                apemodexm::XMFLOAT2 key;
                                key.y = pKeyFb->time( );
                                key.x = pKeyFb->value( );
                                return key;
                            } );
        }
    }

    auto pMeshesFb = pSrcScene->meshes( );
    if ( FlatbuffersTVectorIsNotNullAndNotEmptyAssert( pMeshesFb ) ) {
        { /* All the subsets are stored in Scene instance, and can be referenced
           * by the BaseSubset and SubsetCount values in SceneMeshSubset struct.
           */

            size_t totalSubsetCount = 0;
            for ( auto meshId : meshIds ) {
                auto pMeshFb = FlatbuffersTVectorGetAtIndex( pMeshesFb, meshId );
                totalSubsetCount += pMeshFb->subsets( ) ? pMeshFb->subsets( )->size( ) : 0;
            }

            pScene->Subsets.reserve( totalSubsetCount );
        }

        pScene->Meshes.reserve( meshIds.size( ) );
        for ( auto meshId : meshIds ) {
            auto pMeshFb = FlatbuffersTVectorGetAtIndex( pMeshesFb, meshId );

            assert( pMeshFb );
            assert( FlatbuffersTVectorIsNotNullAndNotEmptyAssert( pMeshFb->vertices( ) ) );
            assert( FlatbuffersTVectorIsNotNullAndNotEmptyAssert( pMeshFb->indices( ) ) );
            assert( FlatbuffersTVectorIsNotNullAndNotEmptyAssert( pMeshFb->subsets( ) ) );
            assert( FlatbuffersTVectorIsNotNullAndNotEmptyAssert( pMeshFb->submeshes( ) ) );

            pScene->Meshes.emplace_back( );
            auto &mesh = pScene->Meshes.back( );
            mesh.SrcId = meshId;

            auto pSubmeshesFb = pMeshFb->submeshes( );
            auto pSubmeshFb   = pSubmeshesFb->Get( 0 );

            mesh.PositionOffset.x = ( pSubmeshFb->position_offset( ).x( ) );
            mesh.PositionOffset.y = ( pSubmeshFb->position_offset( ).y( ) );
            mesh.PositionOffset.z = ( pSubmeshFb->position_offset( ).z( ) );
            mesh.PositionScale.x  = ( pSubmeshFb->position_scale( ).x( ) );
            mesh.PositionScale.y  = ( pSubmeshFb->position_scale( ).y( ) );
            mesh.PositionScale.z  = ( pSubmeshFb->position_scale( ).z( ) );
            mesh.TexcoordOffset.x = ( pSubmeshFb->uv_offset( ).x( ) );
            mesh.TexcoordOffset.y = ( pSubmeshFb->uv_offset( ).y( ) );
            mesh.TexcoordScale.x  = ( pSubmeshFb->uv_scale( ).x( ) );
            mesh.TexcoordScale.y  = ( pSubmeshFb->uv_scale( ).y( ) );

            mesh.SubsetCount = static_cast< uint32_t >( pMeshFb->subsets( )->size( ) );
            mesh.BaseSubset  = static_cast< uint32_t >( pScene->Subsets.size( ) );

            // clang-format off
            std::for_each( pMeshFb->subsets( )->begin( ),
                           pMeshFb->subsets( )->end( ),
                           [&]( const apemodefb::SubsetFb *pSubsetFb ) {
                               if ( pSubsetFb && ( kInvalidId != pSubsetFb->material_id( ) ) ) {
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

            mesh.SkinId = pMeshFb->skin_id( );
        }
    }

    auto pSkinsFb = pSrcScene->skins( );
    if ( FlatbuffersTVectorIsNotNullAndNotEmptyAssert( pSkinsFb ) ) {
        pScene->Skins.resize( pSkinsFb->size() );

        for ( auto &mesh : pScene->Meshes ) {
            if ( mesh.SkinId != kInvalidId ) {
                auto &skin = pScene->Skins[ mesh.SkinId ];

                if ( auto pSkinFb = FlatbuffersTVectorGetAtIndex( pSkinsFb, mesh.SkinId ) ) {
                    auto pLinkIdsFb = pSkinFb->links_ids( );
                    if ( FlatbuffersTVectorIsNotNullAndNotEmptyAssert( pLinkIdsFb ) ) {

                        LogInfo( "Processing skin: {}: {}",
                                 GetStringProperty( pSrcScene, pSkinFb->name_id( ) ).c_str( ),
                                 pLinkIdsFb->size( ) );

                        skin.LinkIds.resize( pLinkIdsFb->size( ) );
                        const size_t byteSize = pLinkIdsFb->size() * sizeof( uint32_t );
                        memcpy( skin.LinkIds.data( ), pLinkIdsFb->data( ), byteSize );
                    }
                }
            }
        }
    }

    auto pTexturesFb = pSrcScene->textures( );
    auto pFilesFb    = pSrcScene->files( );

    std::map< uint32_t, uint32_t > materialIdRemap;

    if ( auto pMaterialsFb = pSrcScene->materials( ) ) {
        pScene->Materials.reserve( materialIds.size( ) );

#define APEMODE_SCENE_DUMP_MATERIAL_NAMES
#ifdef APEMODE_SCENE_DUMP_MATERIAL_NAMES
        for ( uint32_t materialId = 0; materialId < pMaterialsFb->size( ); ++materialId ) {
            auto pMaterialFb = FlatbuffersTVectorGetAtIndex( pMaterialsFb, materialId );
            assert( pMaterialFb );

            LogInfo( "Found material \"{}\": properties: {}, textures: {}",
                     GetCStringProperty( pSrcScene, pMaterialFb->name_id( ) ),
                     pMaterialFb->properties( ) ? pMaterialFb->properties( )->size( ) : 0,
                     pMaterialFb->texture_properties( ) ? pMaterialFb->texture_properties( )->size( ) : 0 );
        }
#endif

        for ( auto materialId : materialIds ) {

            auto pMaterialFb = FlatbuffersTVectorGetAtIndex( pMaterialsFb, materialId );
            assert( pMaterialFb );

            LogInfo( "Processing material \"{}\": properties: {}, textures: {}",
                     GetCStringProperty( pSrcScene, pMaterialFb->name_id( ) ),
                     pMaterialFb->properties( ) ? pMaterialFb->properties( )->size( ) : 0,
                     pMaterialFb->texture_properties( ) ? pMaterialFb->texture_properties( )->size( ) : 0 );

            materialIdRemap[ materialId ] = static_cast< uint32_t >( pScene->Materials.size( ) );

            pScene->Materials.emplace_back( );
            auto &material = pScene->Materials.back( );
            material.SrcId = materialId;

            for ( auto pTexturePropFb : *pMaterialFb->texture_properties( ) ) {
                auto pszTexturePropName = GetCStringProperty( pSrcScene, pTexturePropFb->name_id( ) );

                auto pTextureFb = FlatbuffersTVectorGetAtIndex( pTexturesFb, pTexturePropFb->value_id( ) );
                assert( pTextureFb );

                auto pFileFb = FlatbuffersTVectorGetAtIndex( pFilesFb, pTextureFb->file_id( ) );
                assert( pFileFb );

                auto pszFileName = GetCStringProperty( pSrcScene, pFileFb->name_id( ) );
                assert( pszFileName );

                LogInfo( "\tTexture: \"{}\" -> {}", pszTexturePropName, pszFileName );
            }

            if ( auto pPropertiesFb = pMaterialFb->properties( ) ) {
                for ( auto pMaterialPropFb : *pPropertiesFb ) {

                    auto pszMaterialPropName = GetCStringProperty( pSrcScene, pMaterialPropFb->name_id( ) );
                    LogInfo( "\tProperty: \"{}\"", pszMaterialPropName );

                    if ( strcmp( "diffuseFactor", pszMaterialPropName ) == 0 ) {
                        material.BaseColorFactor = GetVec4Property( pSrcScene, pMaterialPropFb->value_id( ) );
                    } else if ( strcmp( "baseColorFactor", pszMaterialPropName ) == 0 ) {
                        material.BaseColorFactor = GetVec4Property( pSrcScene, pMaterialPropFb->value_id( ) );
                    } else if ( strcmp( "metallicFactor", pszMaterialPropName ) == 0 ) {
                        material.MetallicFactor = GetScalarProperty( pSrcScene, pMaterialPropFb->value_id( ) );
                    } else if ( strcmp( "emissiveFactor", pszMaterialPropName ) == 0 ) {
                        material.EmissiveFactor = GetVec3Property( pSrcScene, pMaterialPropFb->value_id( ) );
                    } else  if ( strcmp( "specularFactor", pszMaterialPropName ) == 0 ) {
                        material.MetallicFactor = GetVec3Property( pSrcScene, pMaterialPropFb->value_id( ) ).x;
                    } else if ( strcmp( "glossinessFactor", pszMaterialPropName ) == 0 ) {
                        material.RoughnessFactor = 1.0f - GetScalarProperty( pSrcScene, pMaterialPropFb->value_id( ) );
                    } else if ( strcmp( "roughnessFactor", pszMaterialPropName ) == 0 ) {
                        material.RoughnessFactor = GetScalarProperty( pSrcScene, pMaterialPropFb->value_id( ) );
                    } else if ( strcmp( "doubleSided", pszMaterialPropName ) == 0 ) {
                        material.bDoubleSided = GetBoolProperty( pSrcScene, pMaterialPropFb->value_id( ) );
                    } else if ( strcmp( "alphaMode", pszMaterialPropName ) == 0 ) {
                        if ( strcmp( "BLEND", GetCStringProperty( pSrcScene, pMaterialPropFb->value_id( ) ) ) == 0 )
                            material.eAlphaMode = SceneMaterial::eAlphaMode_Blend;
                    } else {
                        LogError( "Failed to map the property to the available slot" );
                    }

                } /* pMaterialPropFb */
            }     /* pPropertiesFb */
        }         /* pMaterialFb */
    }             /* pMaterialsFb */

    for ( auto &subset : pScene->Subsets ) {
        subset.MaterialId = materialIdRemap[ subset.MaterialId ];
    }

    return UniqueScenePtrPair( std::move( pScene ), pSrcScene );
}
