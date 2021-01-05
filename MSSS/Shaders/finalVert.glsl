#version 330

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform mat4 ViewProj;
uniform mat4 LightView;
uniform mat4 LightProj;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 texCoord;
layout (location = 3) in vec3 tangent;

out vec3 oWorldPos;
out vec3 oWorldNormal;
out vec4 oShadowCoord;
out float oLightDist;
out vec2 oTex;

out Vertex {
	//vec4 colour;
	vec2 texCoord;
	vec3 normal;
	vec3 tangent;
	vec3 binormal;
	vec3 worldPos;
	vec4 shadowProj;
} OUT;

void main()
{
	vec4 worldPos = modelMatrix * vec4(position, 1.0f);

	oTex = texCoord;
	oWorldPos = worldPos.xyz;

	// transform normal to world space
    oWorldNormal = normalize(mat3(modelMatrix) * aNormal);

	// Find the position of this pixel in light space
	oShadowCoord = LightView * worldPos;
	oLightDist = -oShadowCoord.z;				// view space depth
	oShadowCoord = LightProj * oShadowCoord;

	gl_Position = projMatrix * viewMatrix * worldPos;

	//OUT.colour	   = colour;
	mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));
	OUT.texCoord   = texCoord;
	OUT.normal     = normalize(normalMatrix * normalize(aNormal));
	OUT.tangent    = normalize(normalMatrix * normalize(tangent));
	OUT.binormal   = normalize(normalMatrix * normalize(cross(aNormal, tangent)));
	OUT.worldPos   = (modelMatrix * vec4(position, 1.0)).xyz;
	//OUT.shadowProj = shadowMatrix * vec4(position + (aNormal * 1.5), 1.0);
}