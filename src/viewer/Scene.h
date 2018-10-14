#pragma once

#include <flatbuffers/flatbuffers.h>
#include <flatbuffers/util.h>
#include <scene_generated.h>

#include <MathInc.h>
#include <apemode/platform/memory/MemoryManager.h>

namespace apemode {

/* Additional types for Scene classes.
 */
namespace detail {

/* Base class for device asset.
 * Allows renderer to assign concrete device resources to scene items (meshes).
 */
struct SceneDeviceAsset {
    virtual ~SceneDeviceAsset( ) = default;
};

/* Shortcut for the device asset pointer.
 */
using SceneDeviceAssetPtr = apemode::unique_ptr< SceneDeviceAsset >;

static constexpr uint32_t kInvalidId   = std::numeric_limits< uint32_t >::max( );
static constexpr uint16_t kInvalidId16 = std::numeric_limits< uint16_t >::max( );

enum EVertexType {
    eVertexType_Custom  = 0,
    eVertexType_Default = 1,
    eVertexType_Skinned,
    eVertexType_Packed,
    eVertexType_PackedSkinned,
    eVertexTypeCount,
};

/* Default vertex structure.
 */
struct DefaultVertex {
    XMFLOAT3 position;
    XMFLOAT3 normal;
    XMFLOAT4 tangent;
    XMFLOAT2 texcoords;
};

/* Skinned vertex structure.
 */
struct SkinnedVertex {
    XMFLOAT3 position;
    XMFLOAT3 normal;
    XMFLOAT4 tangent;
    XMFLOAT2 texcoords;
    XMFLOAT4 weights;
    XMFLOAT4 indices;
};

/* Default vertex structure with packing.
 */
struct PackedVertex {
    uint32_t position;
    uint32_t normal;
    uint32_t tangent;
    uint32_t texcoords;
};

/* Skinned vertex structure with packing.
 */
struct PackedSkinnedVertex {
    uint32_t position;
    uint32_t normal;
    uint32_t tangent;
    uint32_t weights;
    uint32_t indices;
};
} // namespace detail

/* Represents the material asset class.
 */
struct SceneMaterialAsset {
    const uint8_t *pBufferData = nullptr;
    size_t         BufferSize  = 0;
};

/* Represents the material class.
 */
struct SceneMaterial {
    enum EAlphaMode { eAlphaMode_Solid, eAlphaMode_Blend };

    detail::SceneDeviceAssetPtr pDeviceAsset;

    uint32_t   Id              = detail::kInvalidId;
    XMFLOAT4   BaseColorFactor = XMFLOAT4{0, 0, 0, 1};
    XMFLOAT3   EmissiveFactor  = XMFLOAT3{0, 0, 0};
    float      MetallicFactor  = 0;
    float      RoughnessFactor = 0;
    float      AlphaCutoff     = 0;
    EAlphaMode eAlphaMode      = eAlphaMode_Solid;
    bool       bDoubleSided    = false;
};

/* Represents the mesh subset class.
 * Indicates a part of mesh and a material id for the pointed part of a mesh.
 */
struct SceneMeshSubset {
    uint32_t MeshId     = detail::kInvalidId;
    uint32_t MaterialId = detail::kInvalidId;
    uint32_t BaseIndex  = 0;
    uint32_t IndexCount = 0;
};

struct SceneMesh {
    detail::SceneDeviceAssetPtr pDeviceAsset;

    uint32_t Id             = detail::kInvalidId;
    uint32_t SkinId         = detail::kInvalidId;
    uint32_t BaseSubset     = detail::kInvalidId;
    uint32_t SubsetCount    = 0;
    XMFLOAT3 PositionOffset = XMFLOAT3{0, 0, 0};
    XMFLOAT3 PositionScale  = XMFLOAT3{1, 1, 1};
    XMFLOAT2 TexcoordOffset = XMFLOAT2{0, 0};
    XMFLOAT2 TexcoordScale  = XMFLOAT2{1, 1};
};

/* Transfrom class that stores main FBX SDK transform properties
 * and calculates local and geometric matrices.
 */
struct SceneNodeTransform {
    XMFLOAT3 Translation          = XMFLOAT3{0, 0, 0};
    XMFLOAT3 RotationOffset       = XMFLOAT3{0, 0, 0};
    XMFLOAT3 RotationPivot        = XMFLOAT3{0, 0, 0};
    XMFLOAT3 PreRotation          = XMFLOAT3{0, 0, 0};
    XMFLOAT3 PostRotation         = XMFLOAT3{0, 0, 0};
    XMFLOAT3 Rotation             = XMFLOAT3{0, 0, 0};
    XMFLOAT3 ScalingOffset        = XMFLOAT3{0, 0, 0};
    XMFLOAT3 ScalingPivot         = XMFLOAT3{0, 0, 0};
    XMFLOAT3 Scaling              = XMFLOAT3{0, 0, 0};
    XMFLOAT3 GeometricTranslation = XMFLOAT3{0, 0, 0};
    XMFLOAT3 GeometricRotation    = XMFLOAT3{0, 0, 0};
    XMFLOAT3 GeometricScaling     = XMFLOAT3{0, 0, 0};

    /* Checks for nans and zero scales.
     * @return True if valid, false otherwise.
     */
    bool Validate( ) const;

    /* Calculates local matrix.
     * @note Contributes node world matrix.
     * @return Node local matrix.
     */
    XMMATRIX CalculateLocalMatrix( ) const;

    /* Calculate geometric transform.
     * @note That is an object-offset 3ds Max concept,
     *       it does not influence scene hierarchy, though
     *       it contributes to node world transform.
     * @return Node geometric transform.
     */
    XMMATRIX CalculateGeometricMatrix( ) const;
};

/* SceneAnimCurvKey class stores time, value, arrive and leave tangents (tangents are only for cubic keys).
 */
struct SceneAnimCurveKey {
    float Time          = 0.0f;
    float Value         = 0.0f;
    float ArriveTangent = 0.0f;
    float LeaveTangent  = 0.0f;
};

/* SceneAnimCurve class stores curve parameters and time-value keys.
 */
struct SceneAnimCurve {

    /* Animation curve channels X, Y and Z.
     */
    enum EChannel {
        eChannel_X = 0,
        eChannel_Y,
        eChannel_Z,
        eChannelCount,
    };

    /* Animation properties, each has 3 channels X, Y and Z.
     * Each property defines 3 possible animation curves (1 for each channel).
     * Each property has a correspondent 3D vector in the SceneNodeTransform structure.
     */
    enum EProperty {
        eProperty_Translation          = 0,                                              // 0
        eProperty_RotationOffset       = eProperty_Translation + eChannelCount,          // 3
        eProperty_RotationPivot        = eProperty_RotationOffset + eChannelCount,       // 6
        eProperty_PreRotation          = eProperty_RotationPivot + eChannelCount,        // 9
        eProperty_PostRotation         = eProperty_PreRotation + eChannelCount,          // 12
        eProperty_LclRotation          = eProperty_PostRotation + eChannelCount,         // 15
        eProperty_ScalingOffset        = eProperty_LclRotation + eChannelCount,          // 18
        eProperty_ScalingPivot         = eProperty_ScalingOffset + eChannelCount,        // 21
        eProperty_Scaling              = eProperty_ScalingPivot + eChannelCount,         // 24
        eProperty_GeometricTranslation = eProperty_Scaling + eChannelCount,              // 27
        eProperty_GeometricRotation    = eProperty_GeometricTranslation + eChannelCount, // 30
        eProperty_GeometricScaling     = eProperty_GeometricRotation + eChannelCount,    // 33
        ePropertyCount                 = eProperty_GeometricScaling + eChannelCount,     // 36
    };

    detail::SceneDeviceAssetPtr pDeviceAsset;

    uint32_t  Id              = detail::kInvalidId;
    uint16_t  AnimStackId     = detail::kInvalidId16;
    uint16_t  AnimLayerId     = detail::kInvalidId16;
    EProperty eProperty       = ePropertyCount;
    EChannel  eChannel        = eChannelCount;
    XMFLOAT3  TimeMinMaxTotal = XMFLOAT3{0, 0, 0};

    /* Animation keys, each is the 2D vector where X stands for time and Y stands for value.
     */
    apemode::vector_map< float, SceneAnimCurveKey > Keys;

    /* Assigns two indices of the keys for the given time value, loops the time value.
     * The two key indices can be used for accessing actual animation keys' values and interpolating between them.
     */
    void GetKeyIndices( float &time, bool bLoop, uint32_t &i, uint32_t &j ) const;

    /* Returns interpolated curve's value for the given time value.
     */
    float Calculate( float time, bool bLoop ) const;
};

/* SceneNodeAnimCurveIds class contains animation curve IDs (for each channel of each property).
 */
struct SceneNodeAnimCurveIds {
    uint32_t AnimCurveIds[ SceneAnimCurve::ePropertyCount ] = {
        detail::kInvalidId, detail::kInvalidId, detail::kInvalidId, detail::kInvalidId, detail::kInvalidId, detail::kInvalidId, detail::kInvalidId, detail::kInvalidId, detail::kInvalidId,
        detail::kInvalidId, detail::kInvalidId, detail::kInvalidId, detail::kInvalidId, detail::kInvalidId, detail::kInvalidId, detail::kInvalidId, detail::kInvalidId, detail::kInvalidId,
        detail::kInvalidId, detail::kInvalidId, detail::kInvalidId, detail::kInvalidId, detail::kInvalidId, detail::kInvalidId, detail::kInvalidId, detail::kInvalidId, detail::kInvalidId,
        detail::kInvalidId, detail::kInvalidId, detail::kInvalidId, detail::kInvalidId, detail::kInvalidId, detail::kInvalidId, detail::kInvalidId, detail::kInvalidId, detail::kInvalidId,
    };
};

/* SceneSkinLink class contains the node ID (link ID) and its inverse bind pose matrix.
 */
struct SceneSkinLink {
    uint32_t LinkId = detail::kInvalidId;
    XMMATRIX InvBindPoseMatrix;
};

struct SceneSkin {
    apemode::vector< SceneSkinLink > Links;
};

struct SceneNode {
    detail::SceneDeviceAssetPtr pDeviceAsset;

    uint32_t Id       = detail::kInvalidId;
    uint32_t ParentId = detail::kInvalidId;
    uint32_t MeshId   = detail::kInvalidId;
};

/* SceneNodeTransformComposite class contains node transformation properties and calculated matrices.
 */
struct SceneNodeTransformComposite {
    SceneNodeTransform Properties;
    XMMATRIX           WorldMatrix;
    XMMATRIX           LocalMatrix;
    XMMATRIX           GeometricalMatrix;
    XMMATRIX           HierarchicalMatrix;
};

/* SceneNodeTransformFrame class contains transforms of the scene nodes.
 */
struct SceneNodeTransformFrame {
    apemode::vector< SceneNodeTransformComposite > Transforms;
};

/* Scene class contains nodes, meshes, materials, animation curves and transform frames.
 */
struct Scene {
    detail::SceneDeviceAssetPtr pDeviceAsset;

    //
    // Scene components
    //

    apemode::vector< SceneNode >                   Nodes;
    apemode::vector< SceneMesh >                   Meshes;
    apemode::vector< SceneMeshSubset >             Subsets;
    apemode::vector< SceneMaterial >               Materials;
    apemode::vector_multimap< uint32_t, uint32_t > NodeToChildIds;
    SceneNodeTransformFrame                        BindPoseFrame;

    apemode::vector< SceneSkin >                             Skins;
    apemode::vector< SceneAnimCurve >                        AnimCurves;
    apemode::vector_multimap< uint32_t, uint32_t >           NodeIdToAnimCurveIds;
    apemode::vector_map< uint64_t, SceneNodeAnimCurveIds >   AnimNodeIdToAnimCurveIds;
    apemode::vector_map< uint32_t, SceneNodeTransformFrame > AnimLayerIdToTransformFrames;

    //
    // Transform matrices storage.
    //

    /* Updates transform frame.
     */
    void UpdateTransformMatrices( uint32_t nodeId, SceneNodeTransformFrame &transformFrame ) const;

    /* Updates transform frame.
     */
    void UpdateTransformMatrices( SceneNodeTransformFrame &transformFrame ) const;

    /* Initializes transform frame.
     */
    void InitializeTransformFrame( SceneNodeTransformFrame &transformFrame ) const;

    /* Animates transform frame, returns it.
     */
    SceneNodeTransformFrame &UpdateTransformProperties( float time, bool bLoop, uint16_t animStackId, uint16_t animLayerId );

    /* Returns animated transform frame.
     */
    SceneNodeTransformFrame &GetBindPoseTransformFrame( );

    /* Returns animated transform frame.
     */
    const SceneNodeTransformFrame &GetBindPoseTransformFrame( ) const;

    /* Returns animated transform frame.
     */
    SceneNodeTransformFrame &GetAnimatedTransformFrame( uint16_t animStackId, uint16_t animLayerId );

    /* Returns animated transform frame.
     */
    const SceneNodeTransformFrame &GetAnimatedTransformFrame( uint16_t animStackId, uint16_t animLayerId ) const;

    /* Returns animation curves for the node.
     */
    const SceneNodeAnimCurveIds &GetAnimCurveIds( uint32_t nodeId, uint16_t animStackId, uint16_t animLayerId ) const;

    /* Calculates skin matrix palette.
     */
    void UpdateSkinMatrices( const SceneSkin &              skin,
                             const SceneNodeTransformFrame &animatedFrame,
                             apemode::vector< XMMATRIX > &  skinMatrices );
};

/* Represents the loaded scene.
 */
struct LoadedScene {
    apemode::vector< uint8_t >   FileContents; /**! The contents of the scene file. */
    const apemodefb::SceneFb *   pSrcScene;    /**! Is valid as long as FileContents. Make sure FileContents outlives it. */
    apemode::unique_ptr< Scene > pScene;       /**! The scene info. */
};

/* Loads scene from the contents of the FbxPipeline's exported scene file.
 */
LoadedScene LoadSceneFromBin( apemode::vector< uint8_t > &&fileContents );

namespace utils {

uint32_t                MaterialPropertyGetIndex( const uint32_t packed );
apemodefb::EValueTypeFb MaterialPropertyGetType( const uint32_t packed );
const char *            GetCStringProperty( const apemodefb::SceneFb *pScene, const uint32_t valueId );
bool                    GetBoolProperty( const apemodefb::SceneFb *pScene, const uint32_t valueId );
float                   GetScalarProperty( const apemodefb::SceneFb *pScene, const uint32_t valueId );
apemodefb::Vec2Fb       GetVec2Property( const apemodefb::SceneFb *pScene, const uint32_t valueId );
apemodefb::Vec3Fb       GetVec3Property( const apemodefb::SceneFb *pScene, const uint32_t valueId );
apemodefb::Vec4Fb       GetVec4Property( const apemodefb::SceneFb *pScene, const uint32_t valueId, const float defaultW = 1.0f );

} // namespace utils

} // namespace apemode