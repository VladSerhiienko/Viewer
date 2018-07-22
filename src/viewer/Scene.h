#pragma once

#include <FbxPipelineSceneUtils.h>
#include <MemoryManager.h>
#include <MathInc.h>

namespace apemode {

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

        SceneDeviceAssetPtr pDeviceAsset;

        uint32_t   Id              = kInvalidId;
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
        uint32_t MeshId     = kInvalidId;
        uint32_t MaterialId = kInvalidId;
        uint32_t BaseIndex  = 0;
        uint32_t IndexCount = 0;
    };

    struct SceneMesh {
        SceneDeviceAssetPtr pDeviceAsset;

        uint32_t Id             = kInvalidId;
        uint32_t SkinId         = kInvalidId;
        uint32_t BaseSubset     = kInvalidId;
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

    /* SceneAnimCurve class stores curve parameters and time-value keys.
     */
    struct SceneAnimCurve {

        enum EChannel {
            eChannel_X = 0,
            eChannel_Y,
            eChannel_Z,
            eChannelCount,
        };

        enum EProperty {
            eProperty_Translation          = 0,
            eProperty_RotationOffset       = eProperty_Translation          + eChannelCount,
            eProperty_RotationPivot        = eProperty_RotationOffset       + eChannelCount,
            eProperty_PreRotation          = eProperty_RotationPivot        + eChannelCount,
            eProperty_PostRotation         = eProperty_PreRotation          + eChannelCount,
            eProperty_LclRotation          = eProperty_PostRotation         + eChannelCount,
            eProperty_ScalingOffset        = eProperty_LclRotation          + eChannelCount,
            eProperty_ScalingPivot         = eProperty_ScalingOffset        + eChannelCount,
            eProperty_Scaling              = eProperty_ScalingPivot         + eChannelCount,
            eProperty_GeometricTranslation = eProperty_Scaling              + eChannelCount,
            eProperty_GeometricRotation    = eProperty_GeometricTranslation + eChannelCount,
            eProperty_GeometricScaling     = eProperty_GeometricRotation    + eChannelCount,
            ePropertyCount                 = eProperty_GeometricScaling     + eChannelCount,
        };

        SceneDeviceAssetPtr pDeviceAsset;

        uint32_t  Id              = kInvalidId;
        uint16_t  AnimStackId     = kInvalidId16;
        uint16_t  AnimLayerId     = kInvalidId16;
        EProperty eProperty       = ePropertyCount;
        EChannel  eChannel        = eChannelCount;
        XMFLOAT3  TimeMinMaxTotal = XMFLOAT3{0, 0, 0};

        apemode::vector< XMFLOAT2 > Keys;

        void  GetKeyIndices( float time, bool bLoop, uint32_t &i, uint32_t &j ) const;
        float Calculate( float time, bool bLoop ) const;
    };

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

    struct SceneNodeAnimCurveIds {
        uint32_t AnimCurveIds[ SceneAnimCurve::ePropertyCount ] = {
            kInvalidId, kInvalidId, kInvalidId, kInvalidId, kInvalidId, kInvalidId, kInvalidId, kInvalidId, kInvalidId,
            kInvalidId, kInvalidId, kInvalidId, kInvalidId, kInvalidId, kInvalidId, kInvalidId, kInvalidId, kInvalidId,
            kInvalidId, kInvalidId, kInvalidId, kInvalidId, kInvalidId, kInvalidId, kInvalidId, kInvalidId, kInvalidId,
            kInvalidId, kInvalidId, kInvalidId, kInvalidId, kInvalidId, kInvalidId, kInvalidId, kInvalidId, kInvalidId,
        };
    };

    struct SceneSkinLink {
        uint32_t LinkId;
        XMMATRIX InvBindPose;
    };

    struct SceneSkin {
        apemode::vector< SceneSkinLink > Links;
    };

    struct SceneNode {
        SceneDeviceAssetPtr pDeviceAsset;

        uint32_t Id       = kInvalidId;
        uint32_t ParentId = kInvalidId;
        uint32_t MeshId   = kInvalidId;
    };

    struct SceneNodeTransformComposite {
        SceneNodeTransform Properties;
        XMMATRIX           WorldMatrix;
        XMMATRIX           LocalMatrix;
        XMMATRIX           GeometricalMatrix;
        XMMATRIX           HierarchicalMatrix;
    };

    struct SceneNodeTransformFrame {
        apemode::vector< SceneNodeTransformComposite > Transforms;
    };

    struct Scene {
        SceneDeviceAssetPtr pDeviceAsset;

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
        SceneNodeTransformFrame &UpdateTransformProperties( float    time,
                                                            bool     bLoop,
                                                            uint16_t animStackId,
                                                            uint16_t animLayerId );

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

    struct LoadedScene {
        apemode::vector< uint8_t >   FileContents; /**! The contents of the scene file. */
        const apemodefb::SceneFb *   pSrcScene;    /**! Is valid as long as FileContents. Make sure FileContents outlives it. */
        apemode::unique_ptr< Scene > pScene;       /**! The scene info. */
    };

    /* Loads scene from loaded .FBXP file
     * @return A pair of pointers: (1) a unique pointer to scene, (2) a pointer to a source scene
     * @note A pointer to source scene reinterprets the data from .FBXP file and depends on it.
     */
    LoadedScene LoadSceneFromBin( apemode::vector< uint8_t > && fileContents );
}