
#include "Scene.h"
#include <AppState.h>
#include <MemoryManager.h>

#ifndef PI
#define PI 3.14159265358979323846264338327950288
#endif

using namespace apemode;

struct SceneAnimLayerId {
    union {
        struct {
            uint16_t AnimStackId;
            uint16_t AnimLayerId;
        };

        uint32_t AnimLayerCompositeId;
    };
};

struct SceneAnimNodeId {
    union {
        struct {
            uint32_t         NodeId;
            SceneAnimLayerId AnimLayerId;
        };

        uint64_t AnimNodeCompositeId;
    };
};

inline XMFLOAT4 ToVec4( apemodefb::Vec4Fb const v ) {
    return XMFLOAT4{v.x( ), v.y( ), v.z( ), v.w( )};
}

inline XMFLOAT3 ToVec3( apemodefb::Vec3Fb const v ) {
    return XMFLOAT3{v.x( ), v.y( ), v.z( )};
}

inline XMFLOAT2 ToVec2( apemodefb::Vec2Fb const v ) {
    return XMFLOAT2{v.x( ), v.y( )};
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

void apemode::Scene::InitializeTransformFrame( SceneNodeTransformFrame &t ) const {
    t.Transforms.resize( Nodes.size( ) );
}

const apemode::SceneNodeAnimCurveIds &apemode::Scene::GetAnimCurveIds( const uint32_t nodeId,
                                                                       const uint16_t animStackId,
                                                                       const uint16_t animLayerId ) const {
    SceneAnimNodeId animNodeCompositeId;
    animNodeCompositeId.NodeId = nodeId;
    animNodeCompositeId.AnimLayerId.AnimStackId = animStackId;
    animNodeCompositeId.AnimLayerId.AnimLayerId = animLayerId;

    const auto animCurveIdsIt = AnimNodeIdToAnimCurveIds.find( animNodeCompositeId.AnimNodeCompositeId );
    assert( AnimNodeIdToAnimCurveIds.end( ) != animCurveIdsIt );
    return animCurveIdsIt->second;
}

void apemode::Scene::UpdateSkinMatrices( const SceneSkin &              skin,
                                         const SceneNodeTransformFrame &animatedFrame,
                                         apemode::vector< XMMATRIX > &  skinMatrices ) {

    skinMatrices.resize( skin.Links.size( ) );
    for ( size_t i = 0; i < skin.Links.size( ); ++i ) {
        const SceneSkinLink & skinLink = skin.Links[ i ];
        const SceneNodeTransformComposite &skinLinkTransform = animatedFrame.Transforms[ skinLink.LinkId ];
        skinMatrices[ skinLink.LinkId ] = skinLink.InvBindPoseMatrix * skinLinkTransform.WorldMatrix;
    }
}

XMFLOAT3 *MapProperty( apemode::SceneAnimCurve::EProperty eProperty, apemode::SceneNodeTransform *pProperties ) {
    static_assert( sizeof( SceneNodeTransform ) == ( sizeof( XMFLOAT3 ) * SceneAnimCurve::ePropertyCount / 3 ), "Re-implement mapping with switch cases." );
    assert( ( ( eProperty % 3 ) == 0 ) && ( eProperty < SceneAnimCurve::ePropertyCount ) );
    return reinterpret_cast< XMFLOAT3 * >( pProperties ) + ( eProperty / 3 );
}

SceneNodeTransformFrame &apemode::Scene::UpdateTransformProperties( const float    time,
                                                                    const bool     bLoop,
                                                                    const uint16_t animStackId,
                                                                    const uint16_t animLayerId ) {
    auto &animTransformFrame = GetAnimatedTransformFrame( animStackId, animLayerId );

    assert( animTransformFrame.Transforms.size( ) == BindPoseFrame.Transforms.size( ) );
    const size_t transformFrameByteSize = sizeof( SceneNodeTransformComposite ) * animTransformFrame.Transforms.size( );
    memcpy( animTransformFrame.Transforms.data( ), BindPoseFrame.Transforms.data( ), transformFrameByteSize );

    for ( const auto &node : Nodes ) {

        const auto &animCurveIds = GetAnimCurveIds( node.Id, animStackId, animLayerId );
        auto & animTransformComposite = animTransformFrame.Transforms[ node.Id ];

        for ( uint32_t propertyIndex = 0; propertyIndex < SceneAnimCurve::ePropertyCount; propertyIndex += SceneAnimCurve::eChannelCount ) {
            XMFLOAT3 *pProperty = MapProperty( SceneAnimCurve::EProperty( propertyIndex ), &animTransformComposite.Properties );

            const uint32_t animCurveIdX = animCurveIds.AnimCurveIds[ propertyIndex + SceneAnimCurve::eChannel_X ];
            const uint32_t animCurveIdY = animCurveIds.AnimCurveIds[ propertyIndex + SceneAnimCurve::eChannel_Y ];
            const uint32_t animCurveIdZ = animCurveIds.AnimCurveIds[ propertyIndex + SceneAnimCurve::eChannel_Z ];

            const SceneAnimCurve *pAnimCurveX = ( animCurveIdX != detail::kInvalidId ) ? ( &AnimCurves[ animCurveIdX ] ) : nullptr;
            const SceneAnimCurve *pAnimCurveY = ( animCurveIdY != detail::kInvalidId ) ? ( &AnimCurves[ animCurveIdY ] ) : nullptr;
            const SceneAnimCurve *pAnimCurveZ = ( animCurveIdZ != detail::kInvalidId ) ? ( &AnimCurves[ animCurveIdZ ] ) : nullptr;

            pProperty->x = pAnimCurveX ? pAnimCurveX->Calculate( time, bLoop ) : pProperty->x;
            pProperty->y = pAnimCurveY ? pAnimCurveY->Calculate( time, bLoop ) : pProperty->y;
            pProperty->z = pAnimCurveZ ? pAnimCurveZ->Calculate( time, bLoop ) : pProperty->z;
        }
    }

    return animTransformFrame;
}

SceneNodeTransformFrame &apemode::Scene::GetBindPoseTransformFrame( ) {
    return BindPoseFrame;
}

const SceneNodeTransformFrame &apemode::Scene::GetBindPoseTransformFrame( ) const {
    return BindPoseFrame;
}

apemode::SceneNodeTransformFrame &apemode::Scene::GetAnimatedTransformFrame( const uint16_t animStackId,
                                                                     const uint16_t animLayerId ) {
    SceneAnimLayerId animLayerCompositeId;
    animLayerCompositeId.AnimStackId = animStackId;
    animLayerCompositeId.AnimLayerId = animLayerId;

    auto animTransformFrame = AnimLayerIdToTransformFrames.find( animLayerCompositeId.AnimLayerCompositeId );
    assert( AnimLayerIdToTransformFrames.end( ) != animTransformFrame );
    return animTransformFrame->second;
}

const apemode::SceneNodeTransformFrame &apemode::Scene::GetAnimatedTransformFrame( const uint16_t animStackId,
                                                                           const uint16_t animLayerId ) const {
    SceneAnimLayerId animLayerCompositeId;
    animLayerCompositeId.AnimStackId = animStackId;
    animLayerCompositeId.AnimLayerId = animLayerId;

    auto animTransformFrame = AnimLayerIdToTransformFrames.find( animLayerCompositeId.AnimLayerCompositeId );
    assert( AnimLayerIdToTransformFrames.end( ) != animTransformFrame );
    return animTransformFrame->second;
}

void apemode::Scene::UpdateTransformMatrices( const uint32_t nodeId, SceneNodeTransformFrame &t ) const {

    const SceneNode & parentNode = Nodes[ nodeId ];
    const SceneNodeTransformComposite &transformComposite = t.Transforms[ nodeId ];
    assert( nodeId == parentNode.Id );

    const auto childIdRange = NodeToChildIds.equal_range( nodeId );
    for ( auto childIdIt = childIdRange.first; childIdIt != childIdRange.second; ++childIdIt ) {

        const uint32_t childId = childIdIt->second;
        const SceneNode & childNode = Nodes[ childId ];
        SceneNodeTransformComposite &childTransformComposite = t.Transforms[ childId ];

        assert( nodeId == childNode.ParentId );

        childTransformComposite.LocalMatrix        = childTransformComposite.Properties.CalculateLocalMatrix( );
        childTransformComposite.GeometricalMatrix  = childTransformComposite.Properties.CalculateGeometricMatrix( );
        childTransformComposite.HierarchicalMatrix = childTransformComposite.LocalMatrix * transformComposite.HierarchicalMatrix;
        childTransformComposite.WorldMatrix        = childTransformComposite.GeometricalMatrix * childTransformComposite.HierarchicalMatrix;

        UpdateTransformMatrices( childId, t );
    }
}

void apemode::Scene::UpdateTransformMatrices( SceneNodeTransformFrame &t ) const {
    if ( t.Transforms.empty( ) || Nodes.empty( ) )
        return;

    //
    // Implicit world calculations for the root node.
    // Start recursive updates from the root node.
    //

    SceneNodeTransformComposite &rootTransformComposite = t.Transforms[ 0 ];

    rootTransformComposite.LocalMatrix        = rootTransformComposite.Properties.CalculateLocalMatrix( );
    rootTransformComposite.GeometricalMatrix  = rootTransformComposite.Properties.CalculateGeometricMatrix( );
    rootTransformComposite.HierarchicalMatrix = rootTransformComposite.LocalMatrix;
    rootTransformComposite.WorldMatrix        = rootTransformComposite.GeometricalMatrix * rootTransformComposite.LocalMatrix;

    UpdateTransformMatrices( 0, t );
}

apemode::LoadedScene apemode::LoadSceneFromBin( apemode::vector< uint8_t > && fileContents ) {

    const apemodefb::SceneFb *pSrcScene = !fileContents.empty( )
                                        ? apemodefb::GetSceneFb( fileContents.data( ) )
                                        : nullptr;

    if ( nullptr == pSrcScene ) {
        LogError( "Failed to reinterpret the buffer to scene." );
        return apemode::LoadedScene{};
    }

    if ( apemodefb::EVersionFb_Value != pSrcScene->version( ) ) {
        LogError( "Library version \"{}\" does not match to file version \"{}\"",
                  apemodefb::EVersionFb_Value,
                  pSrcScene->version( ) );
        return apemode::LoadedScene{};
    }

    auto pNodesFb      = pSrcScene->nodes( );
    auto pMeshesFb     = pSrcScene->meshes( );
    auto pMaterialsFb  = pSrcScene->materials( );
    auto pAnimCurvesFb = pSrcScene->anim_curves( );
    auto pAnimStacksFb = pSrcScene->anim_stacks( );
    auto pAnimLayersFb = pSrcScene->anim_layers( );
    auto pSkinsFb      = pSrcScene->skins( );

    apemode::unique_ptr< Scene > pScene( apemode_new Scene( ) );
    if ( FlatbuffersTVectorIsNotNullAndNotEmpty( pNodesFb ) ) {

        size_t nodeIdCount      = 0;
        size_t animCurveIdCount = 0;

        for ( auto pNodeFb : *pNodesFb ) {
            nodeIdCount += apemodefb::FlatbuffersTVectorIsNotNullAndNotEmpty( pNodeFb->child_ids( ) ) ? pNodeFb->child_ids( )->size( ) : 0;
            animCurveIdCount += apemodefb::FlatbuffersTVectorIsNotNullAndNotEmpty( pNodeFb->anim_curve_ids( ) ) ? pNodeFb->anim_curve_ids( )->size( ) : 0;
        }

        pScene->NodeToChildIds.reserve( nodeIdCount );
        pScene->NodeIdToAnimCurveIds.reserve( animCurveIdCount );
    }

    if ( apemodefb::FlatbuffersTVectorIsNotNullAndNotEmpty( pNodesFb ) ) {
        pScene->Nodes.resize( pNodesFb->size( ) );

        SceneNodeTransformFrame &bindPoseFrame = pScene->BindPoseFrame;
        pScene->InitializeTransformFrame( bindPoseFrame );

        for ( auto pNodeFb : *pNodesFb ) {
            assert( pNodeFb );

            auto &node = pScene->Nodes[ pNodeFb->id( ) ];
            node.Id = pNodeFb->id( );

            if ( pNodeFb->mesh_id( ) != detail::kInvalidId ) {
                node.MeshId = pNodeFb->mesh_id( );
            }

            LogInfo( "Processing node: {}", GetStringProperty( pSrcScene, pNodeFb->name_id( ) ).c_str( ) );

            auto pChildIdsFb = pNodeFb->child_ids( );
            if ( apemodefb::FlatbuffersTVectorIsNotNullAndNotEmpty( pChildIdsFb ) ) {
                for ( const uint32_t childNodeId : *pChildIdsFb ) {
                    pScene->NodeToChildIds.insert( eastl::make_pair( node.Id, childNodeId ) );
                    pScene->Nodes[ childNodeId ].ParentId = node.Id;
                }
            }

            auto &transformComposite = bindPoseFrame.Transforms[ pNodeFb->id( ) ];
            auto transformFb = pSrcScene->transforms( )->Get( pNodeFb->id( ) );

            constexpr float toRadsFactor = float( PI ) / 180.0f;

            transformComposite.Properties.Translation.x          = transformFb->translation( ).x( );
            transformComposite.Properties.Translation.y          = transformFb->translation( ).y( );
            transformComposite.Properties.Translation.z          = transformFb->translation( ).z( );
            transformComposite.Properties.RotationOffset.x       = transformFb->rotation_offset( ).x( );
            transformComposite.Properties.RotationOffset.y       = transformFb->rotation_offset( ).y( );
            transformComposite.Properties.RotationOffset.z       = transformFb->rotation_offset( ).z( );
            transformComposite.Properties.RotationPivot.x        = transformFb->rotation_pivot( ).x( );
            transformComposite.Properties.RotationPivot.y        = transformFb->rotation_pivot( ).y( );
            transformComposite.Properties.RotationPivot.z        = transformFb->rotation_pivot( ).z( );
            transformComposite.Properties.PreRotation.x          = transformFb->pre_rotation( ).x( ) * toRadsFactor;
            transformComposite.Properties.PreRotation.y          = transformFb->pre_rotation( ).y( ) * toRadsFactor;
            transformComposite.Properties.PreRotation.z          = transformFb->pre_rotation( ).z( ) * toRadsFactor;
            transformComposite.Properties.Rotation.x             = transformFb->rotation( ).x( ) * toRadsFactor;
            transformComposite.Properties.Rotation.y             = transformFb->rotation( ).y( ) * toRadsFactor;
            transformComposite.Properties.Rotation.z             = transformFb->rotation( ).z( ) * toRadsFactor;
            transformComposite.Properties.PostRotation.x         = transformFb->post_rotation( ).x( ) * toRadsFactor;
            transformComposite.Properties.PostRotation.y         = transformFb->post_rotation( ).y( ) * toRadsFactor;
            transformComposite.Properties.PostRotation.z         = transformFb->post_rotation( ).z( ) * toRadsFactor;
            transformComposite.Properties.ScalingOffset.x        = transformFb->scaling_offset( ).x( );
            transformComposite.Properties.ScalingOffset.y        = transformFb->scaling_offset( ).y( );
            transformComposite.Properties.ScalingOffset.z        = transformFb->scaling_offset( ).z( );
            transformComposite.Properties.ScalingPivot.x         = transformFb->scaling_pivot( ).x( );
            transformComposite.Properties.ScalingPivot.y         = transformFb->scaling_pivot( ).y( );
            transformComposite.Properties.ScalingPivot.z         = transformFb->scaling_pivot( ).z( );
            transformComposite.Properties.Scaling.x              = transformFb->scaling( ).x( );
            transformComposite.Properties.Scaling.y              = transformFb->scaling( ).y( );
            transformComposite.Properties.Scaling.z              = transformFb->scaling( ).z( );
            transformComposite.Properties.GeometricTranslation.x = transformFb->geometric_translation( ).x( );
            transformComposite.Properties.GeometricTranslation.y = transformFb->geometric_translation( ).y( );
            transformComposite.Properties.GeometricTranslation.z = transformFb->geometric_translation( ).z( );
            transformComposite.Properties.GeometricRotation.x    = transformFb->geometric_rotation( ).x( ) * toRadsFactor;
            transformComposite.Properties.GeometricRotation.y    = transformFb->geometric_rotation( ).y( ) * toRadsFactor;
            transformComposite.Properties.GeometricRotation.z    = transformFb->geometric_rotation( ).z( ) * toRadsFactor;
            transformComposite.Properties.GeometricScaling.x     = transformFb->geometric_scaling( ).x( );
            transformComposite.Properties.GeometricScaling.y     = transformFb->geometric_scaling( ).y( );
            transformComposite.Properties.GeometricScaling.z     = transformFb->geometric_scaling( ).z( );

            if ( !transformComposite.Properties.Validate( ) ) {
                LogError( "Found invalid transform, node id {}", pNodeFb->id( ) );
            }

            auto pAnimCurveIdsFb = pNodeFb->anim_curve_ids( );
            if ( apemodefb::FlatbuffersTVectorIsNotNullAndNotEmpty( pAnimCurveIdsFb ) ) {
                for ( const uint32_t animCurveId : *pAnimCurveIdsFb ) {
                    assert( animCurveId < pAnimCurvesFb->size( ) );
                    pScene->NodeIdToAnimCurveIds.insert( eastl::make_pair( node.Id, animCurveId ) );
                }
            }
        }

        pScene->UpdateTransformMatrices( bindPoseFrame );
    }

    if ( apemodefb::FlatbuffersTVectorIsNotNullAndNotEmpty( pAnimCurvesFb ) ) {

        assert( apemodefb::FlatbuffersTVectorIsNotNullAndNotEmpty( pAnimLayersFb ) );
        assert( apemodefb::FlatbuffersTVectorIsNotNullAndNotEmpty( pAnimStacksFb ) );
        pScene->AnimCurves.reserve( pAnimCurvesFb->size( ) );

        for ( auto pAnimCurveFb : *pAnimCurvesFb ) {
            assert( pAnimCurveFb );

            LogInfo( "Processing curve: #{} -> \"{}\", keys={}",
                     pAnimCurveFb->id( ),
                     GetStringProperty( pSrcScene, pAnimCurveFb->name_id( ) ).c_str( ),
                     pAnimCurveFb->keys( )->size( ) );

            pScene->AnimCurves.emplace_back( );
            auto &animCurve = pScene->AnimCurves.back( );

            assert( pAnimCurveFb->anim_layer_id( ) < pAnimLayersFb->size( ) );
            assert( pAnimCurveFb->anim_stack_id( ) < pAnimStacksFb->size( ) );
            assert( pAnimCurveFb->anim_layer_id( ) < 0xFFFFU );
            assert( pAnimCurveFb->anim_stack_id( ) < 0xFFFFU );

            animCurve.AnimLayerId = static_cast< uint16_t >( pAnimCurveFb->anim_layer_id( ) );
            animCurve.AnimStackId = static_cast< uint16_t >( pAnimCurveFb->anim_stack_id( ) );
            animCurve.eChannel    = SceneAnimCurve::EChannel( pAnimCurveFb->channel( ) );
            animCurve.eProperty   = SceneAnimCurve::EProperty( pAnimCurveFb->property( ) * SceneAnimCurve::eChannelCount );

            assert( FlatbuffersTVectorIsNotNullAndNotEmpty( pAnimCurveFb->keys( ) ) );
            animCurve.Keys.reserve( pAnimCurveFb->keys( )->size( ) );

            std::transform( pAnimCurveFb->keys( )->begin( ),
                            pAnimCurveFb->keys( )->end( ),
                            std::back_inserter( animCurve.Keys ),
                            []( const apemodefb::AnimCurveKeyFb *pKeyFb ) {
                                return XMFLOAT2{pKeyFb->time( ), pKeyFb->value( )};
                            } );

            animCurve.TimeMinMaxTotal.x = animCurve.Keys.front( ).x;
            animCurve.TimeMinMaxTotal.y = animCurve.Keys.back( ).x;
            animCurve.TimeMinMaxTotal.z = animCurve.TimeMinMaxTotal.y - animCurve.TimeMinMaxTotal.x;
        }

        pScene->AnimNodeIdToAnimCurveIds.reserve( pAnimCurvesFb->size( ) );
        for ( auto &node : pScene->Nodes ) {

            const auto animCurveIdRange = pScene->NodeIdToAnimCurveIds.equal_range( node.Id );
            for ( auto animCurveIdIt = animCurveIdRange.first; animCurveIdIt != animCurveIdRange.second; ++animCurveIdIt ) {

                const uint32_t animCurveId = animCurveIdIt->second;
                auto pAnimCurve = &pScene->AnimCurves[ animCurveId ];

                SceneAnimNodeId animNodeId;
                animNodeId.NodeId = node.Id;
                animNodeId.AnimLayerId.AnimLayerId = pAnimCurve->AnimLayerId;
                animNodeId.AnimLayerId.AnimStackId = pAnimCurve->AnimStackId;

                SceneNodeAnimCurveIds &animCurves = pScene->AnimNodeIdToAnimCurveIds[ animNodeId.AnimNodeCompositeId ];
                const uint32_t animCurveIndex = uint32_t( pAnimCurve->eProperty ) + uint32_t( pAnimCurve->eChannel );
                animCurves.AnimCurveIds[ animCurveIndex ] = animCurveIdIt->second;
            }
        }

        pScene->AnimLayerIdToTransformFrames.reserve( pAnimLayersFb->size( ) );
        for ( auto pAnimLayerFb : *pAnimLayersFb ) {
            auto pAnimStackFb = pAnimStacksFb->Get( pAnimLayerFb->anim_stack_id( ) );

            LogInfo( "Processing anim set: layer=#{} \"{}\", stack=#{} \"{}\"",
                     pAnimLayerFb->id( ),
                     GetStringProperty( pSrcScene, pAnimLayerFb->name_id( ) ).c_str( ),
                     pAnimStackFb->id( ),
                     GetStringProperty( pSrcScene, pAnimStackFb->name_id( ) ).c_str( ) );

            SceneAnimLayerId animLayerId;
            animLayerId.AnimLayerId = pAnimLayerFb->id( );
            animLayerId.AnimStackId = pAnimLayerFb->anim_stack_id( );

            auto &animTransformFrame = pScene->AnimLayerIdToTransformFrames[ animLayerId.AnimLayerCompositeId ];
            pScene->InitializeTransformFrame( animTransformFrame );
        }
    }

    if ( apemodefb::FlatbuffersTVectorIsNotNullAndNotEmpty( pMeshesFb ) ) {
        { /* All the subsets are stored in Scene instance, and can be referenced
           * by the BaseSubset and SubsetCount values in SceneMeshSubset struct.
           */

            size_t totalSubsetCount = 0;
            for ( uint32_t meshId = 0; meshId < pMeshesFb->size( ); ++meshId ) {
                auto pMeshFb = pMeshesFb->Get( meshId );
                totalSubsetCount += pMeshFb->subsets( ) ? pMeshFb->subsets( )->size( ) : 0;
            }

            pScene->Subsets.reserve( totalSubsetCount );
        }

        pScene->Meshes.reserve( pMeshesFb->size( ) );
        for ( uint32_t meshId = 0; meshId < pMeshesFb->size( ); ++meshId ) {
            auto pMeshFb = pMeshesFb->Get( meshId );

            assert( pMeshFb );
            assert( apemodefb::FlatbuffersTVectorIsNotNullAndNotEmpty( pMeshFb->vertices( ) ) );
            assert( apemodefb::FlatbuffersTVectorIsNotNullAndNotEmpty( pMeshFb->indices( ) ) );
            assert( apemodefb::FlatbuffersTVectorIsNotNullAndNotEmpty( pMeshFb->subsets( ) ) );
            assert( apemodefb::FlatbuffersTVectorIsNotNullAndNotEmpty( pMeshFb->submeshes( ) ) );

            pScene->Meshes.emplace_back( );
            auto &mesh = pScene->Meshes.back( );
            auto pSubmeshesFb = pMeshFb->submeshes( );
            auto pSubmeshFb   = pSubmeshesFb->Get( 0 );

            mesh.PositionOffset.x = pSubmeshFb->position_offset( ).x( );
            mesh.PositionOffset.y = pSubmeshFb->position_offset( ).y( );
            mesh.PositionOffset.z = pSubmeshFb->position_offset( ).z( );
            mesh.PositionScale.x  = pSubmeshFb->position_scale( ).x( );
            mesh.PositionScale.y  = pSubmeshFb->position_scale( ).y( );
            mesh.PositionScale.z  = pSubmeshFb->position_scale( ).z( );
            mesh.TexcoordOffset.x = pSubmeshFb->uv_offset( ).x( );
            mesh.TexcoordOffset.y = pSubmeshFb->uv_offset( ).y( );
            mesh.TexcoordScale.x  = pSubmeshFb->uv_scale( ).x( );
            mesh.TexcoordScale.y  = pSubmeshFb->uv_scale( ).y( );

            mesh.SubsetCount = static_cast< uint32_t >( pMeshFb->subsets( )->size( ) );
            mesh.BaseSubset  = static_cast< uint32_t >( pScene->Subsets.size( ) );

            std::transform( pMeshFb->subsets( )->begin( ),
                            pMeshFb->subsets( )->end( ),
                            std::back_inserter( pScene->Subsets ),
                            [&]( const apemodefb::SubsetFb *pSubsetFb ) {

                                SceneMeshSubset subset;
                                subset.MeshId     = mesh.Id;
                                subset.MaterialId = pSubsetFb->material_id( );
                                subset.BaseIndex  = pSubsetFb->base_index( );
                                subset.IndexCount = pSubsetFb->index_count( );

                                return subset;
                            } );

            mesh.Id     = meshId;
            mesh.SkinId = pMeshFb->skin_id( );
        }
    }

    if ( apemodefb::FlatbuffersTVectorIsNotNullAndNotEmpty( pSkinsFb ) ) {
        pScene->Skins.resize( pSkinsFb->size() );

        for ( auto &mesh : pScene->Meshes ) {
            if ( mesh.SkinId != detail::kInvalidId ) {

                auto &skin = pScene->Skins[ mesh.SkinId ];
                auto  pSkinFb = pSkinsFb->Get( mesh.SkinId );

                if ( skin.Links.empty( ) ) {

                    auto pLinkIdsFb = pSkinFb->links_ids( );
                    if ( apemodefb::FlatbuffersTVectorIsNotNullAndNotEmpty( pLinkIdsFb ) ) {

                        LogInfo( "Processing skin: \"{}\", links={}",
                                 GetStringProperty( pSrcScene, pSkinFb->name_id( ) ).c_str( ),
                                 pLinkIdsFb->size( ) );

                        skin.Links.reserve( pLinkIdsFb->size( ) );
                        std::transform( pLinkIdsFb->begin( ),
                                        pLinkIdsFb->end( ),
                                        std::back_inserter( skin.Links ),
                                        [&]( const uint32_t linkNodeId ) {
                                            auto &bindPoseFrame = pScene->BindPoseFrame;
                                            assert( linkNodeId < bindPoseFrame.Transforms.size( ) );
                                            const auto bindPoseMatrix = bindPoseFrame.Transforms[ linkNodeId ].WorldMatrix;

                                            SceneSkinLink skinLink;
                                            skinLink.LinkId = linkNodeId;
                                            skinLink.InvBindPoseMatrix = XMMatrixInverse( nullptr, bindPoseMatrix );
                                            return skinLink;
                                        } );
                    }
                }
            }
        }
    }

    auto pTexturesFb = pSrcScene->textures( );
    auto pFilesFb    = pSrcScene->files( );

    if ( FlatbuffersTVectorIsNotNullAndNotEmpty( pMaterialsFb ) ) {
        pScene->Materials.reserve( pMaterialsFb->size( ) );

        for ( uint32_t materialId = 0; materialId < pMaterialsFb->size( ); ++materialId ) {
            auto pMaterialFb = pMaterialsFb->Get( materialId );
            assert( pMaterialFb );

            LogInfo( "Processing material \"{}\": properties: {}, textures: {}",
                     GetCStringProperty( pSrcScene, pMaterialFb->name_id( ) ),
                     pMaterialFb->properties( ) ? pMaterialFb->properties( )->size( ) : 0,
                     pMaterialFb->texture_properties( ) ? pMaterialFb->texture_properties( )->size( ) : 0 );

            pScene->Materials.emplace_back( );
            auto &material = pScene->Materials.back( );
            material.Id = materialId;

            for ( auto pTexturePropFb : *pMaterialFb->texture_properties( ) ) {
                auto pszTexturePropName = GetCStringProperty( pSrcScene, pTexturePropFb->name_id( ) );

                auto pTextureFb = pTexturesFb->Get( pTexturePropFb->value_id( ) );
                assert( pTextureFb );

                auto pFileFb = pFilesFb->Get( pTextureFb->file_id( ) );
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
                        material.BaseColorFactor = ToVec4( GetVec4Property( pSrcScene, pMaterialPropFb->value_id( ) ) );
                    } else if ( strcmp( "baseColorFactor", pszMaterialPropName ) == 0 ) {
                        material.BaseColorFactor = ToVec4( GetVec4Property( pSrcScene, pMaterialPropFb->value_id( ) ) );
                    } else if ( strcmp( "metallicFactor", pszMaterialPropName ) == 0 ) {
                        material.MetallicFactor = GetScalarProperty( pSrcScene, pMaterialPropFb->value_id( ) );
                    } else if ( strcmp( "emissiveFactor", pszMaterialPropName ) == 0 ) {
                        material.EmissiveFactor = ToVec3( GetVec3Property( pSrcScene, pMaterialPropFb->value_id( ) ) );
                    } else  if ( strcmp( "specularFactor", pszMaterialPropName ) == 0 ) {
                        material.MetallicFactor = GetVec3Property( pSrcScene, pMaterialPropFb->value_id( ) ).x();
                    } else if ( strcmp( "glossinessFactor", pszMaterialPropName ) == 0 ) {
                        material.RoughnessFactor = 1.0f - GetScalarProperty( pSrcScene, pMaterialPropFb->value_id( ) );
                    } else if ( strcmp( "roughnessFactor", pszMaterialPropName ) == 0 ) {
                        material.RoughnessFactor = GetScalarProperty( pSrcScene, pMaterialPropFb->value_id( ) );
                    } else if ( strcmp( "doubleSided", pszMaterialPropName ) == 0 ) {
                        material.bDoubleSided = GetBoolProperty( pSrcScene, pMaterialPropFb->value_id( ) );
                    } else if ( strcmp( "alphaCutoff", pszMaterialPropName ) == 0 ) {
                        material.AlphaCutoff = GetScalarProperty( pSrcScene, pMaterialPropFb->value_id( ) );
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

    return LoadedScene{std::move( fileContents ), pSrcScene, std::move( pScene )};
}

void apemode::SceneAnimCurve::GetKeyIndices( float time, bool bLoop, uint32_t &i, uint32_t &j ) const {
    if ( bLoop ) {
        float relativeTime, fractionalPart, integerPart;
        relativeTime = ( time - TimeMinMaxTotal.x ) / TimeMinMaxTotal.z;
        fractionalPart = modf( relativeTime, &integerPart );
        time = TimeMinMaxTotal.x + TimeMinMaxTotal.z * fractionalPart;
        (void) integerPart;
    }

    if ( time < TimeMinMaxTotal.x ) {
        i = 0;
        j = 0;
    } else if ( time > TimeMinMaxTotal.y ) {
        i = static_cast< uint32_t >( Keys.size( ) ) - 1;
        j = i;
    } else {
        const float ii = TimeMinMaxTotal.z / ( time - TimeMinMaxTotal.x );
        i = static_cast< uint32_t >( floorf( ii ) );
        j = static_cast< uint32_t >( ceilf( ii ) );
    }
}

float apemode::SceneAnimCurve::Calculate( float time, bool bLoop ) const {

    uint32_t i, j;
    GetKeyIndices( time, bLoop, i, j );

    const XMFLOAT2 a = Keys[ i ];
    const XMFLOAT2 b = Keys[ j ];

    const float l = ( b.x - a.x ) / ( time - a.x );
    return ( b.y - a.y ) * l + a.y;
}
