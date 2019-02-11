#version 450
#extension GL_ARB_separate_shader_objects : enable

// #include <shaders/shaderlib.inc>

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
    vec4  BaseColorFactor;
    vec4  EmissiveFactor;
    vec4  MetallicRoughnessFactor;
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
layout( location = 2 ) in vec3 WorldTangent;
layout( location = 3 ) in vec3 WorldBitangent;
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

float GetAO( ) {
    if ( IsTextureMapAvailable( Flags.x, 7 ) )
        return texture( OcclusionMap, Texcoords ).x;
    return 1.0;
}

mat3 CalculateTangentToWorldMatrix( vec3 p, vec3 n, vec2 uv ) {
// #define APEMODE_SCENE_FRAG_BETTER_TANGENT_SPACE_PRECISION
#ifdef APEMODE_SCENE_FRAG_BETTER_TANGENT_SPACE_PRECISION
    vec3 e1   = dFdx( p );
    vec3 e2   = dFdy( p );
    vec2 duv1 = dFdx( uv );
    vec2 duv2 = dFdy( uv );
    vec3 t    = ( duv2.y * e1 - duv1.y * e2 ) / ( duv1.x * duv2.y - duv2.x * duv1.y );
    t         = normalize( t - n * dot( n, t ) );
    vec3 b    = normalize( cross( n, t ) );
    return mat3( t, b, n );
#else
    return mat3( WorldTangent, WorldBitangent, WorldNormal );
#endif
}

vec3 CalculateWorldNormal( ) {
    if ( false == IsTextureMapAvailable( Flags.x, 3 ) )
        return WorldNormal;

    // Calculate normal in the tangent space.
    // Read texture color [0, 1], transform to [-1, 1], and normalize.
    vec3 tangentialNormal = normalize( texture( NormalMap, Texcoords ).xyz * vec3( 2.0 ) - vec3( 1.0 ) );

    // Transform from tangent space to world space.
    return CalculateTangentToWorldMatrix( WorldPosition, WorldNormal, Texcoords ) * tangentialNormal.xyz;
}

vec3 CalculateFresnel( vec3 _cspec, float _dot, float _strength ) {
    return _cspec + ( 1.0 - _cspec ) * pow( 1.0 - _dot, 5.0 ) * _strength;
}

vec3 CalculateLambert( vec3 _cdiff, float _ndotl ) {
    return _cdiff * _ndotl;
}

vec3 CalculateBlinn( vec3 _cspec, float _ndoth, float _ndotl, float _specPwr ) {
    float norm = ( _specPwr + 8.0 ) * 0.125;
    float brdf = pow( _ndoth, _specPwr ) * _ndotl * norm;
    return _cspec * brdf;
}

float CalculateSpecPwr( float _gloss ) {
    return exp2( 10.0 * _gloss + 2.0 );
}

vec3 CalculateLinear( vec3 _rgb ) {
    return pow( abs( _rgb ), vec3( 2.2 ) );
}

vec3 CalculateFilmic( vec3 _rgb ) {
    _rgb = max( vec3( 0.0 ), _rgb - 0.004 );
    _rgb = ( _rgb * ( 6.2 * _rgb + 0.5 ) ) / ( _rgb * ( 6.2 * _rgb + 1.7 ) + 0.06 );
    return _rgb;
}

float GetRadianceMipLevelCount() {
    return MetallicRoughnessFactor.w;
}

void main( ) {

    vec3 L          = normalize( LightDirection.xyz );
    vec3 lightColor = LightColor.xyz;

    vec3 N = CalculateWorldNormal( );
    vec3 V = ViewDirection.xyz;
    vec3 R = reflect( -V, N );
    vec3 H = normalize( V + L );

    float dotNV = clamp( dot( N, V ), 0.0, 1.0 );
    float dotNL = clamp( dot( N, L ), 0.0, 1.0 );
    float dotNH = clamp( dot( N, H ), 0.0, 1.0 );
    float dotHV = clamp( dot( H, V ), 0.0, 1.0 );

    vec4  baseColor = GetBaseColor( );
    float metallic  = GetMetallic( );
    float roughness = GetRoughness( );
    float gloss = 1.0 - roughness;

    vec3 reflection = mix( vec3( 0.04 ), baseColor.xyz, metallic );
    vec3 albedo     = baseColor.xyz * vec3( 1.0 - metallic );

    vec3 dirFresnel = CalculateFresnel( reflection, dotHV, gloss );
    vec3 envFresnel = CalculateFresnel( reflection, dotNV, gloss );
    vec3 lambert    = CalculateLambert( albedo * ( vec3( 1.0 ) - dirFresnel ), dotNL );
    vec3 blinn      = CalculateBlinn( dirFresnel, dotNH, dotNL, CalculateSpecPwr( gloss ) );
    vec3 direct     = ( lambert + blinn ) * lightColor.xyz;

    float radianceMipLevel = 1.0 + ( GetRadianceMipLevelCount( ) - 1.0 ) * ( 1.0 - gloss );
    vec3  radiance         = CalculateLinear( textureLod( SkyboxCubeMap, R, radianceMipLevel ).xyz );
    vec3  irradiance       = CalculateLinear( texture( IrradianceCubeMap, R ).xyz );
    vec3  envDiffuse       = albedo * irradiance;
    vec3  envSpecular      = envFresnel * radiance;
    vec3  indirect         = envDiffuse + envSpecular;

    vec3 color = direct + indirect;
    color = color * exp2( 0.0 );
    // color = color * GetAO( );

    if ( IsTextureMapAvailable( Flags.x, 4 ) ) {
        vec3 emissive = EmissiveFactor.rgb * texture( EmissiveMap, Texcoords ).rgb;
        color += emissive;
    }

    OutColor.rgb = CalculateFilmic( color.rgb );
    OutColor.a   = baseColor.a;
}
