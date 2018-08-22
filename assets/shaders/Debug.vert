
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout( std140, binding = 0 ) uniform UBO {
    mat4 WorldMatrix;
    mat4 ViewMatrix;
    mat4 ProjMatrix;
    vec4 Color;
};

layout( location = 0 ) in vec3 inPosition;
layout( location = 0 ) out vec4 outColor;

void main( ) {
    outColor    = Color;
    gl_Position = ProjMatrix * ViewMatrix * WorldMatrix * vec4( inPosition, 1.0 );
}