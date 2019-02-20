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
};

//
// Set 2 (SkinnedObject)
//
// layout( std140, set = 2, binding = 0 ) uniform BoneOffsetsUBO;
// layout( std140, set = 2, binding = 1 ) uniform BoneNormalsUBO;

#ifndef DEFAULT_BONE_COUNT
#define DEFAULT_BONE_COUNT 128
#endif

layout( constant_id = 0 ) const int kBoneCount = DEFAULT_BONE_COUNT;

layout( std140, set = 2, binding = 0 ) uniform BoneOffsetsUBO {
    mat4 BoneOffsetMatrices[ kBoneCount ];
};

layout( std140, set = 2, binding = 1 ) uniform BoneNormalsUBO {
    mat4 BoneNormalMatrices[ kBoneCount ];
};

layout( location = 0 ) in vec3 inPosition;
layout( location = 1 ) in vec2 inTexcoords;
layout( location = 2 ) in vec4 inQTangent;
layout( location = 3 ) in uint inIndexColorRGB;
layout( location = 4 ) in float inColorAlpha;
#define SKINNING_LAYOUT_LOCATION_0 5
#define SKINNING_LAYOUT_LOCATION_1 6

#ifdef SKINNING
layout( location = SKINNING_LAYOUT_LOCATION_0 ) in vec4 inBoneIndicesWeights;
#else
#ifdef SKINNING8
layout( location = SKINNING_LAYOUT_LOCATION_0 ) in vec4 inBoneIndicesWeights0;
layout( location = SKINNING_LAYOUT_LOCATION_1 ) in vec4 inBoneIndicesWeights1;
#endif
#endif

layout( location = 0 ) out vec3 outWorldPosition;
layout( location = 1 ) out vec2 outTexcoords;
layout( location = 2 ) out vec3 outWorldNormal;
layout( location = 3 ) out vec3 outWorldTangent;
layout( location = 4 ) out vec3 outWorldBitangent;
layout( location = 5 ) out vec3 outViewDirection;
layout( location = 6 ) out vec4 outVertexColor;
layout( location = 7 ) out vec3 outBarycoords;

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

// https://github.com/KhronosGroup/glTF/issues/812
// https://bitbucket.org/sinbad/ogre/src/18ebdbed2edc61d30927869c7fb0cf3ae5697f0a/Samples/Media/Hlms/Common/GLSL/QuaternionCode_piece_all.glsl?at=v2-1&fileviewer=file-view-default
void UnpackQTangentAxes( vec4 q, out vec3 x, out vec3 y ) {
    float fTx  = 2.0 * q.x;
    float fTy  = 2.0 * q.y;
    float fTz  = 2.0 * q.z;
    float fTxx = fTx * q.x;
    float fTxy = fTy * q.x;
    float fTxz = fTz * q.x;
    float fTwx = fTx * q.w;
    float fTwy = fTy * q.w;
    float fTwz = fTz * q.w;
    float fTyy = fTy * q.y;
    float fTyz = fTz * q.y;
    float fTzz = fTz * q.z;

    x = vec3( 1.0 - ( fTyy + fTzz ), fTxy + fTwz, fTxz - fTwy );
    y = vec3( fTxy - fTwz, 1.0 - ( fTxx + fTzz ), fTyz + fTwx );
}

void UnpackQTangent( vec4 q, out vec3 t, out vec3 n, out float r ) {
    UnpackQTangentAxes( q, n, t );
    r = ( q.w < 0.0 ) ? -1.0 : 1.0;
}

vec3 kBaryCoords[ 3 ] = {
    vec3( 1, 0, 0 ), vec3( 0, 1, 0 ), vec3( 0, 0, 1 ),
};

void main( ) {
    vec3 modelPosition = inPosition.xyz;

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

    uint baryIndex = ( inIndexColorRGB ) & 0xFF;

    vec3 colorRGB = vec3( float( ( inIndexColorRGB >> 8 ) & 0xFF ),
                          float( ( inIndexColorRGB >> 16 ) & 0xFF ),
                          float( ( inIndexColorRGB >> 24 ) & 0xFF ) ) / 255.0;

    outWorldPosition   = worldPosition.xyz;
    outTexcoords       = inTexcoords;
    outVertexColor.rgb = colorRGB;
    outVertexColor.a   = inColorAlpha;
    outBarycoords      = kBaryCoords[ baryIndex ];
    outViewDirection   = normalize( GetCameraWorldPosition( ).xyz - worldPosition.xyz );

    vec3  normal;
    vec3  tangent;
    float reflection;
    UnpackQTangent( inQTangent, tangent, normal, reflection );

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