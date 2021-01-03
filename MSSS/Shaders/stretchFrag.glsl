#version 330 core

float scale = 1.0;

in Vertex {
	vec3 worldPos;
} IN;

out vec4 fragColor;

//弹性纹理的计算非常有效，可以计算每帧高度变形网格。
//世界空间坐标的纹理空间导数给出了局部拉伸的精确估计，然后将其反转并直接乘以模糊宽度。可能需要一个常量来将值缩放到[0,1]范围。
vec2 computeStretchMap(vec3 worldPos, float scale) {      
    vec3 derivu = dFdx(worldPos);
    vec3 derivv = dFdy(worldPos);
	
    float stretchU = scale / length( derivu );
    float stretchV = scale / length( derivv );
	
    return vec2( stretchU, stretchV ); // two component texture color
}

void main(void) {
	vec2 outColour = computeStretchMap(IN.worldPos, scale);
	fragColor = vec4(outColour.xy, 0.0, 1.0);
}