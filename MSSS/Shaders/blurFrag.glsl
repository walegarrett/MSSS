#version 330 core

uniform sampler2D diffuseTex;
uniform sampler2D stretchTex;
uniform vec2 pixelSize;
uniform bool xAxis;
uniform bool useStretch;

// GaussWidth should be the standard deviation.  高斯宽度应该是标准差
float GaussWidth = 1.0f;
// Gaussian curve - standard deviation of 1.0
const float weights[7] = float[](0.006, 0.061, 0.242, 0.382, 0.242, 0.061, 0.006);

in Vertex {
	vec2 texCoord;
} IN;

out vec4 fragColor;
//七个taps的最终间距由拉伸值和与之卷积的理想高斯函数的宽度决定。
void main(void) {
	if (useStretch) {//----------------------------------------使用拉伸矫正纹理

		vec4 stretch = texture(stretchTex, IN.texCoord);
		vec4 sum = vec4(0.0);

		if (xAxis) {//-----------------------------------------------------这里是u方向和v方向的高斯卷积
			float netFilterWidth = pixelSize.x * GaussWidth * stretch.x;
			vec2 coords = IN.texCoord - vec2(netFilterWidth * 3.0, 0.0);

			for (int i = 0; i < 7; ++i) {
				vec4 tap = texture(diffuseTex, coords);
				sum += weights[i] * tap;
				coords += vec2(netFilterWidth, 0.0);
			}
		}
		else {
			float netFilterWidth = pixelSize.y * GaussWidth * stretch.y;
			vec2 coords = IN.texCoord - vec2(0.0, netFilterWidth * 3.0);

			for (int i = 0; i < 7; ++i) {
				vec4 tap = texture(diffuseTex, coords);
				sum += weights[i] * tap;
				coords += vec2(0.0, netFilterWidth);
			}
		}

		fragColor = sum;
		
	}
	else {//------------------------------------------------不使用拉伸矫正，则简单的高斯核

		vec2 values[7];
		if (xAxis) {
			values = vec2[](vec2(-pixelSize.x * 3.0, 0.0),
							vec2(-pixelSize.x * 2.0, 0.0),
							vec2(-pixelSize.x * 1.0, 0.0),
							vec2( 				0.0, 0.0),
							vec2( pixelSize.x * 1.0, 0.0),
							vec2( pixelSize.x * 2.0, 0.0),
							vec2( pixelSize.x * 3.0, 0.0));
		}
		else {
			values = vec2[](vec2(0.0, -pixelSize.y * 3.0),
							vec2(0.0, -pixelSize.y * 2.0),
							vec2(0.0, -pixelSize.y * 1.0),
							vec2(0.0, 				 0.0),
							vec2(0.0,  pixelSize.y * 1.0),
							vec2(0.0,  pixelSize.y * 2.0),
							vec2(0.0,  pixelSize.y * 3.0));
		}

		for (int i = 0; i < 7; ++i) {
			vec4 tmp = texture(diffuseTex, IN.texCoord + values[i]);
			fragColor += tmp * weights[i];
		}

	}
}
