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
    mat4 WorldMatrix;
    vec4 PositionOffset;
    vec4 PositionScale;
};

layout( location = 0 ) in vec3 inPosition;
layout( location = 1 ) in vec3 inNormal;
layout( location = 2 ) in vec4 inTangent;
layout( location = 3 ) in vec2 inTexcoords;

layout( location = 0 ) out vec3 outNormal;
layout( location = 1 ) out vec3 outWorldNormal;
layout( location = 2 ) out vec3 outViewNormal;

void main( ) {
    outNormal      = inNormal;
    outWorldNormal = mat3( WorldMatrix ) * outNormal;
    outViewNormal  = mat3( ViewMatrix ) * outWorldNormal;

    vec3 adjustedPosition = inPosition.xyz * PositionScale.xyz + PositionOffset.xyz;
    gl_Position           = ProjMatrix * ViewMatrix * WorldMatrix * vec4( adjustedPosition, 1.0 );
}