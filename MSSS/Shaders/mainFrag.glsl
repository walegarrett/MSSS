#version 330 core

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;
//uniform sampler2D texture_diffuse1;
//uniform sampler2D texture_normal1;//凹凸纹理
uniform sampler2DShadow shadowTex;//阴影纹理
uniform sampler2D beckmannTex;//beckmann纹理

uniform sampler2D blurredTex1;//这些是高斯模糊相关
uniform sampler2D blurredTex2;
uniform sampler2D blurredTex3;
uniform sampler2D blurredTex4;
uniform sampler2D blurredTex5;
uniform sampler2D blurredTex6;


uniform sampler2D roughness;//-----------------------------

uniform vec3 cameraPos;
uniform vec3 lightPos;
uniform vec4 lightColour;
uniform float lightRadius;

uniform bool useTexture;
uniform bool useLighting;
uniform vec4 colour;

uniform float m;// = 0.85;//0.3 m=0.7 f=0.158(无高光),m=0.9 f=0.458,m=0.95 f=0.658,
uniform float reflectivity;//0.158
uniform float mix;// = 0.6;//0.5

uniform bool useBlur;

// RGB Gaussian weights that define skin profiles定义皮肤剖面函数的RGB高斯权重
const vec3 gaussWeights1 = vec3(0.233, 0.455, 0.649);
const vec3 gaussWeights2 = vec3(0.100, 0.336, 0.344);
const vec3 gaussWeights3 = vec3(0.118, 0.198, 0.000);
const vec3 gaussWeights4 = vec3(0.113, 0.007, 0.007);
const vec3 gaussWeights5 = vec3(0.358, 0.004, 0.000);
const vec3 gaussWeights6 = vec3(0.078, 0.000, 0.000);


in Vertex {
	//vec4 colour;	
	vec2 texCoord;
	vec3 normal;
	vec3 tangent;
	vec3 binormal;
	vec3 worldPos;
	vec4 shadowProj;
} IN;

out vec4 fragColor;

// Compute Schlick fresnel reflectance approximation://---------------------计算菲涅尔反射近似
float fresnel(vec3 halfVector, vec3 viewVector, float f0) {
    float base = 1.0 - dot(viewVector, halfVector);
    float exponential = pow(base, 5.0);
    return exponential + f0 * (1.0 - exponential);
}

// Compute Kelemen/Szirmay-Kalos specular with a beckmann texture:--------利用beckman纹理计算KSK模型
float specularKSK(sampler2D beckmannTex, vec3 normal, vec3 lightVector, vec3 viewVector, float roughness) {
    vec3 halfVector = lightVector + viewVector;
    vec3 halfVectorN = normalize(halfVector);

    float NdotL = max(0.0, dot(normal, lightVector));
    float NdotH = max(0.0, dot(normal, halfVectorN));

    float ph = pow( texture(beckmannTex, vec2(NdotH, roughness)).r * 2.0, 10.0 );
    float f = fresnel(halfVectorN, viewVector, reflectivity);//调节数值越高高光效果越明显0.028 0.128 0.158
    float ksk = max(0.0, ph * f / dot(halfVector, halfVector));

    return NdotL * ksk;   
}

void main(void) {
// ------ blinn - phong -----这里采用blinn-phong模型计算
	vec4 diffuse = texture(texture_diffuse1, IN.texCoord);//+texture(roughness,IN.texCoord);
	
	mat3 TBN = mat3(IN.tangent, IN.binormal, IN.normal);
	vec3 normal = normalize(TBN * (texture(texture_normal1, IN.texCoord).rgb * 2.0 - 1.0));

	vec3 incident = normalize(lightPos - IN.worldPos);
	vec3 viewDir = normalize(cameraPos - IN.worldPos);
	vec3 halfDir = normalize(incident + viewDir);

	float dist = length(lightPos - IN.worldPos);//距离
	float atten = 1.0 - clamp(dist / lightRadius, 0.0, 1.0);//衰减

	float lambert = max(0.0, dot(incident, normal));//漫反射
	float rFactor = max(0.0, dot(halfDir, normal));
	float sFactor = pow(rFactor, 50.0);

	float shadow = 1.0;
	if(IN.shadowProj.w > 0.0) {
		shadow = textureProj(shadowTex, IN.shadowProj);
	}
	lambert *= shadow;

	vec3 colour = (diffuse.rgb * lightColour.rgb);
	colour += (lightColour.rgb * sFactor) * 0.33;//0.33,0.55
// ----------------------------------------------------------

// ----- sss ---------------------------------------------------下面就是纹理空间次表面散射的主要过程了

	// The total diffuse light exiting the surface表面出射的所有漫反射光线（其实是辐照度贴图）
	vec3 diffuseLight = vec3(0.0);
	
	vec4 irrad1tap = texture( blurredTex1, IN.texCoord );
	vec4 irrad2tap = texture( blurredTex2, IN.texCoord );
	vec4 irrad3tap = texture( blurredTex3, IN.texCoord );
	vec4 irrad4tap = texture( blurredTex4, IN.texCoord );
	vec4 irrad5tap = texture( blurredTex5, IN.texCoord );
	vec4 irrad6tap = texture( blurredTex6, IN.texCoord );
	
	diffuseLight += gaussWeights1 * irrad1tap.xyz;
	diffuseLight += gaussWeights2 * irrad2tap.xyz;
	diffuseLight += gaussWeights3 * irrad3tap.xyz;
	diffuseLight += gaussWeights4 * irrad4tap.xyz;
	diffuseLight += gaussWeights5 * irrad5tap.xyz;
	diffuseLight += gaussWeights6 * irrad6tap.xyz;
	//这里需要注意的是，对于每个剖面，高斯项的权重和为1.0。这是由于我们是用一个漫反射颜色贴图来定义皮肤的颜色，而不是用一个与之相符的漫反射剖面。
	//通过对这些剖面进行归一化来得到白色的漫反射颜色，确保在散射入射光之后，平均结果能保持白色。然后，将此结果乘以基于图像的颜色贴图以获得肤色的色调即可。
	// Renormalize diffusion profiles to white对漫反射剖面重新标准化为1

	vec3 normConst = gaussWeights1 + gaussWeights2 + gaussWeights3 + gaussWeights4 + gaussWeights5 + gaussWeights6;
	diffuseLight /= normConst; // Renormalize to white diffuse light
	
	// Calculate vectors for light shading:
	vec3 lightVector = normalize(lightPos - IN.worldPos);
	vec3 viewVector = normalize(cameraPos - IN.worldPos);
	
	// calculate specular lighting------------------------------------使用KSK模型计算高光
	float specular = specularKSK(beckmannTex, normal, lightVector, viewVector, m);

	// Determine skin color from a diffuseColor map----------利用漫反射贴图决定皮肤的颜色---* lambert可以去除
	//fragColor = vec4( (diffuseLight * pow(diffuse.rgb, vec3(1.0 - mix))) + vec3(specular), 1.0 );
	fragColor = vec4( (diffuseLight * pow(diffuse.rgb, vec3(1.0 - mix))) * lambert + vec3(specular), 1.0 );


	//fragColor = vec4( vec3(diffuseLight * diffuse.rgb), 1.0 );
	//fragColor = vec4( diffuseLight, 1.0 );

	//fragColor = texture(bluredTex1, IN.texCoord) * pow(diffuse, vec4(1.0 - mix));
	//fragColor = vec4(texture(bluredTex1, IN.texCoord).rgb * diffuse.rgb, diffuse.a);
	//fragColor.rgb += sFactor * 0.33;

//-----------------------------------------------------------

	if (!useBlur) {
		//fragColor = diffuse + vec4(specular);
		fragColor = diffuse * lambert+ vec4(specular);
		fragColor.a = 1.0f;
	}
}