#version 330 core
layout (location = 0) in vec3 position;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

void main(void) {
	mat4 mvp = projMatrix * viewMatrix * modelMatrix;

	gl_Position = mvp * vec4(position, 1.0);
}