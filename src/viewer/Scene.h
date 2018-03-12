
#include <map>
#include <vector>
#include <memory>

#include <flatbuffers/flatbuffers.h>
#include <flatbuffers/util.h>

#include <scene_generated.h>
#include <MathInc.h>

namespace apemode {

    struct SceneDeviceAsset {
        virtual ~SceneDeviceAsset( ) {
        }
    };

    struct SceneMaterial {
        void *   pDeviceAsset = nullptr;

        float    alphaCutoff;
        bool     bDoubleSided;
        XMFLOAT3 emissiveFactor;
        XMFLOAT4 baseColorFactor;
        float    metallicFactor;
        float    roughnessFactor;

        /*
        Texture *pBaseColorTexture;
        Texture *pNormalTexture;
        Texture *pOcclusionTexture;
        Texture *pEmissiveTexture;
        Texture *pMetallicRoughnessTexture;
        */
    };

    struct SceneMeshSubset {
        uint32_t materialId = -1;
        uint32_t baseIndex  = 0;
        uint32_t indexCount = 0;
    };

    struct SceneMesh {
        void *pDeviceAsset = nullptr;

        std::vector< SceneMeshSubset > subsets;
        XMFLOAT3                       positionOffset;
        XMFLOAT3                       positionScale;
        XMFLOAT2                       texcoordOffset;
        XMFLOAT2                       texcoordScale;
        /*
        Buffer *                       pVertexBuffer = nullptr;
        Buffer *                       pIndexBuffer = nullptr;
        */
    };

    /**
     * Transfrom class that stores main FBX SDK transform properties
     * and calculates local and geometric matrices.
     **/
    struct SceneNodeTransform {

        XMFLOAT3 translation;
        XMFLOAT3 rotationOffset;
        XMFLOAT3 rotationPivot;
        XMFLOAT3 preRotation;
        XMFLOAT3 postRotation;
        XMFLOAT3 rotation;
        XMFLOAT3 scalingOffset;
        XMFLOAT3 scalingPivot;
        XMFLOAT3 scaling;
        XMFLOAT3 geometricTranslation;
        XMFLOAT3 geometricRotation;
        XMFLOAT3 geometricScaling;

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

    class Scene;
    struct SceneNode {
        void *pDeviceAsset = nullptr;

        uint32_t                id       = -1;
        uint32_t                parentId = -1;
        uint32_t                meshId   = -1;
        std::vector< uint32_t > childIds;
        std::vector< uint32_t > materialIds;

        /*
        Buffer *                pBuffer = nullptr;
        */
    };

    class Scene {
    public:
        //
        // Scene components
        //

        std::string               sourceData;
        const apemodefb::SceneFb *sourceScene;

        std::vector< SceneNode >          nodes;
        std::vector< SceneNodeTransform > transforms;
        std::vector< SceneMesh >          meshes;
        std::vector< SceneMaterial >      materials;
        //std::vector< Image * >            images;
        //std::vector< Texture * >          textures;

        //
        // Transform matrices storage.
        //

        std::vector< XMMATRIX > worldMatrices;
        std::vector< XMMATRIX > localMatrices;
        std::vector< XMMATRIX > geometricMatrices;
        std::vector< XMMATRIX > hierarchicalMatrices;

        /**
         * Internal usage only.
         * @see UpdateMatrices().
         * @todo Safety assertions.
         **/
        void UpdateChildWorldMatrices( uint32_t nodeId );

        /**
         * Update matrices storage with up-to-date values.
         * @todo Non-recursive, no dynamic memory?
         *       Currently, I tried to avoid any stack memory pollution,
         *       tried to avoid any local variable but kept it clean.
         **/
        void UpdateMatrices( );

        template < typename TNodeIdCallback >
        void ForEachNode( uint32_t nodeId, TNodeIdCallback callback ) {
            callback( nodeId );
            for ( auto &childId : nodes[ nodeId ].childIds ) {
                ForEachNode( childId, callback );
            }
        }

        template < typename TNodeIdCallback >
        void ForEachNode( TNodeIdCallback callback ) {
            ForEachNode( 0, callback );
        }
    };

    using UniqueScenePtr     = typename std::unique_ptr< Scene >;
    using UniqueSceneSrcPtr  = typename std::unique_ptr< const apemodefb::SceneFb >;
    using UniqueScenePtrPair = typename std::pair< std::unique_ptr< Scene >, std::unique_ptr< const apemodefb::SceneFb > >;

    UniqueScenePtrPair LoadSceneFromBin( const uint8_t *pData, size_t dataSize );
}