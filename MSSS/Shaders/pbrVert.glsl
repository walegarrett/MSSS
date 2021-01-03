#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;

out vec2 TexCoords;
out vec3 WorldPos;
out vec3 Normal;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

void main()
{
    TexCoords = aTexCoords;
    WorldPos = vec3(modelMatrix * vec4(aPos, 1.0));
    Normal = mat3(modelMatrix) * aNormal;   

    gl_Position =  projMatrix * viewMatrix * vec4(WorldPos, 1.0);
}