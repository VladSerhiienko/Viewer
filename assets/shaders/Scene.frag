#version 450
#extension GL_ARB_separate_shader_objects : enable

#include <shaders/shaderlib.inc>

layout( location = 0 ) in vec4 inColor;
layout( location = 1 ) in vec3 inNormal;

layout( location = 0 ) out vec4 outColor;

void main( ) {
    // outColor.rgb = abs( inNormal.rgb );
    outColor.rgb = inColor.rgb;
    // outColor.rgb = abs( sepia( inNormal ) );
    // outColor.rgb *= inColor.rgb;
    outColor.a = 1;
}
