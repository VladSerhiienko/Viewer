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
    mat4 WorldMatrix; // ObjectToWorld
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

layout( constant_id = 0 ) const int kBoneCount = 128;

layout( std140, set = 2, binding = 0 ) uniform BoneOffsetsUBO {
    mat4 BoneOffsetMatrices[ kBoneCount ];
};

layout( std140, set = 2, binding = 1 ) uniform BoneNormalsUBO {
    mat4 BoneNormalMatrices[ kBoneCount ];
};

layout( location = 0 ) in vec3 inPosition;
layout( location = 1 ) in vec3 inNormal;
layout( location = 2 ) in vec4 inTangent;
layout( location = 3 ) in vec2 inTexcoords;
layout( location = 4 ) in vec4 inBoneWeights;
layout( location = 5 ) in vec4 inBoneIndices;

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
    offsetTransform  = BoneOffsetMatrices[ int( indices[ 0 ] ) ] * weights[ 0 ];
    offsetTransform += BoneOffsetMatrices[ int( indices[ 1 ] ) ] * weights[ 1 ];
    offsetTransform += BoneOffsetMatrices[ int( indices[ 2 ] ) ] * weights[ 2 ];
    offsetTransform += BoneOffsetMatrices[ int( indices[ 3 ] ) ] * weights[ 3 ];
    return offsetTransform;
}

mat3 AccumulatedBoneNormalTransform( vec4 weights, vec4 indices ) {
    mat3 normalTransfrom;
    normalTransfrom  = mat3( BoneNormalMatrices[ int( indices[ 0 ] ) ] ) * weights[ 0 ];
    normalTransfrom += mat3( BoneNormalMatrices[ int( indices[ 1 ] ) ] ) * weights[ 1 ];
    normalTransfrom += mat3( BoneNormalMatrices[ int( indices[ 2 ] ) ] ) * weights[ 2 ];
    normalTransfrom += mat3( BoneNormalMatrices[ int( indices[ 3 ] ) ] ) * weights[ 3 ];
    return normalTransfrom;
}

void main( ) {
    vec3 modelPosition = inPosition.xyz * PositionScale.xyz + PositionOffset.xyz;
    vec4 worldPosition = WorldMatrix * AccumulatedBoneOffsetTransform( inBoneWeights, inBoneIndices ) * vec4( modelPosition, 1 );
    gl_Position        = ProjMatrix * ViewMatrix * worldPosition;

    outWorldPosition = worldPosition.xyz;
    outViewDirection = normalize( GetCameraWorldPosition( ).xyz - worldPosition.xyz );
    outTexcoords     = inTexcoords * TexcoordOffsetScale.zw + TexcoordOffsetScale.xy;

    mat3 accumBoneNormalMatrix = AccumulatedBoneNormalTransform( inBoneWeights, inBoneIndices );
    vec3 worldNormal           = normalize( mat3( NormalMatrix ) * accumBoneNormalMatrix * inNormal );
    vec3 worldTangent          = normalize( mat3( NormalMatrix ) * accumBoneNormalMatrix * inTangent.xyz );

    outWorldNormal    = worldNormal;
    outWorldTangent   = worldTangent;
    outWorldBitangent = cross( worldNormal, worldTangent ) * inTangent.w;
}