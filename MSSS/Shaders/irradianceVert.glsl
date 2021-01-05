#version 330

uniform mat4 modelMatrix;
uniform mat4 lightViewMatrix;
uniform mat4 lightProjMatrix;


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
	// unwrap the texture coordinate to fill the screen
	gl_Position = vec4(texCoord * 2.0 - 1.0, 0.0, 1.0);
	
	vec4 worldPos = modelMatrix * vec4(position, 1.0f);

	oTex = texCoord;
	oWorldPos = worldPos.xyz;

	// transform normal to world space
    oWorldNormal = normalize(mat3(modelMatrix) * aNormal);
	
	oShadowCoord = lightViewMatrix * worldPos;
	oLightDist = -oShadowCoord.z;				// view space depth
	oShadowCoord = lightProjMatrix * oShadowCoord;

	//OUT.colour	   = colour;
	mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));
	OUT.texCoord   = texCoord;
	OUT.normal     = normalize(normalMatrix * normalize(aNormal));
	OUT.tangent    = normalize(normalMatrix * normalize(tangent));
	OUT.binormal   = normalize(normalMatrix * normalize(cross(aNormal, tangent)));
	OUT.worldPos   = (modelMatrix * vec4(position, 1.0)).xyz;
	//OUT.shadowProj = shadowMatrix * vec4(position + (aNormal * 1.5), 1.0);
}