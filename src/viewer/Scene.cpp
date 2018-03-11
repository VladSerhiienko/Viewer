
#include "Scene.h"
#include <AppState.h>
#include <MemoryManager.h>

#ifndef PI
#define PI 3.14159265358979323846264338327950288
#endif

#define DirectOrder 1

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
           ( isnan( translation.x ) || isnan( translation.y ) || isnan( translation.z ) || isnan( rotationOffset.x ) ||
             isnan( rotationOffset.y ) || isnan( rotationOffset.z ) || isnan( rotationPivot.x ) || isnan( rotationPivot.y ) ||
             isnan( rotationPivot.z ) || isnan( preRotation.x ) || isnan( preRotation.y ) || isnan( preRotation.z ) ||
             isnan( postRotation.x ) || isnan( postRotation.y ) || isnan( postRotation.z ) || isnan( rotation.x ) ||
             isnan( rotation.y ) || isnan( rotation.z ) || isnan( scalingOffset.x ) || isnan( scalingOffset.y ) ||
             isnan( scalingOffset.z ) || isnan( scalingPivot.x ) || isnan( scalingPivot.y ) || isnan( scalingPivot.z ) ||
             isnan( scaling.x ) || isnan( scaling.y ) || isnan( scaling.z ) || isnan( geometricTranslation.x ) ||
             isnan( geometricTranslation.y ) || isnan( geometricTranslation.z ) || isnan( geometricRotation.x ) ||
             isnan( geometricRotation.y ) || isnan( geometricRotation.z ) || isnan( geometricScaling.x ) ||
             isnan( geometricScaling.y ) || isnan( geometricScaling.z ) || scaling.x == 0 || scaling.y == 0 || scaling.z == 0 ||
             geometricScaling.x == 0 || geometricScaling.y == 0 || geometricScaling.z == 0 );
}

/**
 * Calculates local matrix.
 * @note Contributes node world matrix.
 * @return Node local matrix.
 **/

XMMATRIX XMMatrixRotationZYX( const XMFLOAT3 *v ) {

    return XMMatrixRotationX( v->x ) *
           XMMatrixRotationY( v->y ) *
           XMMatrixRotationZ( v->z ) ;
}

XMMATRIX apemode::SceneNodeTransform::CalculateLocalMatrix( ) const {

    return XMMatrixTranslationFromVector( XMVectorNegate( XMLoadFloat3( &scalingPivot ) ) ) *
           XMMatrixScalingFromVector( XMLoadFloat3( &scaling ) ) *
           XMMatrixTranslationFromVector( XMLoadFloat3( &scalingPivot ) ) *
           XMMatrixTranslationFromVector( XMLoadFloat3( &scalingOffset ) ) *
           XMMatrixTranslationFromVector( XMVectorNegate( XMLoadFloat3( &rotationPivot ) ) ) *
           XMMatrixRotationZYX( &postRotation ) *
           XMMatrixRotationZYX( &rotation ) *
           XMMatrixRotationZYX( &preRotation ) *
           XMMatrixTranslationFromVector( XMLoadFloat3( &rotationPivot ) ) *
           XMMatrixTranslationFromVector( XMLoadFloat3( &rotationOffset ) ) *
           XMMatrixTranslationFromVector( XMLoadFloat3( &translation ) );
}

/**
 * Calculate geometric transform.
 * @note That is an object-offset 3ds Max concept,
 *       it does not influence scene hierarchy, though
 *       it contributes to node world transform.
 * @return Node geometric transform.
 **/

XMMATRIX apemode::SceneNodeTransform::CalculateGeometricMatrix( ) const {

    return XMMatrixScalingFromVector( XMLoadFloat3( &geometricScaling ) ) *
           XMMatrixRotationZYX( &geometricRotation ) *
           XMMatrixTranslationFromVector( XMLoadFloat3( &geometricTranslation ) );
}

/**
 * Internal usage only.
 * @see UpdateMatrices().
 * @todo Safety assertions.
 **/

void apemode::Scene::UpdateChildWorldMatrices( const uint32_t nodeId ) {
    for ( const auto childId : nodes[ nodeId ].childIds ) {

        localMatrices[ childId ]        = transforms[ childId ].CalculateLocalMatrix( );
        geometricMatrices[ childId ]    = transforms[ childId ].CalculateGeometricMatrix( );
        hierarchicalMatrices[ childId ] = localMatrices[ childId ] * hierarchicalMatrices[ nodes[ childId ].parentId ];
        worldMatrices[ childId ]        = geometricMatrices[ childId ] * hierarchicalMatrices[ childId ];

        if ( false == nodes[ childId ].childIds.empty( ) )
            UpdateChildWorldMatrices( childId );
    }
}

/**
 * Update matrices storage with up-to-date values.
 **/

void apemode::Scene::UpdateMatrices( ) {
    if ( transforms.empty( ) || nodes.empty( ) )
        return;

    //
    // Resize matrices storage if needed
    //

    if ( localMatrices.size( ) < transforms.size( ) ) {
        localMatrices.resize( transforms.size( ) );
        worldMatrices.resize( transforms.size( ) );
        geometricMatrices.resize( transforms.size( ) );
        hierarchicalMatrices.resize( transforms.size( ) );
    }

    //
    // Implicit world calculations for the root node.
    // Start recursive updates from the root node.
    //

    localMatrices[ 0 ]        = transforms[ 0 ].CalculateLocalMatrix( );
    geometricMatrices[ 0 ]    = transforms[ 0 ].CalculateGeometricMatrix( );
    hierarchicalMatrices[ 0 ] = localMatrices[ 0 ];
    worldMatrices[ 0 ]        = geometricMatrices[ 0 ] * localMatrices[ 0 ];

    UpdateChildWorldMatrices( 0 );
}

std::unique_ptr< Scene > apemode::LoadSceneFromFile( const char *filename ) {
    std::unique_ptr< Scene > scene( new Scene( ) );
    if ( flatbuffers::LoadFile( filename, true, &scene->sourceData ) ) {
        if ( scene->sourceScene = apemodefb::GetSceneFb( scene->sourceData.c_str( ) ) ) {
            std::set< uint32_t > meshIds;
            std::set< uint32_t > materialIds;
            // std::map< uint32_t, Texture * > textureIdToTexture;

            if ( auto nodesFb = scene->sourceScene->nodes( ) ) {
                scene->nodes.resize( nodesFb->size( ) );
                scene->transforms.resize( nodesFb->size( ) );

                const float toRadsFactor = float( PI ) / 180.0f;

                for ( auto nodeFb : *nodesFb ) {
                    assert( nodeFb );

                    auto &node = scene->nodes[ nodeFb->id( ) ];

                    node.id     = nodeFb->id( );
                    node.meshId = nodeFb->mesh_id( );

                    AppState::Get( )->Logger->info( "Processing node: {}", GetStringProperty( scene->sourceScene, nodeFb->name_id( ) ).c_str( ) );

                    if ( nodeFb->child_ids( ) && nodeFb->child_ids( )->size( ) )
                        node.childIds.reserve( nodeFb->child_ids( )->size( ) );

                    if ( nodeFb->material_ids( ) && nodeFb->material_ids( )->size( ) )
                        node.materialIds.reserve( nodeFb->material_ids( )->size( ) );

                    auto childIdsIt = nodeFb->child_ids( )->data( );
                    auto childIdsEndIt = childIdsIt + nodeFb->child_ids( )->size( );
                    std::transform( childIdsIt, childIdsEndIt, std::back_inserter( node.childIds ), [&]( uint32_t id ) {
                        auto &childNode    = scene->nodes[ id ];
                        childNode.parentId = node.id;
                        return id;
                    } );

                    if ( nodeFb->mesh_id( ) != -1 ) {
                        meshIds.insert( nodeFb->mesh_id( ) );
                    }

                    if ( nodeFb->material_ids( ) && nodeFb->material_ids( )->size( ) ) {
                        auto matIdsIt = nodeFb->material_ids( )->data( );
                        auto matIdsEndIt = matIdsIt + nodeFb->material_ids( )->size( );
                        std::transform( matIdsIt, matIdsEndIt, std::back_inserter( node.materialIds ), [&]( uint32_t id ) { return id; } );
                        materialIds.insert( matIdsIt, matIdsEndIt );
                    }

                    auto &transform = scene->transforms[ nodeFb->id( ) ];
                    auto transformFb = ( *scene->sourceScene->transforms( ) )[ nodeFb->id( ) ];

                    transform.translation.x = ( transformFb->translation( ).x( ) );
                    transform.translation.y = ( transformFb->translation( ).y( ) );
                    transform.translation.z = ( transformFb->translation( ).z( ) );
                    transform.rotationOffset.x = ( transformFb->rotation_offset( ).x( ) );
                    transform.rotationOffset.y = ( transformFb->rotation_offset( ).y( ) );
                    transform.rotationOffset.z = ( transformFb->rotation_offset( ).z( ) );
                    transform.rotationPivot.x = ( transformFb->rotation_pivot( ).x( ) );
                    transform.rotationPivot.y = ( transformFb->rotation_pivot( ).y( ) );
                    transform.rotationPivot.z = ( transformFb->rotation_pivot( ).z( ) );
                    transform.preRotation.x = ( transformFb->pre_rotation( ).x( ) * toRadsFactor );
                    transform.preRotation.y = ( transformFb->pre_rotation( ).y( ) * toRadsFactor );
                    transform.preRotation.z = ( transformFb->pre_rotation( ).z( ) * toRadsFactor );
                    transform.rotation.x = ( transformFb->rotation( ).x( ) * toRadsFactor );
                    transform.rotation.y = ( transformFb->rotation( ).y( ) * toRadsFactor );
                    transform.rotation.z = ( transformFb->rotation( ).z( ) * toRadsFactor );
                    transform.postRotation.x = ( transformFb->post_rotation( ).x( ) * toRadsFactor );
                    transform.postRotation.y = ( transformFb->post_rotation( ).y( ) * toRadsFactor );
                    transform.postRotation.z = ( transformFb->post_rotation( ).z( ) * toRadsFactor );
                    transform.scalingOffset.x = ( transformFb->scaling_offset( ).x( ) );
                    transform.scalingOffset.y = ( transformFb->scaling_offset( ).y( ) );
                    transform.scalingOffset.z = ( transformFb->scaling_offset( ).z( ) );
                    transform.scalingPivot.x = ( transformFb->scaling_pivot( ).x( ) );
                    transform.scalingPivot.y = ( transformFb->scaling_pivot( ).y( ) );
                    transform.scalingPivot.z = ( transformFb->scaling_pivot( ).z( ) );
                    transform.scaling.x = ( transformFb->scaling( ).x( ) );
                    transform.scaling.y = ( transformFb->scaling( ).y( ) );
                    transform.scaling.z = ( transformFb->scaling( ).z( ) );
                    transform.geometricTranslation.x = ( transformFb->geometric_translation( ).x( ) );
                    transform.geometricTranslation.y = ( transformFb->geometric_translation( ).y( ) );
                    transform.geometricTranslation.z = ( transformFb->geometric_translation( ).z( ) );
                    transform.geometricRotation.x = ( transformFb->geometric_rotation( ).x( ) * toRadsFactor );
                    transform.geometricRotation.y = ( transformFb->geometric_rotation( ).y( ) * toRadsFactor );
                    transform.geometricRotation.z = ( transformFb->geometric_rotation( ).z( ) * toRadsFactor );
                    transform.geometricScaling.x = ( transformFb->geometric_scaling( ).x( ) );
                    transform.geometricScaling.y = ( transformFb->geometric_scaling( ).y( ) );
                    transform.geometricScaling.z = ( transformFb->geometric_scaling( ).z( ) );

                    assert( transform.Validate( ) );
                }

                scene->UpdateMatrices( );
            }

            if ( auto meshesFb = scene->sourceScene->meshes( ) ) {
                scene->meshes.reserve( meshIds.size( ) );

                for ( auto meshId : meshIds ) {
                    auto meshFb = FlatbuffersTVectorGetAtIndex( meshesFb, meshId );

                    assert( meshFb );
                    assert( meshFb->vertices( ) && meshFb->vertices( )->size( ) );
                    assert( meshFb->subsets( ) && meshFb->subsets( )->size( ) > 0 );
                    assert( meshFb->submeshes( ) && meshFb->submeshes( )->size( ) == 1 );

                    if ( meshFb->vertices( ) && meshFb->vertices( )->size( ) && meshFb->submeshes( ) &&
                         meshFb->submeshes( )->size( ) == 1 ) {
                        scene->meshes.emplace_back( );
                        auto &mesh = scene->meshes.back( );

                        auto submeshesFb = meshFb->submeshes( );
                        auto submeshFb   = submeshesFb->Get( 0 );

                        mesh.positionOffset.x = ( submeshFb->position_offset( ).x( ) );
                        mesh.positionOffset.y = ( submeshFb->position_offset( ).y( ) );
                        mesh.positionOffset.z = ( submeshFb->position_offset( ).z( ) );
                        mesh.positionScale.x  = ( submeshFb->position_scale( ).x( ) );
                        mesh.positionScale.y  = ( submeshFb->position_scale( ).y( ) );
                        mesh.positionScale.z  = ( submeshFb->position_scale( ).z( ) );
                        mesh.texcoordOffset.x = ( submeshFb->uv_offset( ).x( ) );
                        mesh.texcoordOffset.y = ( submeshFb->uv_offset( ).y( ) );
                        mesh.texcoordScale.x  = ( submeshFb->uv_scale( ).x( ) );
                        mesh.texcoordScale.y  = ( submeshFb->uv_scale( ).y( ) );

                        /*BufferLoadDesc vertexBufferLoadDesc = {};
                        memset( &vertexBufferLoadDesc, 0, sizeof( vertexBufferLoadDesc ) );

                        vertexBufferLoadDesc.mDesc.mUsage        = BUFFER_USAGE_VERTEX;
                        vertexBufferLoadDesc.mDesc.mMemoryUsage  = RESOURCE_MEMORY_USAGE_GPU_ONLY;
                        vertexBufferLoadDesc.mDesc.mSize         = submeshFb->vertex_count( ) * submeshFb->vertex_stride( );
                        vertexBufferLoadDesc.mDesc.mVertexStride = submeshFb->vertex_stride( );
                        vertexBufferLoadDesc.pData =
                            meshFb->vertices( )->data( ) + submeshFb->base_vertex( ) * submeshFb->vertex_stride( );
                        vertexBufferLoadDesc.ppBuffer = &mesh.pVertexBuffer;

                        addResource( &vertexBufferLoadDesc );

                        if ( submeshFb->index_count( ) ) {
                            BufferLoadDesc indexBufferLoadDesc = {};
                            memset( &indexBufferLoadDesc, 0, sizeof( vertexBufferLoadDesc ) );

                            uint32_t  stride = meshFb->index_type( ) == apemodefb::EIndexTypeFb_UInt32 ? 4 : 2;
                            IndexType indexType =
                                meshFb->index_type( ) == apemodefb::EIndexTypeFb_UInt32 ? INDEX_TYPE_UINT32 : INDEX_TYPE_UINT16;

                            indexBufferLoadDesc.mDesc.mUsage       = BUFFER_USAGE_INDEX;
                            indexBufferLoadDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
                            indexBufferLoadDesc.mDesc.mSize        = submeshFb->index_count( ) * stride;
                            indexBufferLoadDesc.mDesc.mIndexType   = indexType;
                            indexBufferLoadDesc.pData    = meshFb->indices( )->data( ) + submeshFb->base_index( ) * stride;
                            indexBufferLoadDesc.ppBuffer = &mesh.pIndexBuffer;

                            addResource( &indexBufferLoadDesc );
                        }*/

                        mesh.subsets.reserve( meshFb->subsets( )->size( ) );
                        std::transform( meshFb->subsets( )->begin( ),
                                        meshFb->subsets( )->end( ),
                                        std::back_inserter( mesh.subsets ),
                                        [&]( const apemodefb::SubsetFb *subsetFb ) {
                                            assert( subsetFb && subsetFb->index_count( ) );

                                            SceneMeshSubset subset;
                                            subset.materialId = subsetFb->material_id( );
                                            subset.baseIndex  = subsetFb->base_index( );
                                            subset.indexCount = subsetFb->index_count( );

                                            return subset;
                                        } );
                    }
                }
            }

            if ( auto pMaterialsFb = scene->sourceScene->materials( ) ) {
                scene->materials.reserve( pMaterialsFb->size( ) );

                for ( auto materialId : materialIds ) {
                    auto pMaterialFb = FlatbuffersTVectorGetAtIndex( pMaterialsFb, materialId );
                    assert( pMaterialFb );

                    AppState::Get( )->Logger->info( "Processing material {}",
                               GetStringProperty( scene->sourceScene, pMaterialFb->name_id( ) ).c_str( ) );

                    scene->materials.emplace_back( );
                    auto &material = scene->materials.back( );

                    if ( auto pPropertiesFb = pMaterialFb->properties( ) ) {
                        for ( auto pMaterialPropFb : *pPropertiesFb ) {
                            switch ( MaterialPropertyGetType( pMaterialPropFb->value_id( ) ) ) {
                                case apemodefb::EValueTypeFb::EValueTypeFb_String:
                                    AppState::Get( )->Logger->info( "\t{} -> {}",
                                               GetStringProperty( scene->sourceScene, pMaterialPropFb->name_id( ) ).c_str( ),
                                               GetStringProperty( scene->sourceScene, pMaterialPropFb->value_id( ) ).c_str( ) );
                                    break;

                                case apemodefb::EValueTypeFb::EValueTypeFb_Bool:
                                    AppState::Get( )->Logger->info( "\t{} -> {}",
                                               GetStringProperty( scene->sourceScene, pMaterialPropFb->name_id( ) ).c_str( ),
                                               GetBoolProperty( scene->sourceScene, pMaterialPropFb->value_id( ) ) );
                                    break;

                                case apemodefb::EValueTypeFb::EValueTypeFb_Float:
                                    AppState::Get( )->Logger->info( "\t{} -> {}",
                                               GetStringProperty( scene->sourceScene, pMaterialPropFb->name_id( ) ).c_str( ),
                                               GetScalarProperty( scene->sourceScene, pMaterialPropFb->value_id( ) ) );
                                    break;

                                case apemodefb::EValueTypeFb::EValueTypeFb_Float2:
                                    AppState::Get( )->Logger->info(
                                        "\t{} -> {} {}",
                                        GetStringProperty( scene->sourceScene, pMaterialPropFb->name_id( ) ).c_str( ),
                                        (float) GetVec2Property( scene->sourceScene, pMaterialPropFb->value_id( ) ).x,
                                        (float) GetVec2Property( scene->sourceScene, pMaterialPropFb->value_id( ) ).y );
                                    break;

                                case apemodefb::EValueTypeFb::EValueTypeFb_Float3:
                                    AppState::Get( )->Logger->info(
                                        "\t{} -> {} {} {}",
                                        GetStringProperty( scene->sourceScene, pMaterialPropFb->name_id( ) ).c_str( ),
                                        (float) GetVec3Property( scene->sourceScene, pMaterialPropFb->value_id( ) ).x,
                                        (float) GetVec3Property( scene->sourceScene, pMaterialPropFb->value_id( ) ).y,
                                        (float) GetVec3Property( scene->sourceScene, pMaterialPropFb->value_id( ) ).z );
                                    break;

                                case apemodefb::EValueTypeFb::EValueTypeFb_Float4:
                                    AppState::Get( )->Logger->info(
                                        "\t{} -> {} {} {} {}",
                                        GetStringProperty( scene->sourceScene, pMaterialPropFb->name_id( ) ).c_str( ),
                                        (float) GetVec4Property( scene->sourceScene, pMaterialPropFb->value_id( ) ).x,
                                        (float) GetVec4Property( scene->sourceScene, pMaterialPropFb->value_id( ) ).y,
                                        (float) GetVec4Property( scene->sourceScene, pMaterialPropFb->value_id( ) ).z,
                                        (float) GetVec4Property( scene->sourceScene, pMaterialPropFb->value_id( ) ).w );
                                    break;

                                default:
                                    break;
                            }
                        }
                    }

                    /*TextureLoadDesc textureLoadDesc = {};
                    if ( auto pTexturePropertiesFb = pMaterialFb->texture_properties( ) ) {
                        for ( auto pMaterialPropFb : *pTexturePropertiesFb ) {
                            AppState::Get( )->Logger->info( "\t{} -> {}",
                                       GetStringProperty( scene->sourceScene, pMaterialPropFb->name_id( ) ).c_str( ),
                                       pMaterialPropFb->value_id( ) );

                            if ( auto pTexturesFb = scene->sourceScene->textures( ) )
                                if ( auto pFilesFb = scene->sourceScene->files( ) ) {
                                    if ( textureIdToTexture.find( pMaterialPropFb->value_id( ) ) ==
                                         textureIdToTexture.end( ) ) {
                                        if ( auto pTextureFb =
                                                 FlatbuffersTVectorGetAtIndex( pTexturesFb, pMaterialPropFb->value_id( ) ) ) {
                                            scene->images.emplace_back(
                                                conf_placement_new< Image >( conf_malloc( sizeof( Image ) ) ) );
                                            scene->textures.emplace_back(
                                                conf_placement_new< Texture >( conf_malloc( sizeof( Texture ) ) ) );

                                            auto pFileFb = FlatbuffersTVectorGetAtIndex( pFilesFb, pTextureFb->file_id( ) );
                                            auto fileName =
                                                FlatbuffersTVectorGetAtIndex( scene->sourceScene->string_values( ),
                                                                              MaterialPropertyGetIndex( pFileFb->name_id( ) ) );

                                            AppState::Get( )->Logger->info(
                                                "Loading texture: {}",
                                                GetStringProperty( scene->sourceScene, pTextureFb->name_id( ) ).c_str( ) );
                                            AppState::Get( )->Logger->info( "Loading texture from file: {}", fileName->c_str( ) );

                                            scene->images.back( )->iLoadSTBIFromMemory(
                                                (const char *) pFileFb->buffer( )->data( ), pFileFb->buffer( )->size( ), true );

                                            textureLoadDesc.pDesc       = nullptr;
                                            textureLoadDesc.mUseMipmaps = true;
                                            textureLoadDesc.pImage      = scene->images.back( );
                                            textureLoadDesc.ppTexture   = &scene->textures.back( );

                                            addResource( &textureLoadDesc );
                                            textureIdToTexture[ pMaterialPropFb->value_id( ) ] = scene->textures.back( );
                                        }
                                    }
                                }
                        }
                    }*/
                }
            }

            return std::move( scene );
        }
    }

    return nullptr;
}
