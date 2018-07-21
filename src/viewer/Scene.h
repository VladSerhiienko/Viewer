#pragma once

#include <MemoryManager.h>

#include <map>
#include <vector>
#include <memory>

#include <flatbuffers/flatbuffers.h>
#include <flatbuffers/util.h>

#include <scene_generated.h>
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

    static constexpr uint32_t kInvalidId = ~0U;

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

        uint32_t SrcId = kInvalidId;
        SceneDeviceAssetPtr pDeviceAsset;

        XMFLOAT4   BaseColorFactor = XMFLOAT4{0, 0, 0, 1};
        XMFLOAT3   EmissiveFactor  = XMFLOAT3{0, 0, 0};
        float      MetallicFactor  = 0;
        float      RoughnessFactor = 0;
        float      AlphaCutoff     = 0;
        bool       bDoubleSided    = false;
        EAlphaMode eAlphaMode      = eAlphaMode_Solid;
    };

    /* Represents the mesh subset class.
     * Indicates a part of mesh and a material id for the pointed part of a mesh.
     */
    struct SceneMeshSubset {
        uint32_t MaterialId = kInvalidId;
        uint32_t BaseIndex  = 0;
        uint32_t IndexCount = 0;
    };

    struct SceneMesh {
        SceneDeviceAssetPtr pDeviceAsset;

        uint32_t            SrcId          = kInvalidId;
        uint32_t            SkinId         = kInvalidId;
        uint32_t            BaseSubset     = kInvalidId;
        uint32_t            SubsetCount    = 0;
        XMFLOAT3            PositionOffset = XMFLOAT3{0, 0, 0};
        XMFLOAT3            PositionScale  = XMFLOAT3{1, 1, 1};
        XMFLOAT2            TexcoordOffset = XMFLOAT2{0, 0};
        XMFLOAT2            TexcoordScale  = XMFLOAT2{1, 1};
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
        enum EProperty {
            eProperty_LclTranslation = 0,
            eProperty_RotationOffset,
            eProperty_RotationPivot,
            eProperty_PreRotation,
            eProperty_PostRotation,
            eProperty_LclRotation,
            eProperty_ScalingOffset,
            eProperty_ScalingPivot,
            eProperty_LclScaling,
            eProperty_GeometricTranslation,
            eProperty_GeometricRotation,
            eProperty_GeometricScaling,
            ePropertyCount,
        };

        enum EChannel {
            eChannel_X = 0,
            eChannel_Y,
            eChannel_Z,
            eChannelCount,
        };

        SceneDeviceAssetPtr         pDeviceAsset;
        uint32_t                    Id          = kInvalidId;
        uint32_t                    AnimStackId = kInvalidId;
        uint32_t                    AnimLayerId = kInvalidId;
        EProperty                   eProp       = ePropertyCount;
        EChannel                    eChannel    = eChannelCount;
        apemode::vector< XMFLOAT2 > Keys;
    };

    struct SceneSkin {
        apemode::vector< uint32_t > LinkIds;
    };

    struct SceneNode {
        SceneDeviceAssetPtr         pDeviceAsset;
        uint32_t                    Id       = kInvalidId;
        uint32_t                    ParentId = kInvalidId;
        uint32_t                    MeshId   = kInvalidId;
        apemode::vector< uint32_t > ChildIds;
        apemode::vector< uint32_t > AnimCurveIds;
    };

    struct Scene {
        SceneDeviceAssetPtr pDeviceAsset;

        //
        // Scene components
        //

        apemode::vector< SceneNode >          Nodes;
        apemode::vector< SceneNodeTransform > Transforms;
        apemode::vector< SceneMesh >          Meshes;
        apemode::vector< SceneMeshSubset >    Subsets;
        apemode::vector< SceneMaterial >      Materials;
        apemode::vector< SceneAnimCurve >     AnimCurves;
        apemode::vector< SceneSkin >          Skins;

        //
        // Transform matrices storage.
        //

        apemode::vector< XMMATRIX > WorldMatrices;
        apemode::vector< XMMATRIX > LocalMatrices;
        apemode::vector< XMMATRIX > GeometricalMatrices;
        apemode::vector< XMMATRIX > HierarchicalMatrices;

        /* Internal usage only.
         * @see UpdateMatrices().
         * @todo Safety assertions.
         */
        void UpdateChildWorldMatrices( uint32_t nodeId );

        /* Update matrices storage with up-to-date values.
         * @todo Non-recursive, no dynamic memory?
         *       Currently, I tried to avoid any stack memory pollution,
         *       tried to avoid any local variable but kept it clean.
         */
        void UpdateMatrices( );

        template < typename TNodeIdCallback >
        void ForEachNode( uint32_t nodeId, TNodeIdCallback callback ) {
            callback( nodeId );
            for ( auto &childId : Nodes[ nodeId ].ChildIds ) {
                ForEachNode( childId, callback );
            }
        }

        template < typename TNodeIdCallback >
        void ForEachNode( TNodeIdCallback callback ) {
            constexpr uint32_t rootNodeId = 0;
            ForEachNode( rootNodeId, callback );
        }
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

    inline uint32_t MaterialPropertyGetIndex( uint32_t packed ) {
        const uint32_t valueIndex = ( packed >> 8 ) & 0x0fff;
        return valueIndex;
    }

    inline apemodefb::EValueTypeFb MaterialPropertyGetType( uint32_t packed ) {
        const uint32_t valueType = packed & 0x000f;
        return apemodefb::EValueTypeFb( valueType );
    }

    template < typename T /*, bool bAssert = false */ >
    inline bool FlatbuffersTVectorIsNotNullAndNotEmptyAssert( const flatbuffers::Vector< T > *pVector ) {
        /* assert( !bAssert || ( pVector && pVector->size( ) ) ); */
        return pVector && pVector->size( );
    }

    template < typename T >
    inline typename flatbuffers::Vector< T >::return_type FlatbuffersTVectorGetAtIndex( const flatbuffers::Vector< T > *pVector,
                                                                                        const size_t itemIndex ) {
        assert( pVector );
        const size_t vectorSize = pVector->size( );

        assert( vectorSize > itemIndex );
        return pVector->operator[]( static_cast< flatbuffers::uoffset_t >( itemIndex ) );
    }

    inline std::string GetStringProperty( const apemodefb::SceneFb *pScene, uint32_t valueId ) {
        assert( pScene );
        assert( apemodefb::EValueTypeFb_String == MaterialPropertyGetType( valueId ) );

        const uint32_t valueIndex = MaterialPropertyGetIndex( valueId );
        return FlatbuffersTVectorGetAtIndex( pScene->string_values( ), valueIndex )->str( );
    }

    inline const char *GetCStringProperty( const apemodefb::SceneFb *pScene, uint32_t valueId ) {
        assert( pScene );
        assert( apemodefb::EValueTypeFb_String == MaterialPropertyGetType( valueId ) );

        const uint32_t valueIndex = MaterialPropertyGetIndex( valueId );
        return FlatbuffersTVectorGetAtIndex( pScene->string_values( ), valueIndex )->c_str( );
    }

    inline bool GetBoolProperty( const apemodefb::SceneFb *pScene, uint32_t valueId ) {
        assert( pScene );
        assert( apemodefb::EValueTypeFb_Bool == MaterialPropertyGetType( valueId ) );

        const uint32_t valueIndex = MaterialPropertyGetIndex( valueId );
        return FlatbuffersTVectorGetAtIndex( pScene->bool_values( ), valueIndex );
    }

    inline float GetScalarProperty( const apemodefb::SceneFb *pScene, uint32_t valueId ) {
        assert( pScene );
        assert( apemodefb::EValueTypeFb_Float == MaterialPropertyGetType( valueId ) );

        const uint32_t valueIndex = MaterialPropertyGetIndex( valueId );
        return FlatbuffersTVectorGetAtIndex( pScene->float_values( ), valueIndex );
    }

    inline XMFLOAT2 GetVec2Property( const apemodefb::SceneFb *pScene, uint32_t valueId ) {
        assert( pScene );

        const auto valueType = MaterialPropertyGetType( valueId );
        assert( apemodefb::EValueTypeFb_Float2 == valueType );

        const uint32_t valueIndex = MaterialPropertyGetIndex( valueId );
        return XMFLOAT2( FlatbuffersTVectorGetAtIndex( pScene->float_values( ), valueIndex ),
                         FlatbuffersTVectorGetAtIndex( pScene->float_values( ), valueIndex + 1 ) );
    }

    inline XMFLOAT3 GetVec3Property( const apemodefb::SceneFb *pScene, uint32_t valueId ) {
        assert( pScene );

        const auto valueType = MaterialPropertyGetType( valueId );
        assert( apemodefb::EValueTypeFb_Float3 == valueType );

        const uint32_t valueIndex = MaterialPropertyGetIndex( valueId );
        return XMFLOAT3( FlatbuffersTVectorGetAtIndex( pScene->float_values( ), valueIndex ),
                         FlatbuffersTVectorGetAtIndex( pScene->float_values( ), valueIndex + 1 ),
                         FlatbuffersTVectorGetAtIndex( pScene->float_values( ), valueIndex + 2 ) );
    }

    inline XMFLOAT4 GetVec4Property( const apemodefb::SceneFb *pScene, uint32_t valueId, float defaultW = 1.0f ) {
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
}