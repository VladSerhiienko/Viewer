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
};

layout( set = 1, binding = 2 ) uniform sampler2D BaseColorMap;
layout( set = 1, binding = 3 ) uniform sampler2D NormalMap;
layout( set = 1, binding = 4 ) uniform sampler2D EmissiveMap;
layout( set = 1, binding = 5 ) uniform sampler2D MetallicMap;
layout( set = 1, binding = 6 ) uniform sampler2D RoughnessMap;
layout( set = 1, binding = 7 ) uniform sampler2D OcclusionMap;

layout( location = 0 ) in vec3 inNormal;
layout( location = 1 ) in vec3 inWorldNormal;
layout( location = 2 ) in vec3 inViewNormal;
layout( location = 3 ) in vec2 inTexcoords;

layout( location = 0 ) out vec4 outColor;

void main( ) {
    vec2 texcoords = inTexcoords;
    // outColor.rg = inTexcoords.xy;
    // outColor.b = 0;
    // outColor.rgb = texture( OcclusionMap, texcoords ).rgb;
    // outColor.rgb = texture( NormalMap, texcoords ).rgb;
    // outColor.rgb = texture( BaseColorMap, texcoords ).rgb * BaseColorFactor.rgb;
    // outColor.rgb = texture( NormalMap, texcoords ).rgb;
    // outColor.rgb = texture( BaseColorMap, texcoords ).rgb;
    // outColor.rgb = BaseColorFactor.rgb;
    outColor.rgb = abs( inNormal.rgb );
    // outColor.rgb = abs( sepia( inNormal ) );
    // outColor.rgb *= inColor.rgb;

    outColor.a = 1;
}
