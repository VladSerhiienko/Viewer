#version 450
#extension GL_ARB_separate_shader_objects : enable

#include <shaders/shaderlib.inc>

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

layout( std140, set = 0, binding = 1 ) uniform LightUBO {
	vec4 LightDirection;
	vec4 LightColor;
};

layout( set = 0, binding = 2 ) uniform samplerCube SkyboxCubeMap;
layout( set = 0, binding = 3 ) uniform samplerCube IrradianceCubeMap;

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

layout( std140, set = 1, binding = 1 ) uniform MaterialUBO {
    vec4 BaseColorFactor;
    vec4 EmissiveFactor;
    vec4 MetallicRoughnessFactor;
    uvec4 Flags;
};

layout( set = 1, binding = 2 ) uniform sampler2D BaseColorMap;
layout( set = 1, binding = 3 ) uniform sampler2D NormalMap;
layout( set = 1, binding = 4 ) uniform sampler2D EmissiveMap;
layout( set = 1, binding = 5 ) uniform sampler2D MetallicMap;
layout( set = 1, binding = 6 ) uniform sampler2D RoughnessMap;
layout( set = 1, binding = 7 ) uniform sampler2D OcclusionMap;

layout( location = 0 ) in vec3 WorldPosition;
layout( location = 1 ) in vec3 WorldNormal;
layout( location = 2 ) in vec4 WorldTangent;
layout( location = 3 ) in vec4 WorldBitangent;
layout( location = 4 ) in vec3 ViewDirection;
layout( location = 5 ) in vec2 Texcoords;

layout( location = 0 ) out vec4 OutColor;

bool IsTextureMapAvailable( uint textureFlags, uint textureMapBinding ) {
    return 0 != ( textureFlags & ( 1 << ( textureMapBinding - 2 ) ) );
}

vec4 GetBaseColor( ) {
    if ( IsTextureMapAvailable( Flags.x, 2 ) )
        return BaseColorFactor * texture( BaseColorMap, Texcoords );
    return BaseColorFactor;
}

float GetMetallic( ) {
    if ( IsTextureMapAvailable( Flags.x, 5 ) )
        return MetallicRoughnessFactor.x * texture( MetallicMap, Texcoords ).x;
    return MetallicRoughnessFactor.x;
}

float GetRoughness( ) {
    if ( IsTextureMapAvailable( Flags.x, 6 ) )
        return MetallicRoughnessFactor.y * texture( RoughnessMap, Texcoords ).x;
    return MetallicRoughnessFactor.y;
}

vec3 CalculateWorldNormal( ) {
    if ( false == IsTextureMapAvailable( Flags.x, 3 ) )
        return WorldNormal;

    vec3 detailNormal = normalize( texture( NormalMap, Texcoords ).xyz );
    detailNormal = normalize( detailNormal * vec3( 2.0 ) - vec3( 1.0 ) );

    vec3 detailedWorldNormal = ( detailNormal.x * WorldTangent.xyz ) + ( detailNormal.y * WorldBitangent.xyz ) + ( detailNormal.z * WorldNormal.xyz );
    return normalize( detailedWorldNormal );
}

void main( ) {
    vec3 worldNormal = CalculateWorldNormal();
    vec3 R = reflect( -ViewDirection, worldNormal );
    // OutColor.rgb = worldNormal.xyz; //texture( NormalMap, Texcoords ).xyz;
    // OutColor.rgb = normalize( worldNormal.xyz );
    // OutColor.rgb = WorldNormal.xyz;
    // OutColor.r = IsTextureMapAvailable( Flags.x, 3 ) ? 1.0 : 0;
    // OutColor.gb = vec2(0 ,0 );
    // OutColor.rgb = textureLod( SkyboxCubeMap, R, 3 ).rgb; // * MetallicRoughnessFactor.x;
    OutColor.rgb = texture( SkyboxCubeMap, R ).rgb; // * MetallicRoughnessFactor.x;
    // OutColor.rgb = texture( IrradianceCubeMap, R ).rgb; // * (1.0f - MetallicRoughnessFactor.x );
    OutColor.a = 1;
}
