#version 330

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;


uniform float Grow =  0.000001;
uniform float ZNear;
uniform float ZFar;


layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

out float oDepth;
out vec2 oTex;

void main()
{
	mat4 WorldView = viewMatrix * modelMatrix;

	vec4 modelPos = vec4(position, 1.0f);
	modelPos.xyz += normalize(normal) * Grow;  // scale vertex along normal
	gl_Position = projMatrix * WorldView * modelPos;

	vec4 viewPos = WorldView * vec4(position, 1.0f);
	oDepth = (-viewPos.z-ZNear)/(ZFar-ZNear); // will map near..far to 0..1
	oTex = texCoord;	
}
