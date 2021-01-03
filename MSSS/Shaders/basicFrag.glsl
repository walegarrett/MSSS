#version 330 core

uniform sampler2D texture_diffuse1;

in Vertex {
	vec2 texCoord;
} IN;

out vec4 fragColor;

void main(void) {
	vec4 diffuse = texture(texture_diffuse1, IN.texCoord);
	fragColor = diffuse;
}