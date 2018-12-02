
#include "Scene.h"
#include <apemode/platform/AppState.h>
#include <apemode/platform/memory/MemoryManager.h>

#ifndef PI
#define PI 3.14159265358979323846264338327950288
#endif

constexpr float toRadsFactor = float( PI ) / 180.0f;

namespace {
template < typename T >
inline bool IsNotNullAndNotEmpty( const flatbuffers::Vector< T > *pVector ) {
    return pVector && pVector->size( );
}
bool IsValid( const float a ) {
    return !isnan( a ) && !isinf( a );
}
bool IsIdentity( const apemode::XMFLOAT4X4 m ) {
    using namespace apemodexm;
    return IsNearlyEqual( m._11, 1 ) && IsNearlyEqual( m._12, 0 ) && IsNearlyEqual( m._13, 0 ) && IsNearlyEqual( m._14, 0 ) &&
           IsNearlyEqual( m._21, 0 ) && IsNearlyEqual( m._22, 1 ) && IsNearlyEqual( m._23, 0 ) && IsNearlyEqual( m._24, 0 ) &&
           IsNearlyEqual( m._31, 0 ) && IsNearlyEqual( m._32, 0 ) && IsNearlyEqual( m._33, 1 ) && IsNearlyEqual( m._34, 0 ) &&
           IsNearlyEqual( m._41, 0 ) && IsNearlyEqual( m._42, 0 ) && IsNearlyEqual( m._43, 0 ) && IsNearlyEqual( m._44, 1 );
}
bool IsValid( const apemode::XMFLOAT4X4 m ) {
    using namespace apemodexm;
    return /*!IsNearlyEqual( m._11, 0 ) && */ IsValid( m._12 ) && IsValid( m._13 ) && IsValid( m._14 ) && IsValid( m._21 ) &&
           /*!IsNearlyEqual( m._22, 0 ) && */ IsValid( m._23 ) && IsValid( m._24 ) && IsValid( m._31 ) && IsValid( m._32 ) &&
           /*!IsNearlyEqual( m._33, 0 ) && */ IsValid( m._34 ) && IsValid( m._41 ) && IsValid( m._42 ) && IsValid( m._43 ) &&
           /*!IsNearlyEqual( m._44, 0 ) && */ IsValid( m._11 ) && IsValid( m._22 ) && IsValid( m._33 ) && IsValid( m._44 ) &&
           false == ( IsNearlyZero( m._12 ) && IsNearlyZero( m._13 ) && IsNearlyZero( m._14 ) && IsNearlyZero( m._21 ) &&
                      IsNearlyZero( m._23 ) && IsNearlyZero( m._24 ) && IsNearlyZero( m._31 ) && IsNearlyZero( m._32 ) &&
                      IsNearlyZero( m._34 ) && IsNearlyZero( m._41 ) && IsNearlyZero( m._42 ) && IsNearlyZero( m._43 ) &&
                      IsNearlyZero( m._11 ) && IsNearlyZero( m._22 ) && IsNearlyZero( m._33 ) && IsNearlyZero( m._44 ) );
}
bool IsValid( const apemode::XMMATRIX m ) {
    apemode::XMFLOAT4X4 storedMatrix;
    XMStoreFloat4x4( &storedMatrix, m );
    return IsValid( storedMatrix );
}
} // namespace

using namespace apemode;

struct SceneAnimLayerId {
    union {
        struct {
            uint16_t AnimStackIndex;
            uint16_t AnimLayerIndex;
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
    return IsValid( Translation.x ) && IsValid( Translation.y ) && IsValid( Translation.z ) && IsValid( RotationOffset.x ) &&
           IsValid( RotationOffset.y ) && IsValid( RotationOffset.z ) && IsValid( RotationPivot.x ) &&
           IsValid( RotationPivot.y ) && IsValid( RotationPivot.z ) && IsValid( PreRotation.x ) && IsValid( PreRotation.y ) &&
           IsValid( PreRotation.z ) && IsValid( PostRotation.x ) && IsValid( PostRotation.y ) && IsValid( PostRotation.z ) &&
           IsValid( Rotation.x ) && IsValid( Rotation.y ) && IsValid( Rotation.z ) && IsValid( ScalingOffset.x ) &&
           IsValid( ScalingOffset.y ) && IsValid( ScalingOffset.z ) && IsValid( ScalingPivot.x ) && IsValid( ScalingPivot.y ) &&
           IsValid( ScalingPivot.z ) && IsValid( Scaling.x ) && IsValid( Scaling.y ) && IsValid( Scaling.z ) &&
           IsValid( GeometricTranslation.x ) && IsValid( GeometricTranslation.y ) && IsValid( GeometricTranslation.z ) &&
           IsValid( GeometricRotation.x ) && IsValid( GeometricRotation.y ) && IsValid( GeometricRotation.z ) &&
           IsValid( GeometricScaling.x ) && IsValid( GeometricScaling.y ) && IsValid( GeometricScaling.z ) &&
           !IsNearlyZero( Scaling.x ) && !IsNearlyZero( Scaling.y ) && !IsNearlyZero( Scaling.z ) &&
           !IsNearlyZero( GeometricScaling.x ) && !IsNearlyZero( GeometricScaling.y ) && !IsNearlyZero( GeometricScaling.z );
}

XMMATRIX XMMatrixRotationOrdered( const XMFLOAT3 v, detail::ERotationOrder eRotOrder ) {
    // return XMMatrixRotationZ( v.z ) * XMMatrixRotationY( v.y ) * XMMatrixRotationX( v.x );
    switch ( eRotOrder ) {
        case detail::eRotationOrder_EulerXYZ:
            return XMMatrixRotationX( v.x ) * XMMatrixRotationY( v.y ) * XMMatrixRotationZ( v.z ); //
        case detail::eRotationOrder_EulerXZY:
            return XMMatrixRotationX( v.x ) * XMMatrixRotationZ( v.z ) * XMMatrixRotationY( v.y ); //
        case detail::eRotationOrder_EulerYZX:
            return XMMatrixRotationY( v.y ) * XMMatrixRotationZ( v.z ) * XMMatrixRotationX( v.x ); //
        case detail::eRotationOrder_EulerYXZ:
            return XMMatrixRotationY( v.y ) * XMMatrixRotationX( v.x ) * XMMatrixRotationZ( v.z ); //
        case detail::eRotationOrder_EulerZXY:
            return XMMatrixRotationZ( v.z ) * XMMatrixRotationX( v.x ) * XMMatrixRotationY( v.y ); //
        case detail::eRotationOrder_EulerZYX:
            return XMMatrixRotationZ( v.z ) * XMMatrixRotationY( v.y ) * XMMatrixRotationX( v.x );
        case detail::eRotationOrder_EulerSphericXYZ:
            return XMMatrixRotationX( v.x ) * XMMatrixRotationY( v.y ) * XMMatrixRotationZ( v.z );
    }

    assert( false && "Unhandled rotation orders." );
    return XMMatrixRotationX( v.x ) * XMMatrixRotationY( v.y ) * XMMatrixRotationZ( v.z );
}

XMMATRIX XMMatrixRotationOrderedInversed( XMFLOAT3 v, const detail::ERotationOrder eRotOrder ) {
    v.x = -v.x;
    v.y = -v.y;
    v.z = -v.z;
    return XMMatrixRotationOrdered( v, eRotOrder );
}

XMMATRIX XMMatrixRotationZYX( const XMFLOAT3 *v ) {
    return XMMatrixRotationX( v->x ) *
           XMMatrixRotationY( v->y ) *
           XMMatrixRotationZ( v->z );
}

void apemode::SceneNodeTransform::ApplyLimits( const SceneNodeTransformLimits &limits ) {
#define CLAMP_PROPERTY_COMPONENT( P )           \
    if ( limits.Is##P##MaxActive.x ) {          \
        P.x = std::min( P.x, limits.P##Max.x ); \
    }                                           \
    if ( limits.Is##P##MinActive.x ) {          \
        P.x = std::max( P.x, limits.P##Min.x ); \
    }                                           \
    if ( limits.Is##P##MaxActive.x ) {          \
        P.y = std::min( P.y, limits.P##Max.y ); \
    }                                           \
    if ( limits.Is##P##MinActive.x ) {          \
        P.y = std::max( P.y, limits.P##Min.y ); \
    }                                           \
    if ( limits.Is##P##MaxActive.x ) {          \
        P.z = std::min( P.z, limits.P##Max.z ); \
    }                                           \
    if ( limits.Is##P##MinActive.x ) {          \
        P.z = std::max( P.z, limits.P##Min.z ); \
    }

    CLAMP_PROPERTY_COMPONENT( Translation );
    CLAMP_PROPERTY_COMPONENT( Rotation );
    CLAMP_PROPERTY_COMPONENT( Scaling );

#undef CLAMP_PROPERTY_COMPONENT
}

XMMATRIX apemode::SceneNodeTransform::CalculateLocalMatrix( const detail::ERotationOrder eOrder ) const {
//    return XMMatrixTranslationFromVector( XMLoadFloat3( &Translation ) ) *
//           XMMatrixRotationOrdered( Rotation, eOrder ) *
//           XMMatrixScalingFromVector( XMLoadFloat3( &Scaling ) );

//    return XMMatrixScalingFromVector( XMLoadFloat3( &Scaling ) ) *
//           XMMatrixRotationOrdered( Rotation, eOrder ) *
//           XMMatrixTranslationFromVector( XMLoadFloat3( &Translation ) );
//
//    return XMMatrixTranslationFromVector( XMLoadFloat3( &Translation ) ) *
//           XMMatrixTranslationFromVector( XMLoadFloat3( &RotationOffset ) ) *
//           XMMatrixTranslationFromVector( XMLoadFloat3( &RotationPivot ) ) *
//           XMMatrixRotationOrdered( PreRotation, eOrder ) *
//           XMMatrixRotationOrdered( Rotation, eOrder ) *
//           XMMatrixRotationOrderedInversed( PostRotation, eOrder ) *
//           XMMatrixTranslationFromVector( XMVectorNegate( XMLoadFloat3( &RotationPivot ) ) ) *
//           XMMatrixTranslationFromVector( XMLoadFloat3( &ScalingOffset ) ) *
//           XMMatrixTranslationFromVector( XMLoadFloat3( &ScalingPivot ) ) *
//           XMMatrixScalingFromVector( XMLoadFloat3( &Scaling ) ) *
//           XMMatrixTranslationFromVector( XMVectorNegate( XMLoadFloat3( &ScalingPivot ) ) );

    return XMMatrixTranslationFromVector( XMVectorNegate( XMLoadFloat3( &ScalingPivot ) ) ) *
           XMMatrixScalingFromVector( XMLoadFloat3( &Scaling ) ) *
           XMMatrixTranslationFromVector( XMLoadFloat3( &ScalingPivot ) ) *
           XMMatrixTranslationFromVector( XMLoadFloat3( &ScalingOffset ) ) *
           XMMatrixTranslationFromVector( XMVectorNegate( XMLoadFloat3( &RotationPivot ) ) ) *
           XMMatrixRotationOrderedInversed( PostRotation, eOrder ) *
           XMMatrixRotationOrdered( Rotation, eOrder ) *
           XMMatrixRotationOrdered( PreRotation, eOrder ) *
           XMMatrixTranslationFromVector( XMLoadFloat3( &RotationPivot ) ) *
           XMMatrixTranslationFromVector( XMLoadFloat3( &RotationOffset ) ) *
           XMMatrixTranslationFromVector( XMLoadFloat3( &Translation ) );

}

XMMATRIX apemode::SceneNodeTransform::CalculateGeometricMatrix( const detail::ERotationOrder eOrder ) const {
    return XMMatrixScalingFromVector( XMLoadFloat3( &GeometricScaling ) ) *
           XMMatrixRotationOrdered( GeometricRotation, eOrder ) *
           XMMatrixTranslationFromVector( XMLoadFloat3( &GeometricTranslation ) );
}

void apemode::Scene::InitializeTransformFrame( SceneNodeTransformFrame &t ) const {
    t.Transforms.resize( Nodes.size( ) );
}

const apemode::SceneNodeAnimCurveIds *apemode::Scene::GetAnimCurveIds( const uint32_t nodeId,
                                                                       const uint16_t animStackId,
                                                                       const uint16_t animLayerId ) const {

    if ( ! AnimNodeIdToAnimCurveIds.empty( ) ) {

        SceneAnimNodeId animNodeCompositeId;
        animNodeCompositeId.NodeId                  = nodeId;
        animNodeCompositeId.AnimLayerId.AnimStackIndex = animStackId;
        animNodeCompositeId.AnimLayerId.AnimLayerIndex = animLayerId;

        const auto animCurveIdsIt = AnimNodeIdToAnimCurveIds.find( animNodeCompositeId.AnimNodeCompositeId );
        if ( animCurveIdsIt != AnimNodeIdToAnimCurveIds.end( ) ) {
            assert( AnimNodeIdToAnimCurveIds.end( ) != animCurveIdsIt );
            return &animCurveIdsIt->second;
        }
    }

    return nullptr;
}

inline XMMATRIX CalculateOffsetMatrix( const XMMATRIX invBindPoseMatrix, const XMMATRIX currentAnimatedMatrix ) {
    return invBindPoseMatrix * currentAnimatedMatrix;
}

void apemode::Scene::UpdateSkinMatrices( const SceneSkin &              skin,
                                         const SceneNodeTransformFrame *pSceneAnimatedFrame,
                                         const XMMATRIX                 nodeWorldTransform,
                                         XMFLOAT4X4 *                   pOffsetMatrices,
                                         XMFLOAT4X4 *                   pNormalMatrices,
                                         size_t                         matrixCount ) const {

    assert( matrixCount >= skin.LinkIds.size( ) );
    for ( size_t i = 0; i < skin.LinkIds.size( ); ++i ) {
        uint32_t nodeId = skin.LinkIds[ i ];
        XMMATRIX currentWorldMatrix = pSceneAnimatedFrame->Transforms[ nodeId ].WorldMatrix;
        // currentWorldMatrix = XMMatrixInverse(0, nodeWorldTransform) * currentWorldMatrix;

        // const SceneSkeleton &skeleton  = Skeletons[ skin.SkeletonId ];
        // const uint32_t nodeIndex = skin.SkeletonNodeIndices[ i ];
        // assert( nodeIndex < skeleton.NodeIds.size( ) );
        // const XMMATRIX currentWorldMatrix = pSkeletonAnimatedFrame->Transforms[ nodeIndex ].WorldMatrix;
        // const XMMATRIX currentWorldMatrix = pSkeletonAnimatedFrame->Transforms[ nodeIndex ].HierarchicalMatrix;

        const XMMATRIX invBindPoseMatrix  = skin.InvBindPoseMatrices[ i ];
        const XMMATRIX offsetMatrix       = CalculateOffsetMatrix( invBindPoseMatrix , currentWorldMatrix );

        assert( IsValid( currentWorldMatrix ) );
        assert( IsValid( invBindPoseMatrix ) );
        assert( IsValid( offsetMatrix ) );

        XMStoreFloat4x4( &pOffsetMatrices[ i ], offsetMatrix );
        // assert( IsIdentity( pOffsetMatrices[ i ] ) );

        if ( pNormalMatrices ) {
            XMStoreFloat4x4( &pNormalMatrices[ i ], XMMatrixTranspose( XMMatrixInverse( 0, offsetMatrix ) ) );
        }
    }
}

bool IsRotationProperty( const apemode::SceneAnimCurve::EProperty eProperty ) {
    switch ( eProperty ) {
        case apemode::SceneAnimCurve::eProperty_LclRotation:
        case apemode::SceneAnimCurve::eProperty_PreRotation:
        case apemode::SceneAnimCurve::eProperty_PostRotation:
        case apemode::SceneAnimCurve::eProperty_GeometricRotation:
            return true;

        default:
            return false;
    }
}

XMFLOAT3 *MapProperty( apemode::SceneAnimCurve::EProperty eProperty, apemode::SceneNodeTransform *pProperties ) {
    static_assert( sizeof( SceneNodeTransform ) == ( sizeof( XMFLOAT3 ) * SceneAnimCurve::ePropertyCount / 3 ), "Re-implement mapping with switch cases." );
    assert( ( ( eProperty % 3 ) == 0 ) && ( eProperty < SceneAnimCurve::ePropertyCount ) );
    return reinterpret_cast< XMFLOAT3 * >( pProperties ) + ( eProperty / 3 );
}

void apemode::Scene::UpdateTransformProperties( float                    time,
                                                const bool               bLoop,
                                                const uint16_t           animStackId,
                                                const uint16_t           animLayerId,
                                                SceneNodeTransformFrame *pAnimTransformFrame ) {
    const float debugTimeSpan = 20;

    XMFLOAT3 TimeMinMaxTotal;
    TimeMinMaxTotal.x = 0;
    TimeMinMaxTotal.y = debugTimeSpan;
    TimeMinMaxTotal.z = debugTimeSpan;

    if ( bLoop ) {
// Loop the given time value.
#define SCENEANIMCURVE_USE_MODF
#ifdef SCENEANIMCURVE_USE_MODF
        float relativeTime, fractionalPart, integerPart;
        relativeTime = ( time - TimeMinMaxTotal.x ) / TimeMinMaxTotal.z;
        fractionalPart = modf( relativeTime, &integerPart );
        time = TimeMinMaxTotal.x + TimeMinMaxTotal.z * fractionalPart;
        (void) integerPart;
#else
        float relativeTime, fractionalPart;
        relativeTime = ( time - TimeMinMaxTotal.x ) / TimeMinMaxTotal.z;
        fractionalPart = relativeTime - (float) (long) relativeTime;
        time = TimeMinMaxTotal.x + TimeMinMaxTotal.z * fractionalPart;
#endif
    }

    if ( pAnimTransformFrame ) { //SceneNodeTransformFrame *pAnimTransformFrame = GetAnimatedTransformFrame( animStackId, animLayerId ) ) {
        *pAnimTransformFrame = BindPoseFrame;
        // assert( pAnimTransformFrame->Transforms.size( ) == BindPoseFrame.Transforms.size( ) );
        // const size_t transformFrameByteSize = sizeof( SceneNodeTransformComposite ) * pAnimTransformFrame->Transforms.size( );
        // memcpy( pAnimTransformFrame->Transforms.data( ), BindPoseFrame.Transforms.data( ), transformFrameByteSize );

        for ( const SceneNode &node : Nodes ) {
            if ( const SceneNodeAnimCurveIds *animCurveIds = GetAnimCurveIds( node.Id, animStackId, animLayerId ) ) {
                auto &animTransformComposite = pAnimTransformFrame->Transforms[ node.Id ];

                for ( uint32_t propertyIndex = 0; propertyIndex < SceneAnimCurve::ePropertyCount; propertyIndex += SceneAnimCurve::eChannelCount ) {
                    const SceneAnimCurve::EProperty eProperty = SceneAnimCurve::EProperty( propertyIndex );
                    const float convertionFactor = IsRotationProperty( eProperty ) ? toRadsFactor : 1.0f;

                    XMFLOAT3 *pProperty = MapProperty( eProperty, &animTransformComposite.Properties );

                    const uint32_t animCurveIdX = animCurveIds->AnimCurveIds[ propertyIndex + SceneAnimCurve::eChannel_X ];
                    const uint32_t animCurveIdY = animCurveIds->AnimCurveIds[ propertyIndex + SceneAnimCurve::eChannel_Y ];
                    const uint32_t animCurveIdZ = animCurveIds->AnimCurveIds[ propertyIndex + SceneAnimCurve::eChannel_Z ];

                    const SceneAnimCurve *pAnimCurveX = ( animCurveIdX != detail::kInvalidId ) ? ( &AnimCurves[ animCurveIdX ] ) : nullptr;
                    const SceneAnimCurve *pAnimCurveY = ( animCurveIdY != detail::kInvalidId ) ? ( &AnimCurves[ animCurveIdY ] ) : nullptr;
                    const SceneAnimCurve *pAnimCurveZ = ( animCurveIdZ != detail::kInvalidId ) ? ( &AnimCurves[ animCurveIdZ ] ) : nullptr;

                    if ( pAnimCurveX || pAnimCurveY || pAnimCurveZ ) {
                        switch ( eProperty ) {
                            case SceneAnimCurve::eProperty_PreRotation:
                            case SceneAnimCurve::eProperty_PostRotation:
                            case SceneAnimCurve::eProperty_ScalingPivot:
                            case SceneAnimCurve::eProperty_ScalingOffset:
                            case SceneAnimCurve::eProperty_RotationPivot:
                            case SceneAnimCurve::eProperty_RotationOffset:
                            case SceneAnimCurve::eProperty_GeometricScaling:
                            case SceneAnimCurve::eProperty_GeometricRotation:
                            case SceneAnimCurve::eProperty_GeometricTranslation: {
                                LogWarn("Animating an object-offset property: {}",
                                        apemodefb::EnumNameEAnimCurvePropertyFb(
                                        apemodefb::EAnimCurvePropertyFb(eProperty)));
                            } break;
                            default:
                                break;
                        }
                    }

                    assert( !pAnimCurveX || pAnimCurveX->eProperty == eProperty && pAnimCurveX->eChannel == SceneAnimCurve::eChannel_X );
                    assert( !pAnimCurveY || pAnimCurveY->eProperty == eProperty && pAnimCurveY->eChannel == SceneAnimCurve::eChannel_Y );
                    assert( !pAnimCurveZ || pAnimCurveZ->eProperty == eProperty && pAnimCurveZ->eChannel == SceneAnimCurve::eChannel_Z );

                    pProperty->x = pAnimCurveX ? ( pAnimCurveX->Calculate( time ) * convertionFactor ) : pProperty->x;
                    pProperty->y = pAnimCurveY ? ( pAnimCurveY->Calculate( time ) * convertionFactor ) : pProperty->y;
                    pProperty->z = pAnimCurveZ ? ( pAnimCurveZ->Calculate( time ) * convertionFactor ) : pProperty->z;

                    assert( animTransformComposite.Properties.Validate( ) );
                }
            }
        }

        // return pAnimTransformFrame;
    }

    // return false;
    // return nullptr;
}

SceneNodeTransformFrame &apemode::Scene::GetBindPoseTransformFrame( ) {
    return BindPoseFrame;
}

const SceneNodeTransformFrame &apemode::Scene::GetBindPoseTransformFrame( ) const {
    return BindPoseFrame;
}


bool apemode::Scene::HasAnimStackLayer( uint16_t animStackId, uint16_t animLayerId ) const {
    if ( animStackId < AnimStacks.size() ) {
        return animLayerId < AnimStacks[animStackId].LayerCount;
    }

    return false;
}

void apemode::Scene::UpdateTransformMatrices( const uint32_t parentNodeId, SceneNodeTransformFrame &t ) const {
    assert( parentNodeId == Nodes[ parentNodeId ].Id );
    const SceneNodeTransformComposite &transformComposite = t.Transforms[ parentNodeId ];

    const auto childIdRange = NodeToChildIds.equal_range( parentNodeId );
    for ( auto childIdIt = childIdRange.first; childIdIt != childIdRange.second; ++childIdIt ) {
        const uint32_t childId = childIdIt->second;
        const SceneNode & childNode = Nodes[ childId ];
        SceneNodeTransformComposite &childTransformComposite = t.Transforms[ childId ];

        assert( parentNodeId == childNode.ParentId );

        if (childNode.LimitsId != uint32_t(-1)) {
            childTransformComposite.Properties.ApplyLimits(Limits[childNode.LimitsId]);
        }

        childTransformComposite.LocalMatrix        = childTransformComposite.Properties.CalculateLocalMatrix( childNode.eOrder );
        childTransformComposite.GeometricalMatrix  = childTransformComposite.Properties.CalculateGeometricMatrix( childNode.eOrder );
        childTransformComposite.HierarchicalMatrix = childTransformComposite.LocalMatrix * transformComposite.HierarchicalMatrix;
        childTransformComposite.WorldMatrix        = childTransformComposite.GeometricalMatrix * childTransformComposite.HierarchicalMatrix;

        assert( childTransformComposite.Properties.Validate() );
        assert( IsValid( transformComposite.HierarchicalMatrix ) );
        assert( IsValid( childTransformComposite.LocalMatrix ) );
        assert( IsValid( childTransformComposite.HierarchicalMatrix ) );
        assert( IsValid( childTransformComposite.GeometricalMatrix ) );
        assert( IsValid( childTransformComposite.WorldMatrix ) );

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
    const detail::ERotationOrder eRootOrder = Nodes.front( ).eOrder;

    rootTransformComposite.LocalMatrix        = rootTransformComposite.Properties.CalculateLocalMatrix( eRootOrder );
    rootTransformComposite.GeometricalMatrix  = rootTransformComposite.Properties.CalculateGeometricMatrix( eRootOrder );
    rootTransformComposite.HierarchicalMatrix = rootTransformComposite.LocalMatrix;
    rootTransformComposite.WorldMatrix        = rootTransformComposite.GeometricalMatrix * rootTransformComposite.LocalMatrix;

    UpdateTransformMatrices( 0, t );
}

apemode::LoadedScene apemode::LoadSceneFromBin( apemode::vector< uint8_t > && fileContents ) {
    using namespace utils;

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

    if ( IsNotNullAndNotEmpty( pAnimStacksFb ) ) {
        pScene->AnimStacks.resize( pAnimStacksFb->size( ) );
        for ( uint32_t i = 0; i < pAnimStacksFb->size( ); ++i ) {
            pScene->AnimStacks[ i ].pszName = GetCStringProperty( pSrcScene, pAnimStacksFb->Get( i )->name_id( ) );
            pScene->AnimStacks[ i ].LayerCount = 0;
            LogInfo( "Adding animation stack: \"{}\"", pScene->AnimStacks[ i ].pszName );
        }
    }

    pScene->BindPoseBoundingBox.Extents.x = ( pSrcScene->bbox_max( )->x( ) - pSrcScene->bbox_min( )->x( ) ) * 0.5f;
    pScene->BindPoseBoundingBox.Extents.y = ( pSrcScene->bbox_max( )->y( ) - pSrcScene->bbox_min( )->y( ) ) * 0.5f;
    pScene->BindPoseBoundingBox.Extents.z = ( pSrcScene->bbox_max( )->z( ) - pSrcScene->bbox_min( )->z( ) ) * 0.5f;
    pScene->BindPoseBoundingBox.Center.x  = pScene->BindPoseBoundingBox.Extents.x + pSrcScene->bbox_min( )->x( );
    pScene->BindPoseBoundingBox.Center.y  = pScene->BindPoseBoundingBox.Extents.y + pSrcScene->bbox_min( )->y( );
    pScene->BindPoseBoundingBox.Center.z  = pScene->BindPoseBoundingBox.Extents.z + pSrcScene->bbox_min( )->z( );

    if ( IsNotNullAndNotEmpty( pNodesFb ) ) {
        size_t nodeIdCount      = 0;
        size_t animCurveIdCount = 0;

        for ( auto pNodeFb : *pNodesFb ) {
            nodeIdCount += IsNotNullAndNotEmpty( pNodeFb->child_ids( ) ) ? pNodeFb->child_ids( )->size( ) : 0;
            animCurveIdCount += IsNotNullAndNotEmpty( pNodeFb->anim_curve_ids( ) ) ? pNodeFb->anim_curve_ids( )->size( ) : 0;
        }

        pScene->NodeToChildIds.reserve( nodeIdCount );
        pScene->NodeIdToAnimCurveIds.reserve( animCurveIdCount );

        LogInfo( "Node IDs: {}", nodeIdCount );
        LogInfo( "Curve IDs: {}", animCurveIdCount );
    }

    if ( IsNotNullAndNotEmpty( pNodesFb ) ) {
        pScene->Nodes.resize( pNodesFb->size( ) );

        SceneNodeTransformFrame &bindPoseFrame = pScene->BindPoseFrame;
        pScene->InitializeTransformFrame( bindPoseFrame );

        apemode::vector<uint32_t> rootIds;
        apemode::vector<uint32_t> limbIds;

        for ( auto pNodeFb : *pNodesFb ) {
            assert( pNodeFb );

            auto &node = pScene->Nodes[ pNodeFb->id( ) ];

            node.Id            = pNodeFb->id( );
            node.eOrder        = detail::ERotationOrder( pNodeFb->rotation_order( ) );
            node.eInheritType  = detail::EInheritType( pNodeFb->inherit_type( ) );
            node.eSkeletonType = detail::ESkeletonType( pNodeFb->skeleton_type( ) );
            node.pszName       = GetCStringProperty( pSrcScene, pNodeFb->name_id( ) );

            switch (node.eSkeletonType) {
              case detail::eSkeletonType_Root:
                rootIds.push_back( node.Id );
                limbIds.push_back( node.Id );
                break;
              case detail::eSkeletonType_Limb:
              case detail::eSkeletonType_LimbNode:
                limbIds.push_back( node.Id );
                break;

              default:
                break;
            }

            if ( pNodeFb->mesh_id( ) != detail::kInvalidId ) {
                node.MeshId = pNodeFb->mesh_id( );
            }

            LogInfo( "Processing node: {}{}"
                , GetCStringProperty( pSrcScene, pNodeFb->name_id( ) )
                , pNodeFb->mesh_id() != detail::kInvalidId ? " (has mesh)" : "" );

            auto pChildIdsFb = pNodeFb->child_ids( );
            if ( IsNotNullAndNotEmpty( pChildIdsFb ) ) {
                for ( const uint32_t childNodeId : *pChildIdsFb ) {
                    pScene->NodeToChildIds.insert( eastl::make_pair( node.Id, childNodeId ) );
                    pScene->Nodes[ childNodeId ].ParentId = node.Id;
                }
            }

            auto &transformComposite = bindPoseFrame.Transforms[ pNodeFb->id( ) ];
            auto transformFb = pSrcScene->transforms( )->Get( pNodeFb->id( ) );

#define MATCH_VECTOR_TYPE( v, V ) \
    {                             \
        v.x = V.x( );             \
        v.y = V.y( );             \
        v.z = V.z( );             \
    }

            MATCH_VECTOR_TYPE( transformComposite.Properties.Translation, transformFb->translation( ) );
            MATCH_VECTOR_TYPE( transformComposite.Properties.RotationOffset, transformFb->rotation_offset( ) );
            MATCH_VECTOR_TYPE( transformComposite.Properties.RotationPivot, transformFb->rotation_pivot( ) );
            MATCH_VECTOR_TYPE( transformComposite.Properties.PreRotation, transformFb->pre_rotation( ) );
            MATCH_VECTOR_TYPE( transformComposite.Properties.Rotation, transformFb->rotation( ) );
            MATCH_VECTOR_TYPE( transformComposite.Properties.PostRotation, transformFb->post_rotation( ) );
            MATCH_VECTOR_TYPE( transformComposite.Properties.ScalingOffset, transformFb->scaling_offset( ) );
            MATCH_VECTOR_TYPE( transformComposite.Properties.ScalingPivot, transformFb->scaling_pivot( ) );
            MATCH_VECTOR_TYPE( transformComposite.Properties.Scaling, transformFb->scaling( ) );
            MATCH_VECTOR_TYPE( transformComposite.Properties.GeometricTranslation, transformFb->geometric_translation( ) );
            MATCH_VECTOR_TYPE( transformComposite.Properties.GeometricRotation, transformFb->geometric_rotation( ) );
            MATCH_VECTOR_TYPE( transformComposite.Properties.GeometricScaling, transformFb->geometric_scaling( ) );

            transformComposite.Properties.PreRotation.x *= toRadsFactor;
            transformComposite.Properties.PreRotation.y *= toRadsFactor;
            transformComposite.Properties.PreRotation.z *= toRadsFactor;
            transformComposite.Properties.Rotation.x *= toRadsFactor;
            transformComposite.Properties.Rotation.y *= toRadsFactor;
            transformComposite.Properties.Rotation.z *= toRadsFactor;
            transformComposite.Properties.PostRotation.x *= toRadsFactor;
            transformComposite.Properties.PostRotation.y *= toRadsFactor;
            transformComposite.Properties.PostRotation.z *= toRadsFactor;
            transformComposite.Properties.GeometricRotation.x *= toRadsFactor;
            transformComposite.Properties.GeometricRotation.y *= toRadsFactor;
            transformComposite.Properties.GeometricRotation.z *= toRadsFactor;

#define REPORT_USED_PROPERTY( P, d )                                                    \
    if ( !IsNearlyEqual( P.x, d ) || !IsNearlyEqual( P.y, d ) || !IsNearlyEqual( P.z, d ) ) { \
        LogWarn( "Node \"{}\" uses property \"{}\".", GetCStringProperty( pSrcScene, pNodeFb->name_id( ) ), #P );                                          \
    }

            REPORT_USED_PROPERTY( transformComposite.Properties.Translation, 0 );
            REPORT_USED_PROPERTY( transformComposite.Properties.Rotation, 0 );
            REPORT_USED_PROPERTY( transformComposite.Properties.Scaling, 1 );
            REPORT_USED_PROPERTY( transformComposite.Properties.ScalingOffset, 0 );
            REPORT_USED_PROPERTY( transformComposite.Properties.ScalingPivot, 0 );
            REPORT_USED_PROPERTY( transformComposite.Properties.RotationOffset, 0 );
            REPORT_USED_PROPERTY( transformComposite.Properties.RotationPivot, 0 );
            REPORT_USED_PROPERTY( transformComposite.Properties.PreRotation, 0 );
            REPORT_USED_PROPERTY( transformComposite.Properties.PostRotation, 0 );
            REPORT_USED_PROPERTY( transformComposite.Properties.GeometricRotation, 0 );
            REPORT_USED_PROPERTY( transformComposite.Properties.GeometricTranslation, 0 );
            REPORT_USED_PROPERTY( transformComposite.Properties.GeometricScaling, 1 );

#undef REPORT_USED_PROPERTY

            if ( !transformComposite.Properties.Validate( ) ) {
                LogError( "Found invalid transform, node id {}", pNodeFb->id( ) );
                assert( false );
            }

            if ( pNodeFb->transform_limits_id( ) != uint32_t( -1 ) ) {
                auto pTransformLimitsFb = pSrcScene->transform_limits( )->Get( pNodeFb->transform_limits_id( ) );
                node.LimitsId = uint32_t(pScene->Limits.size());

                auto &limits = pScene->Limits.emplace_back( );
                MATCH_VECTOR_TYPE( limits.IsTranslationMaxActive, pTransformLimitsFb->translation_max_active( ) );
                MATCH_VECTOR_TYPE( limits.IsTranslationMinActive, pTransformLimitsFb->translation_min_active( ) );
                MATCH_VECTOR_TYPE( limits.IsRotationMaxActive, pTransformLimitsFb->rotation_max_active( ) );
                MATCH_VECTOR_TYPE( limits.IsRotationMinActive, pTransformLimitsFb->rotation_min_active( ) );
                MATCH_VECTOR_TYPE( limits.IsScalingMaxActive, pTransformLimitsFb->scaling_max_active( ) );
                MATCH_VECTOR_TYPE( limits.IsScalingMinActive, pTransformLimitsFb->scaling_min_active( ) );
                MATCH_VECTOR_TYPE( limits.TranslationMax, pTransformLimitsFb->translation_max( ) );
                MATCH_VECTOR_TYPE( limits.TranslationMin, pTransformLimitsFb->translation_min( ) );
                MATCH_VECTOR_TYPE( limits.RotationMax, pTransformLimitsFb->rotation_max( ) );
                MATCH_VECTOR_TYPE( limits.RotationMin, pTransformLimitsFb->rotation_min( ) );
                MATCH_VECTOR_TYPE( limits.ScalingMax, pTransformLimitsFb->scaling_max( ) );
                MATCH_VECTOR_TYPE( limits.ScalingMin, pTransformLimitsFb->scaling_min( ) );

                limits.RotationMax.x *= toRadsFactor;
                limits.RotationMax.y *= toRadsFactor;
                limits.RotationMax.z *= toRadsFactor;
                limits.RotationMin.x *= toRadsFactor;
                limits.RotationMin.y *= toRadsFactor;
                limits.RotationMin.z *= toRadsFactor;
            }

#undef MATCH_VECTOR_TYPE

            auto pAnimCurveIdsFb = pNodeFb->anim_curve_ids( );
            if ( IsNotNullAndNotEmpty( pAnimCurveIdsFb ) ) {
                for ( const uint32_t animCurveId : *pAnimCurveIdsFb ) {
                    assert( animCurveId < pAnimCurvesFb->size( ) );
                    pScene->NodeIdToAnimCurveIds.insert( eastl::make_pair( node.Id, animCurveId ) );
                }
            }
        }

        detail::ScenePrintPretty prettyPrint;
        prettyPrint.PrintPretty( pScene.get( ) );

        pScene->UpdateTransformMatrices( bindPoseFrame );
        //BuildSkeletons( pScene.get(), eastl::move( rootIds ), eastl::move( limbIds ) );

#if 0
        LogInfo( "----------------------------------" );
        for ( auto &node : pScene->Nodes ) {
            XMFLOAT4X4 WM;
            XMFLOAT4X4 HM;
            XMStoreFloat4x4( &WM, bindPoseFrame.Transforms[ node.Id ].WorldMatrix );
            XMStoreFloat4x4( &HM, bindPoseFrame.Transforms[ node.Id ].HierarchicalMatrix );

            LogInfo( "Node \"{}\"", node.pszName );
            LogInfo( "----------------------------------" );
            LogInfo( "\t{} {} {} {}", WM._11, WM._12, WM._13, WM._14 );
            LogInfo( "\t{} {} {} {}", WM._21, WM._22, WM._23, WM._24 );
            LogInfo( "\t{} {} {} {}", WM._31, WM._32, WM._33, WM._34 );
            LogInfo( "\t{} {} {} {}", WM._41, WM._42, WM._43, WM._44 );
            LogInfo( "----------------------------------" );
            LogInfo( "\t{} {} {} {}", HM._11, HM._12, HM._13, HM._14 );
            LogInfo( "\t{} {} {} {}", HM._21, HM._22, HM._23, HM._24 );
            LogInfo( "\t{} {} {} {}", HM._31, HM._32, HM._33, HM._34 );
            LogInfo( "\t{} {} {} {}", HM._41, HM._42, HM._43, HM._44 );
            LogInfo( "----------------------------------" );
        }
#endif
    }

    if ( IsNotNullAndNotEmpty( pAnimCurvesFb ) ) {

        assert( IsNotNullAndNotEmpty( pAnimLayersFb ) );
        assert( IsNotNullAndNotEmpty( pAnimStacksFb ) );
        pScene->AnimCurves.reserve( pAnimCurvesFb->size( ) );

        for ( auto pAnimCurveFb : *pAnimCurvesFb ) {
            assert( pAnimCurveFb );

            LogInfo( "Processing curve: #{} -> \"{}\", keys={}",
                     pAnimCurveFb->id( ),
                     GetCStringProperty( pSrcScene, pAnimCurveFb->name_id( ) ),
                     pAnimCurveFb->keys( )->size( ) );

            pScene->AnimCurves.emplace_back( );
            auto &animCurve = pScene->AnimCurves.back( );

            assert( pAnimCurveFb->anim_layer_id( ) < pAnimLayersFb->size( ) );
            assert( pAnimCurveFb->anim_stack_id( ) < pAnimStacksFb->size( ) );
            assert( pAnimCurveFb->anim_layer_id( ) < 0xFFFFU );
            assert( pAnimCurveFb->anim_stack_id( ) < 0xFFFFU );

            auto pAnimLayerFbIt = eastl::find_if(pAnimLayersFb->begin(), pAnimLayersFb->end(), [pAnimCurveFb](const apemodefb::AnimLayerFb * pAnimLayerFb) {
                return pAnimLayerFb->id() == pAnimCurveFb->anim_layer_id();
            });

            pScene->AnimStacks[pAnimCurveFb->anim_stack_id( )].LayerCount =
                eastl::max(pScene->AnimStacks[pAnimCurveFb->anim_stack_id( )].LayerCount, pAnimLayerFbIt->anim_stack_idx( ) + 1);

            animCurve.AnimLayerIndex = static_cast< uint16_t >( pAnimLayerFbIt->anim_stack_idx( ) );
            animCurve.AnimStackIndex = static_cast< uint16_t >( pAnimCurveFb->anim_stack_id( ) );
            animCurve.eChannel    = SceneAnimCurve::EChannel( pAnimCurveFb->channel( ) );
            animCurve.eProperty   = SceneAnimCurve::EProperty( pAnimCurveFb->property( ) * SceneAnimCurve::eChannelCount );

            assert( IsNotNullAndNotEmpty( pAnimCurveFb->keys( ) ) );
            animCurve.Keys.reserve( pAnimCurveFb->keys( )->size( ) );

            eastl::transform( pAnimCurveFb->keys( )->begin( ),
                              pAnimCurveFb->keys( )->end( ),
                              eastl::inserter( animCurve.Keys, animCurve.Keys.begin( ) ),
                              []( const apemodefb::AnimCurveKeyFb *pKeyFb ) {
                                  return eastl::make_pair< float, SceneAnimCurveKey >( pKeyFb->time( ),
                                                                                       SceneAnimCurveKey{
                                                                                           SceneAnimCurveKey::EInterpolationMode(pKeyFb->interpolation_mode()),
                                                                                           pKeyFb->time( ),
                                                                                           pKeyFb->value_bez0_bez3( ),
                                                                                           pKeyFb->bez1( ),
                                                                                           pKeyFb->bez2( ),
                                                                                       } );
                              } );

            animCurve.TimeMinMaxTotal.x = animCurve.Keys.cbegin( )->second.Time;
            animCurve.TimeMinMaxTotal.y = animCurve.Keys.crbegin( )->second.Time;
            animCurve.TimeMinMaxTotal.z = animCurve.TimeMinMaxTotal.y - animCurve.TimeMinMaxTotal.x;

            LogInfo( "\tStart: {} -> End: {} (Duration: {})",
                     animCurve.TimeMinMaxTotal.x,
                     animCurve.TimeMinMaxTotal.y,
                     animCurve.TimeMinMaxTotal.z );
        }

        pScene->AnimNodeIdToAnimCurveIds.reserve( pAnimCurvesFb->size( ) );
        for ( auto &node : pScene->Nodes ) {

            const auto animCurveIdRange = pScene->NodeIdToAnimCurveIds.equal_range( node.Id );
            for ( auto animCurveIdIt = animCurveIdRange.first; animCurveIdIt != animCurveIdRange.second; ++animCurveIdIt ) {

                const uint32_t animCurveId = animCurveIdIt->second;
                auto pAnimCurve = &pScene->AnimCurves[ animCurveId ];

                SceneAnimNodeId animNodeId;
                animNodeId.NodeId = node.Id;
                animNodeId.AnimLayerId.AnimLayerIndex = pAnimCurve->AnimLayerIndex;
                animNodeId.AnimLayerId.AnimStackIndex = pAnimCurve->AnimStackIndex;

                SceneNodeAnimCurveIds &animCurves = pScene->AnimNodeIdToAnimCurveIds[ animNodeId.AnimNodeCompositeId ];
                const uint32_t animCurveIndex = uint32_t( pAnimCurve->eProperty ) + uint32_t( pAnimCurve->eChannel );
                animCurves.AnimCurveIds[ animCurveIndex ] = animCurveIdIt->second;
            }
        }

        // pScene->AnimLayerIdToTransformFrames.reserve( pAnimLayersFb->size( ) );
        for ( auto pAnimLayerFb : *pAnimLayersFb ) {
            auto pAnimStackFb = pAnimStacksFb->Get( pAnimLayerFb->anim_stack_id( ) );

            LogInfo( "Processing anim set: stack=#{} \"{}\", layer=#{} \"{}\"",
                     pAnimStackFb->id( ),
                     GetCStringProperty( pSrcScene, pAnimStackFb->name_id( ) ),
                     pAnimLayerFb->anim_stack_idx( ),
                     GetCStringProperty( pSrcScene, pAnimLayerFb->name_id( ) ) );

            SceneAnimLayerId animLayerId;
            animLayerId.AnimLayerIndex = pAnimLayerFb->anim_stack_idx( );
            animLayerId.AnimStackIndex = pAnimLayerFb->anim_stack_id( );

//            auto &animTransformFrame = pScene->AnimLayerIdToTransformFrames[ animLayerId.AnimLayerCompositeId ];
//            pScene->InitializeTransformFrame( animTransformFrame );
//
//            const SceneNodeTransformFrame &bindPoseFrame = pScene->BindPoseFrame;
//            animTransformFrame = bindPoseFrame;
        }
    }

    if ( IsNotNullAndNotEmpty( pMeshesFb ) ) {
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
            assert( IsNotNullAndNotEmpty( pMeshFb->vertices( ) ) );
            assert( IsNotNullAndNotEmpty( pMeshFb->indices( ) ) );
            assert( IsNotNullAndNotEmpty( pMeshFb->subsets( ) ) );
            assert( IsNotNullAndNotEmpty( pMeshFb->submeshes( ) ) );

            // std::array<apemode::detail::SkinnedVertex, 128> sv;
            // memcpy(sv.data(), pMeshFb->vertices(), std::min<size_t>(sizeof(sv), pMeshFb->vertices()->size()));

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

            switch ( pSubmeshFb->vertex_format( ) ) {
                case apemodefb::EVertexFormatFb_Packed:
                    mesh.eVertexType = detail::eVertexType_Packed;
                    break;
                case apemodefb::EVertexFormatFb_PackedSkinned:
                    mesh.eVertexType = detail::eVertexType_PackedSkinned;
                    break;
                case apemodefb::EVertexFormatFb_Static:
                    mesh.eVertexType = detail::eVertexType_Default;
                    break;
                case apemodefb::EVertexFormatFb_StaticSkinned:
                    mesh.eVertexType = detail::eVertexType_Skinned;
                    break;
                case apemodefb::EVertexFormatFb_StaticSkinned8:
                    mesh.eVertexType = detail::eVertexType_Skinned8;
                    break;
                default:
                    mesh.eVertexType = detail::eVertexType_Custom;
                    break;
            }

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

    if ( IsNotNullAndNotEmpty( pSkinsFb ) ) {
        pScene->Skins.resize( pSkinsFb->size() );

        for ( auto &mesh : pScene->Meshes ) {
            if ( mesh.SkinId != detail::kInvalidId ) {
                auto &skin = pScene->Skins[ mesh.SkinId ];
                skin.Id = mesh.SkinId;

                auto pSkinFb = pSkinsFb->Get( mesh.SkinId );
                if ( skin.LinkIds.empty( ) && pSkinFb ) {

                    auto pLinkIdsFb = pSkinFb->links_ids( );
                    if ( IsNotNullAndNotEmpty( pLinkIdsFb ) ) {

                        LogInfo( "Processing skin: \"{}\", links {}",
                                 GetCStringProperty( pSrcScene, pSkinFb->name_id( ) ),
                                 pLinkIdsFb->size( ) );

                        auto meshIt = eastl::find_if(pScene->Meshes.begin(), pScene->Meshes.end(), [&]( const SceneMesh & mesh ) {
                            return mesh.SkinId == skin.Id;
                        });
                        auto nodeIt = eastl::find_if(pScene->Nodes.begin(), pScene->Nodes.end(), [&]( const SceneNode & node ) {
                            return node.MeshId == meshIt->Id;
                        });

                        skin.MeshId = meshIt->Id;
                        skin.NodeId = nodeIt->Id;
                        skin.LinkIds.reserve( pLinkIdsFb->size( ) );
                        skin.InvBindPoseMatrices.reserve( pLinkIdsFb->size( ) );

                        for ( uint32_t linkIndex = 0; linkIndex < pLinkIdsFb->size( ); ++linkIndex ) {
                            auto linkNodeId = pLinkIdsFb->Get( linkIndex );
                            LogInfo( "\t+ link {} \"{}\"", linkNodeId, pScene->Nodes[ linkNodeId ].pszName );

                            auto &bindPoseFrame = pScene->BindPoseFrame;
                            auto &linkTransform = bindPoseFrame.Transforms[ linkNodeId ];
                            auto &meshTransform = bindPoseFrame.Transforms[ skin.NodeId ];

                            auto transformLinkFb   = (const XMFLOAT4X4 *) pSkinFb->transform_link_matrices( )->Get( linkIndex );
                            auto transformFb       = (const XMFLOAT4X4 *) pSkinFb->transform_matrices( )->Get( linkIndex );
                            XMMATRIX transformLink = XMLoadFloat4x4( transformLinkFb );
                            XMMATRIX transform     = XMLoadFloat4x4( transformFb );
                            XMMATRIX geometric     = meshTransform.GeometricalMatrix;
                            XMMATRIX invBindPoseMatrix = geometric * transform * XMMatrixInverse( nullptr, transformLink );

                            skin.LinkIds.push_back(linkNodeId);
                            skin.InvBindPoseMatrices.push_back(invBindPoseMatrix);
                        }
                    }
                }
            }
        }
    }

    auto pTexturesFb = pSrcScene->textures( );
    auto pFilesFb    = pSrcScene->files( );

    if ( IsNotNullAndNotEmpty( pMaterialsFb ) ) {
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

                if ( pTextureFb->file_id( ) != -1 ) {
                    auto pFileFb = pFilesFb->Get( pTextureFb->file_id( ) );
                    assert( pFileFb );

                    auto pszFileName = GetCStringProperty( pSrcScene, pFileFb->name_id( ) );
                    assert( pszFileName );

                    LogInfo( "\tTexture: \"{}\" -> {}", pszTexturePropName, pszFileName );
                }
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

void apemode::SceneAnimCurve::GetKeyIndices( float & time, uint32_t &i, uint32_t &j ) const {

    if ( apemode::IsNearlyEqualOrLess( time, TimeMinMaxTotal.x ) ) {
        // Case: before the curve's first key, or on it.
        i = 0;
        j = 0;
    } else if ( apemode::IsNearlyEqualOrGreater( time, TimeMinMaxTotal.y ) ) {
        // Case: after the curve's last key, or on it.
        i = static_cast< uint32_t >( Keys.size( ) ) - 1;
        j = i;
    } else {
        // Case: inside the curve's timeline.
        auto matchOrUpperBoundIt = Keys.lower_bound( time );
        assert( matchOrUpperBoundIt != Keys.end( ) );

        if ( apemode::IsNearlyEqual( matchOrUpperBoundIt->second.Time, time ) ) {
            // Case: exactly on curve's key.
            i = static_cast< uint32_t >( eastl::distance( Keys.cbegin( ), matchOrUpperBoundIt ) );
            j = i;
        } else {
            const auto lowerBoundIt = eastl::prev( matchOrUpperBoundIt );
            const auto upperBoundIt = matchOrUpperBoundIt;
            assert( lowerBoundIt != Keys.end( ) );

            // Case: inside the curve's segment.
            i = static_cast< uint32_t >( eastl::distance( Keys.cbegin( ), lowerBoundIt ) );
            j = static_cast< uint32_t >( eastl::distance( Keys.cbegin( ), upperBoundIt ) );
        }
    }
}

float CubicInterp( const float P0, const float P1, const float P2, const float P3, const float t ) {
#define SQUARED( x ) ( ( x ) * ( x ) )
#define CUBED( x ) ( ( x ) * ( x ) * ( x ) )
    return CUBED( 1 - t ) * P0 + 3 * SQUARED( 1 - t ) * t * P1 + 3 * ( 1 - t ) * SQUARED( t ) * P2 + CUBED( t ) * P3;
#undef SQUARED
#undef CUBED
}

float InterpolateValue( const float time, const SceneAnimCurveKey& a, const SceneAnimCurveKey& b ) {
    switch ( a.eInterpMode ) {
        case apemode::SceneAnimCurveKey::eInterpolationMode_Const:
            return a.Value;

        case apemode::SceneAnimCurveKey::eInterpolationMode_Linear: {
            const float l = ( time - a.Time ) / ( b.Time - a.Time );
            const float t = ( b.Value - a.Value ) * l + a.Value;
            return t;
        }

        default: {
            const float l = ( time - a.Time ) / ( b.Time - a.Time );
            const float t = CubicInterp( a.Bez0( ), a.Bez1, a.Bez2, b.PrevBez3( ), l );
            return t;
        }
    }
}

float apemode::SceneAnimCurve::Calculate( float time ) const {

    uint32_t i, j;
    GetKeyIndices( time, i, j );

    if ( i == j ) {
        const SceneAnimCurveKey a = Keys.at( i ).second;
        return a.Value;
    } else {
        const SceneAnimCurveKey a = Keys.at( i ).second;
        const SceneAnimCurveKey b = Keys.at( j ).second;

        const float interpolatedValue = InterpolateValue( time, a, b );
        assert( !isnan( interpolatedValue) );
        return interpolatedValue;
    }
}

uint32_t apemode::utils::MaterialPropertyGetIndex( const uint32_t packed ) {
    const uint32_t valueIndex = ( packed >> 8 ) & 0x0fff;
    return valueIndex;
}

apemodefb::EValueTypeFb apemode::utils::MaterialPropertyGetType( const uint32_t packed ) {
    const uint32_t valueType = packed & 0x000f;
    return apemodefb::EValueTypeFb( valueType );
}

const char *apemode::utils::GetCStringProperty( const apemodefb::SceneFb *pScene, const uint32_t valueId ) {
    assert( pScene && apemodefb::EValueTypeFb_String == MaterialPropertyGetType( valueId ) );
    const uint32_t valueIndex = MaterialPropertyGetIndex( valueId );
    return pScene->string_values( )->Get( valueIndex )->c_str( );
}

bool apemode::utils::GetBoolProperty( const apemodefb::SceneFb *pScene, const uint32_t valueId ) {
    assert( pScene && apemodefb::EValueTypeFb_Bool == MaterialPropertyGetType( valueId ) );
    const uint32_t valueIndex = MaterialPropertyGetIndex( valueId );
    return pScene->bool_values( )->Get( valueIndex );
}

float apemode::utils::GetScalarProperty( const apemodefb::SceneFb *pScene, const uint32_t valueId ) {
    assert( pScene && apemodefb::EValueTypeFb_Float == MaterialPropertyGetType( valueId ) );
    const uint32_t valueIndex = MaterialPropertyGetIndex( valueId );
    return pScene->float_values( )->Get( valueIndex );
}

apemodefb::Vec2Fb apemode::utils::GetVec2Property( const apemodefb::SceneFb *pScene, const uint32_t valueId ) {
    assert( pScene && apemodefb::EValueTypeFb_Float2 == MaterialPropertyGetType( valueId ) );
    const uint32_t valueIndex = MaterialPropertyGetIndex( valueId );
    return apemodefb::Vec2Fb( pScene->float_values( )->Get( valueIndex ), pScene->float_values( )->Get( valueIndex + 1 ) );
}

apemodefb::Vec3Fb apemode::utils::GetVec3Property( const apemodefb::SceneFb *pScene, const uint32_t valueId ) {
    assert( pScene && apemodefb::EValueTypeFb_Float3 == MaterialPropertyGetType( valueId ) );
    const uint32_t valueIndex = MaterialPropertyGetIndex( valueId );
    return apemodefb::Vec3Fb( pScene->float_values( )->Get( valueIndex ),
                              pScene->float_values( )->Get( valueIndex + 1 ),
                              pScene->float_values( )->Get( valueIndex + 2 ) );
}

apemodefb::Vec4Fb apemode::utils::GetVec4Property( const apemodefb::SceneFb *pScene,
                                                   const uint32_t            valueId,
                                                   const float               defaultW ) {
    assert( pScene && ( apemodefb::EValueTypeFb_Float3 == MaterialPropertyGetType( valueId ) ||
                        apemodefb::EValueTypeFb_Float4 == MaterialPropertyGetType( valueId ) ) );
    const uint32_t valueIndex = MaterialPropertyGetIndex( valueId );
    return apemodefb::Vec4Fb( pScene->float_values( )->Get( valueIndex ),
                              pScene->float_values( )->Get( valueIndex + 1 ),
                              pScene->float_values( )->Get( valueIndex + 2 ),
                              apemodefb::EValueTypeFb_Float4 == MaterialPropertyGetType( valueId )
                                  ? pScene->float_values( )->Get( valueIndex + 3 )
                                  : defaultW );
}
