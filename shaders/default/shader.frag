#version 450 core
#extension GL_GOOGLE_include_directive : require
//#extension GL_EXT_debug_printf : require

#include "../glsl_def.glsl"

layout (location = 0) in vec2 TexCoord;
layout (location = 1) in FLT Alpha;

layout(binding = 0) uniform sampler2D TextureAtlas;

layout (location = 0) out vec4 OutColor;

/**
 * \brief Main shader function
 */
VOID main( VOID )
{
  vec4 ResultColor = texture(TextureAtlas, TexCoord);
  
  ResultColor.a *= Alpha;
  
  OutColor = ResultColor;
}