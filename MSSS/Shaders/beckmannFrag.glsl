#version 330 core
in Vertex {
	vec2 texCoord;
} IN;

out vec4 fragColor;

//beckman纹理
float PHBeckmann(float ndoth, float m) {
	float alpha = acos(ndoth);
	float ta = tan(alpha);
	float val = 1.0 / (m * m * pow(ndoth, 4.0)) * exp(-(ta * ta) / (m * m));
	return val;
}
//渲染一个屏幕对齐的四边形来预计算一个512x512纹理。
void main(void) {
	// Scale the value to fit within [0,1]
	fragColor = vec4(vec3(0.5 * pow(PHBeckmann(IN.texCoord.x, IN.texCoord.y), 0.1)), 1.0);
}