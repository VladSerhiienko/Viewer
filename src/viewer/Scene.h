
#include <map>
#include <vector>
#include <memory>

#include <flatbuffers/flatbuffers.h>
#include <flatbuffers/util.h>

#include <scene_generated.h>
#include <MathInc.h>

namespace apemode {

    struct SceneDeviceAsset {
        virtual ~SceneDeviceAsset( ) = default;
    };

    struct SceneMaterial {
        SceneDeviceAsset *pDeviceAsset = nullptr;
        float             AlphaCutoff  = 0;
        bool              bDoubleSided = false;
        XMFLOAT3          EmissiveFactor;
        XMFLOAT4          BaseColorFactor;
        float             MetallicFactor  = 0;
        float             RoughnessFactor = 0;
    };

    struct SceneMeshSubset {
        uint32_t MaterialId = -1;
        uint32_t BaseIndex  = 0;
        uint32_t IndexCount = 0;
    };

    struct SceneMesh {
        SceneDeviceAsset *pDeviceAsset = nullptr;
        uint32_t          BaseSubset   = -1;
        uint32_t          SubsetCount  = 0;
        XMFLOAT3          PositionOffset;
        XMFLOAT3          PositionScale;
        XMFLOAT2          TexcoordOffset;
        XMFLOAT2          TexcoordScale;
    };

    /* Transfrom class that stores main FBX SDK transform properties
     * and calculates local and geometric matrices.
     */
    struct SceneNodeTransform {

        XMFLOAT3 Translation;
        XMFLOAT3 RotationOffset;
        XMFLOAT3 RotationPivot;
        XMFLOAT3 PreRotation;
        XMFLOAT3 PostRotation;
        XMFLOAT3 Rotation;
        XMFLOAT3 ScalingOffset;
        XMFLOAT3 ScalingPivot;
        XMFLOAT3 Scaling;
        XMFLOAT3 GeometricTranslation;
        XMFLOAT3 GeometricRotation;
        XMFLOAT3 GeometricScaling;

        /**
         * Checks for nans and zero scales.
         * @return True if valid, false otherwise.
         **/
        bool Validate( ) const;

        /**
         * Calculates local matrix.
         * @note Contributes node world matrix.
         * @return Node local matrix.
         **/
        XMMATRIX CalculateLocalMatrix( ) const;

        /**
         * Calculate geometric transform.
         * @note That is an object-offset 3ds Max concept,
         *       it does not influence scene hierarchy, though
         *       it contributes to node world transform.
         * @return Node geometric transform.
         **/
        XMMATRIX CalculateGeometricMatrix( ) const;
    };

    struct SceneNode {
        SceneDeviceAsset *      pDeviceAsset = nullptr;
        uint32_t                Id           = -1;
        uint32_t                ParentId     = -1;
        uint32_t                MeshId       = -1;
        std::vector< uint32_t > ChildIds;
    };

    struct Scene {
        SceneDeviceAsset *pDeviceAsset = nullptr;

        //
        // Scene components
        //

        std::vector< SceneNode >          Nodes;
        std::vector< SceneNodeTransform > Transforms;
        std::vector< SceneMesh >          Meshes;
        std::vector< SceneMeshSubset >    Subsets;
        std::vector< SceneMaterial >      Materials;

        //
        // Transform matrices storage.
        //

        std::vector< XMMATRIX > WorldMatrices;
        std::vector< XMMATRIX > LocalMatrices;
        std::vector< XMMATRIX > GeometricalMatrices;
        std::vector< XMMATRIX > HierarchicalMatrices;

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

    using UniqueScenePtr     = typename std::unique_ptr< Scene >;
    using UniqueScenePtrPair = typename std::pair< std::unique_ptr< Scene >, const apemodefb::SceneFb * >;

    /* Loads scene from loaded .FBXP file
     * @return A pair of pointers: (1) a unique pointer to scene, (2) a pointer to a source scene
     * @note A pointer to source scene reinterprets the data from .FBXP file and depends on it.
     */
    UniqueScenePtrPair LoadSceneFromBin( const uint8_t *pData, size_t dataSize );
}