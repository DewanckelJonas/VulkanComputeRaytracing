#version 450

layout (binding = 0) uniform sampler2DArray oldSamples;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	ivec3 dim = textureSize(oldSamples, 0);
	vec4 color = {0.f, 0.f, 0.f, 0.f};
	for(int i = 0; i < dim.z; ++i)
	{
		color += texture(oldSamples, vec3(inUV.s, 1.0 - inUV.t, i))/dim.z;
	}
	outFragColor = color;
  //outFragColor = texture(samplerColor, vec2(inUV.s, 1.0 - inUV.t)) + texture(oldSamples, vec2(inUV.s, 1.0 - inUV.t));
}