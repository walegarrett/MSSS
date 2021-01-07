#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D depthMap;
uniform float ZNear;
uniform float ZFar;


// required when using a perspective projection matrix
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * ZNear * ZFar) / (ZFar + ZNear - z * (ZFar - ZNear));	
}

void main()
{             
    float depthValue = texture(depthMap, TexCoords).r;
    //FragColor = vec4(vec3(LinearizeDepth(depthValue) / ZFar), 1.0); // perspective
    FragColor = vec4(vec3(depthValue), 1.0); // orthographic
}