#version 450
#extension GL_ARB_separate_shader_objects : enable

layout( std140, binding = 0 ) uniform UBO {
    mat4 ProjBias;
    mat4 InvView;
    mat4 InvProj;
    vec4 Params0;
    vec4 Params1;
};

// layout( location = 0 ) in vec3 a_position;
// layout( location = 1 ) in vec2 a_texcoords;

layout( location = 0 ) out vec3 v_dir;
layout( location = 1 ) out vec4 v_params0;
layout( location = 2 ) out vec4 v_params1;
layout( location = 3 ) out vec2 v_texcoords;

void main( ) {
    // https://rauwendaal.net/2014/06/14/rendering-a-screen-covering-triangle-in-opengl/
    vec2 texcoords = vec2( ( gl_VertexIndex << 1 ) & 2, gl_VertexIndex & 2 );
    gl_Position = ProjBias * vec4( texcoords * 2.0f - 1.0f, 0.0f, 1.0f );
    v_dir = mat3(InvView) * (InvProj * gl_Position).xyz; 
    v_params0 = Params0;
    v_params1 = Params1;
    v_texcoords = texcoords;
}
