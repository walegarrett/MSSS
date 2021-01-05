#version 330

uniform mat4 modelMatrix;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 texCoord;

out vec3 oWorldPos;


void main()
{
	// unwrap the texture coordinate to fill the screen
	gl_Position = vec4(texCoord * 2.0 - 1.0, 0.0, 1.0);

	// transform to world space
	oWorldPos = vec3(modelMatrix * vec4(position, 1.0f));
}