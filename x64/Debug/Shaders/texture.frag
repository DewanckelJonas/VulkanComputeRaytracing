#version 450

layout (binding = 0) uniform sampler2D samplerColor;
layout (binding = 1) uniform sampler2DArray oldSamples;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec4 color = {0.f, 0.f, 0.f, 0.f};
	for(int i = 0; i < 7; ++i)
	{
		color += texture(oldSamples, vec3(inUV.s, 1.0 - inUV.t, 2))/7.f;
	}
	outFragColor = color;
  //outFragColor = texture(samplerColor, vec2(inUV.s, 1.0 - inUV.t)) + texture(oldSamples, vec2(inUV.s, 1.0 - inUV.t));
}