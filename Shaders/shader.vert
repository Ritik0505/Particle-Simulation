#version 450
#extension GL_ARB_separate_shader_objects : enable

//Location specifies Attribute Location
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec4 inColor;
//layout (location = 2) in vec2 inVelocity;

layout (location = 0) out vec4 outColor;
layout (location = 1) out float outGradientPos;

layout( set = 0, binding = 1 ) uniform UniformBuffer {
  mat4 MVP;
};
//out gl_PerVertex
//{
//	vec4 gl_Position;
//	float gl_PointSize;
//};

void main () 
{
  gl_PointSize = 8.0;
  outColor = inColor;
  gl_Position = MVP*vec4(inPos.xyz, 1.0);
}
