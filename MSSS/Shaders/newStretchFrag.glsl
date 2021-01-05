#version 330

uniform float Scale;

in vec3 oWorldPos;

out vec4 FragColor;

vec2 ComputeStretchMap(vec3 worldPos, float scale)
{
	vec3 derivu = dFdx(worldPos);
	vec3 derivv = dFdy(worldPos);
	float strechU = scale / length(derivu);
	float strechV = scale / length(derivv);
	return vec2(strechU, strechV);
}

void main()
{
	vec2 stretcUV = ComputeStretchMap(oWorldPos, Scale);
	FragColor = vec4(stretcUV, 0.0, 1.0);
}