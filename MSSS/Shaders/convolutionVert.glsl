#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

out Vertex {
	vec2 texCoord;
} OUT;

void main(void) {
	mat4 mvp = projMatrix * viewMatrix * modelMatrix;

	OUT.texCoord = texCoord;
	gl_Position = mvp * vec4(position, 1.0);
}