#version 450
#extension GL_ARB_separate_shader_objects : enable

#include <shaders/apemode/shaderlib.inc>

layout( set = 0, binding = 1 ) uniform samplerCube u_samplerCube0;
layout( set = 0, binding = 2 ) uniform samplerCube u_samplerCube1;

layout( location = 0 ) in vec3 v_dir;
layout( location = 1 ) in vec4 v_params0;
layout( location = 2 ) in vec4 v_params1;
layout( location = 3 ) in vec2 v_texcoords;
layout( location = 0 ) out vec4 o_color;

#define u_lerpFactor v_params0.y
#define u_unused v_params1.y
#define u_exposure0 v_params0.x
#define u_exposure1 v_params1.x
#define u_textureCubeDim0 v_params0.z
#define u_textureCubeLod0 v_params0.w
#define u_textureCubeDim1 v_params1.z
#define u_textureCubeLod1 v_params1.w

void main( ) {
    vec4 textureColor0 = texture( u_samplerCube0, v_dir );
    o_color = textureColor0;

// #ifdef APEMODE_FIX_CUBE_LOOKUP_FOR_LOD
//     vec3 fixedDir0     = fixCubeLookup( v_dir, u_textureCubeLod0, u_textureCubeDim0 );
//     vec4 textureColor0 = toLinear( textureLod( u_samplerCube0, fixedDir0, u_textureCubeLod0 ) );
// #else // !APEMODE_FIX_CUBE_LOOKUP_FOR_LOD
//     vec4 textureColor0 = toLinear( texture( u_samplerCube0, v_dir ) );
// #endif // APEMODE_FIX_CUBE_LOOKUP_FOR_LOD
//     textureColor0 *= exp2( u_exposure0 );

// #ifdef APEMODE_LERP
// #ifdef APEMODE_FIX_CUBE_LOOKUP_FOR_LOD
//     vec3 fixedDir1     = fixCubeLookup( v_dir, u_textureCubeLod1, u_textureCubeDim1 );
//     vec4 textureColor1 = toLinear( textureLod( u_samplerCube1, fixedDir1, u_textureCubeLod1 ) );
// #else// !APEMODE_FIX_CUBE_LOOKUP_FOR_LOD
//     vec4 textureColor1 = toLinear( texture( u_samplerCube1, v_dir ) );
// #endif // APEMODE_FIX_CUBE_LOOKUP_FOR_LOD
//     textureColor1 *= exp2( u_exposure1 );
//     o_color = toFilmic( lerp(textureColor1, textureColor1, u_lerpFactor) );
// #else // !APEMODE_LERP
//     o_color = toFilmic( textureColor0 );
// #endif // APEMODE_LERP

//     // o_color = vec4(0.0, v_texcoords.y, 0.0, 1.0);
//     // o_color = vec4(v_texcoords.x, 0.0, 0.0, 1.0);
//     // o_color = vec4(v_texcoords, 0.0, 1.0);
//     // o_color = vec4(v_texcoords, 0.5, 1.0);
//     o_color = textureColor0;
//     //o_color = vec4(1.0, 1.0, 0.5, 1.0);
}
