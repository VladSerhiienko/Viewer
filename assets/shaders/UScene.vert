#version 450
#extension GL_ARB_separate_shader_objects : enable

//
// Set 0 (Pass)
//
// layout( std140, set = 0, binding = 0 ) uniform CameraUBO;
// layout( std140, set = 0, binding = 1 ) uniform LightUBO;
// layout( set = 0, binding = 2 ) uniform samplerCube SkyboxCubeMap;
// layout( set = 0, binding = 3 ) uniform samplerCube IrradianceCubeMap;

layout( std140, set = 0, binding = 0 ) uniform CameraUBO {
    mat4 ViewMatrix;
    mat4 ProjMatrix;
    mat4 InvViewMatrix;
    mat4 InvProjMatrix;
};

//
// Set 1 (Object)
//
// layout( std140, set = 1, binding = 0 ) uniform ObjectUBO;
// layout( std140, set = 1, binding = 1 ) uniform MaterialUBO;
// layout( set = 1, binding = 2 ) uniform sampler2D BaseColorMap;
// layout( set = 1, binding = 3 ) uniform sampler2D NormalMap;
// layout( set = 1, binding = 4 ) uniform sampler2D OcclusionMap;
// layout( set = 1, binding = 5 ) uniform sampler2D EmissiveMap;
// layout( set = 1, binding = 6 ) uniform sampler2D MetallicMap;
// layout( set = 1, binding = 7 ) uniform sampler2D RoughnessMap;

layout( std140, set = 1, binding = 0 ) uniform ObjectUBO {
    mat4 WorldMatrix;  // ObjectToWorld
    mat4 NormalMatrix; // ObjectNormalToWorld
    vec4 PositionOffset;
    vec4 PositionScale;
    vec4 TexcoordOffsetScale;
};

//
// Set 2 (SkinnedObject)
//
// layout( std140, set = 2, binding = 0 ) uniform BoneOffsetsUBO;
// layout( std140, set = 2, binding = 1 ) uniform BoneNormalsUBO;

#ifndef DEFAULT_BONE_COUNT
#define DEFAULT_BONE_COUNT 64
#endif

layout( constant_id = 0 ) const int kBoneCount = DEFAULT_BONE_COUNT;

layout( std140, set = 2, binding = 0 ) uniform BoneOffsetsUBO {
    mat4 BoneOffsetMatrices[ kBoneCount ];
};

layout( std140, set = 2, binding = 1 ) uniform BoneNormalsUBO {
    mat4 BoneNormalMatrices[ kBoneCount ];
};

#ifdef QTANGENTS
layout( location = 0 ) in vec3 inPosition;
layout( location = 1 ) in vec4 inQTangent;
layout( location = 2 ) in vec2 inTexcoords;
#define SKINNING_LAYOUT_LOCATION 3
#else
layout( location = 0 ) in vec3 inPosition;
layout( location = 1 ) in vec3 inNormal;
layout( location = 2 ) in vec4 inTangent;
layout( location = 3 ) in vec2 inTexcoords;
#define SKINNING_LAYOUT_LOCATION 4
#endif

#ifdef SKINNING
layout( location = SKINNING_LAYOUT_LOCATION ) in vec4 inBoneIndicesWeights;
#else
#ifdef SKINNING8
layout( location = SKINNING_LAYOUT_LOCATION ) in vec4 inBoneIndicesWeights0;
layout( location = SKINNING_LAYOUT_LOCATION + 1 ) in vec4 inBoneIndicesWeights1;
#endif
#endif

layout( location = 0 ) out vec3 outWorldPosition;
layout( location = 1 ) out vec3 outWorldNormal;
layout( location = 2 ) out vec3 outWorldTangent;
layout( location = 3 ) out vec3 outWorldBitangent;
layout( location = 4 ) out vec3 outViewDirection;
layout( location = 5 ) out vec2 outTexcoords;

vec3 GetCameraWorldPosition( ) {
    return InvViewMatrix[ 3 ].xyz;
}

mat4 AccumulatedBoneOffsetTransform( vec4 weights, vec4 indices ) {
    mat4 offsetTransform;
    offsetTransform = BoneOffsetMatrices[ int( indices[ 0 ] ) ] * weights[ 0 ];
    offsetTransform += BoneOffsetMatrices[ int( indices[ 1 ] ) ] * weights[ 1 ];
    offsetTransform += BoneOffsetMatrices[ int( indices[ 2 ] ) ] * weights[ 2 ];
    offsetTransform += BoneOffsetMatrices[ int( indices[ 3 ] ) ] * weights[ 3 ];
    return offsetTransform;
}

mat3 AccumulatedBoneNormalTransform( vec4 weights, vec4 indices ) {
    mat3 normalTransfrom;
    normalTransfrom = mat3( BoneNormalMatrices[ int( indices[ 0 ] ) ] ) * weights[ 0 ];
    normalTransfrom += mat3( BoneNormalMatrices[ int( indices[ 1 ] ) ] ) * weights[ 1 ];
    normalTransfrom += mat3( BoneNormalMatrices[ int( indices[ 2 ] ) ] ) * weights[ 2 ];
    normalTransfrom += mat3( BoneNormalMatrices[ int( indices[ 3 ] ) ] ) * weights[ 3 ];
    return normalTransfrom;
}

void UnpackQTangent( vec4 q, out vec3 t, out vec3 n, out float r ) {
    float qx2 = q.x + q.x, qy2 = q.y + q.y;
    float qz2   = q.z + q.z;
    float qxqx2 = q.x * qx2;
    float qxqy2 = q.x * qy2;
    float qxqz2 = q.x * qz2;
    float qxqw2 = q.w * qx2;
    float qyqy2 = q.y * qy2;
    float qyqz2 = q.y * qz2;
    float qyqw2 = q.w * qy2;
    float qzqz2 = q.z * qz2;
    float qzqw2 = q.w * qz2;

    t = vec3( 1.0 - ( qyqy2 + qzqz2 ), qxqy2 + qzqw2, qxqz2 - qyqw2 );
    n = vec3( qxqy2 - qzqw2, 1.0 - ( qxqx2 + qzqz2 ), qyqz2 + qxqw2 );
    r = ( q.w < 0.0 ) ? -1.0 : 1.0;
}

void main( ) {
    vec3 modelPosition = inPosition.xyz * PositionScale.xyz + PositionOffset.xyz;

#if defined( SKINNING )
    vec4 boneWeights = fract( inBoneIndicesWeights );
    vec4 boneIndices = inBoneIndicesWeights - boneWeights;

    mat4 accumBoneMatrix = AccumulatedBoneOffsetTransform( boneWeights, boneIndices );
#elif defined( SKINNING8 )
    vec4 boneWeights0 = fract( inBoneIndicesWeights0 );
    vec4 boneIndices0 = inBoneIndicesWeights0 - boneWeights0;
    vec4 boneWeights1 = fract( inBoneIndicesWeights1 );
    vec4 boneIndices1 = inBoneIndicesWeights1 - boneWeights1;

    mat4 accumBoneMatrix = AccumulatedBoneOffsetTransform( boneWeights1, boneIndices1 ) +
                           AccumulatedBoneOffsetTransform( boneWeights0, boneIndices0 );
#endif

#if defined( SKINNING ) || defined( SKINNING8 )
    vec4 worldPosition = WorldMatrix * accumBoneMatrix * vec4( modelPosition, 1 );
#else
    vec4  worldPosition = WorldMatrix * vec4( modelPosition, 1 );
#endif

    gl_Position = ProjMatrix * ViewMatrix * worldPosition;

    outTexcoords     = inTexcoords * TexcoordOffsetScale.zw + TexcoordOffsetScale.xy;
    outWorldPosition = worldPosition.xyz;
    outViewDirection = normalize( GetCameraWorldPosition( ).xyz - worldPosition.xyz );

#ifndef QTANGENTS
    vec3  normal     = inNormal.xyz;
    vec3  tangent    = inTangent.xyz;
    float reflection = inTangent.w;
#else
    vec3  normal;
    vec3  tangent;
    float reflection;
    UnpackQTangent( inQTangent, tangent, normal, reflection );
#endif

#if defined( SKINNING )
    mat3 accumBoneNormalMatrix = AccumulatedBoneNormalTransform( boneWeights, boneIndices );
#elif defined( SKINNING8 )
    mat3 accumBoneNormalMatrix = AccumulatedBoneNormalTransform( boneWeights1, boneIndices1 ) +
                                 AccumulatedBoneNormalTransform( boneWeights0, boneIndices0 );
#endif

#if defined( SKINNING ) || defined( SKINNING8 )
    vec3 worldNormal  = normalize( mat3( NormalMatrix ) * accumBoneNormalMatrix * normal );
    vec3 worldTangent = normalize( mat3( NormalMatrix ) * accumBoneNormalMatrix * tangent.xyz );
#else
    vec3 worldNormal  = normalize( mat3( NormalMatrix ) * normal );
    vec3 worldTangent = normalize( mat3( NormalMatrix ) * tangent.xyz );
#endif

    outWorldNormal    = worldNormal;
    outWorldTangent   = worldTangent;
    outWorldBitangent = cross( worldNormal.xyz, worldTangent.xyz ) * reflection;
}