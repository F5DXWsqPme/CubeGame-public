#version 450 core
#extension GL_GOOGLE_include_directive : require
//#extension GL_EXT_debug_printf : require

#include "../glsl_def.glsl"

layout (location = 0) in vec3 Position;
layout (location = 1) in FLT Alpha;
layout (location = 2) in vec2 TexCoord;

//layout(push_constant) uniform PUSH_CONSTANTS_STRUCTURE
//{
//  mat4 MatrWVP;
//} PushConstants;

layout (binding = 1) uniform UNIFORM_BUFFER
{
  mat4 MatrWVP;
} UniformBuffer;

layout (location = 0) out vec2 OutTexCoord;
layout (location = 1) out FLT OutAlpha;

/**
 * \brief Main shader function
 */
VOID main( VOID )
{
  gl_Position = UniformBuffer.MatrWVP * vec4(Position, 1);
  OutTexCoord = TexCoord;
  OutAlpha = Alpha;
}