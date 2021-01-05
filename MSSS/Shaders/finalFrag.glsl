#version 330

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;
uniform sampler2D shadowTex;//阴影纹理
uniform sampler2D beckmannTex;//beckmann纹理
uniform sampler2D StretchTex;//拉伸矫正贴图

uniform sampler2D blurredTex1;//这些是高斯模糊相关
uniform sampler2D blurredTex2;
uniform sampler2D blurredTex3;
uniform sampler2D blurredTex4;
uniform sampler2D blurredTex5;
uniform sampler2D blurredTex6;

uniform mat4 modelMatrix;// World Transform

uniform vec3 cameraPos;

uniform vec3 lightPos;
uniform vec4 lightColour;
uniform float lightRadius;

uniform float m;// = 0.85;//0.3 m=0.7 f=0.158(无高光),m=0.9 f=0.458,m=0.95 f=0.658,
uniform float reflectivity;//0.158
uniform float mix;// = 0.6;//0.5
uniform bool isShadowVSM;

uniform bool useBlur;
uniform bool useTranslucent;

uniform float ZNear;
uniform float ZFar;

// RGB Gaussian weights that define skin profiles定义皮肤剖面函数的RGB高斯权重
const vec3 gaussWeights1 = vec3(0.233, 0.455, 0.649);
const vec3 gaussWeights2 = vec3(0.100, 0.336, 0.344);
const vec3 gaussWeights3 = vec3(0.118, 0.198, 0.000);
const vec3 gaussWeights4 = vec3(0.113, 0.007, 0.007);
const vec3 gaussWeights5 = vec3(0.358, 0.004, 0.000);
const vec3 gaussWeights6 = vec3(0.078, 0.000, 0.000);


uniform float Rho_s = 0.18;
uniform float DepthScale = 10.0;
uniform float EnvAmount = 0.2;
uniform float SpecularIntensity = 1.88;

in vec3 oWorldPos;
in vec3 oWorldNormal;
in vec4 oShadowCoord;
in float oLightDist;
in vec2 oTex;

in Vertex {
	//vec4 colour;	
	vec2 texCoord;
	vec3 normal;
	vec3 tangent;
	vec3 binormal;
	vec3 worldPos;
	vec4 shadowProj;
} IN;

out vec4 FragColor;

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
//------------------------------------------------------------
#define GlossyNumMipmap 8
#define SHADOW_MAP_SIZE 1024
#define SHADOW_BIAS 0.001
#define saturate(value) clamp(value, 0.0, 1.0)


float NormalizedDepth(float depth, float zNear, float zFar)
{
	return (depth - zNear) / (zFar - zNear);
}
//求解真实的深度距离
vec4 RealDepth(vec4 depth, vec2 nearFar)
{
	return depth * (nearFar.y - nearFar.x) + nearFar.x;
}

vec2 ShadowTexCoord(vec4 clipPos)
{
	return (clipPos.xy / clipPos.w) * 0.5 + vec2(0.5, 0.5);
}

float Shadow(vec2 UV, sampler2D shadowTex, float depth)
{
    return (texture(shadowTex, UV).r < depth) ? 0.0 : 1.0;
}

float ShadowPCF(vec2 UV, sampler2D shadowTex, float depth, int samples, float width)
{
	float shadow = 0.0;
    float offset = (samples - 1.0) / 2.0;
   
    for (float x = -offset; x <= offset; x += 1.0) 
	{
        for (float y = -offset; y <= offset; y += 1.0)
		{
            vec2 tex = UV + width * vec2(x, y) / SHADOW_MAP_SIZE;
            shadow += (texture(shadowTex, tex).r < depth) ? 0.0 : 1.0;
        }
    }
    shadow /= samples * samples;
    return shadow;
}

float chebyshevUpperBound(float dist, vec2 moments)
{
	// Surface is fully lit. as the current fragment is before the light occluder
	if (dist <= moments.x)
		return 1.0 ;
	
	// The fragment is either in shadow or penumbra. We now use chebyshev's upperBound to check
	// How likely this pixel is to be lit (p_max)
	float variance = moments.y - (moments.x*moments.x);
	variance = max(variance,0.00001);
	
	float d = dist - moments.x;
	float p_max = variance / (variance + d*d);
	
	return p_max;
}


float FresnelReflectance( vec3 H, vec3 V, float F0 )  
{
	float base = 1.0 - dot( V, H );  
	float exponential = pow( base, 5.0 );  
	return F0 + (1.0 - F0) * exponential;
}

float KS_Skin_Specular( vec3 N, // Bumped surface normal  
                        vec3 L, // Points to light  
                        vec3 V, // Points to eye  
                        float m,  // Roughness  
                        float rho_s, // Specular brightness  
                        sampler2D beckmannTex )  
{  
	float result = 0.0;  
    float ndotl = dot( N, L );  
    if( ndotl > 0.0 )  
    {  
		vec3 h = L + V; // Unnormalized half-way vector  
		vec3 H = normalize( h );  
		float ndoth = dot( N, H );  
		float PH = pow( 2.0* texture(beckmannTex,vec2(ndoth,m)).y, 10.0 );  
		float F = FresnelReflectance( H, V, 0.028 );  
		float frSpec = max( PH * F / dot( h, h ), 0 );  
		result = ndotl * rho_s * frSpec; // BRDF * dot(N,L) * rho_s  
	}  
	return result;  
}  

void main()
{
	// lighting parameters
	vec3 L0 = normalize( lightPos - oWorldPos ); // point light 0 light vector
	

	// View vector
	vec3 V = normalize( cameraPos - oWorldPos );
	
	vec3 N_nonBumped = normalize( oWorldNormal );
	// compute bumped world normal
    vec3 objNormal = texture( texture_normal1, oTex ).xyz * vec3( 2.0, 2.0, 2.0 ) - vec3( 1.0, 1.0, 1.0 );  
    
	mat3 TBN = mat3(IN.tangent, IN.binormal, IN.normal);
    //vec3 N_bumped = normalize( mat3(modelMatrix) * objNormal );
	vec3 N_bumped = normalize( TBN * objNormal );
	
	float depth0 = NormalizedDepth(oLightDist, ZNear, ZFar);
	vec2 UV0 = ShadowTexCoord(oShadowCoord);
	

	float L0Shadow = chebyshevUpperBound(depth0, texture2D(shadowTex, UV0).ra);
	if(isShadowVSM){
		L0Shadow = chebyshevUpperBound(depth0, texture2D(shadowTex, UV0).ra);
	}else{
		L0Shadow = Shadow(UV0, shadowTex, depth0 - SHADOW_BIAS);
	}
	
	// ------ blinn - phong -----这里采用blinn-phong模型计算
	vec4 diffuse = texture(texture_diffuse1, oTex);//+texture(roughness,IN.texCoord);
	//-----------------------------------------------------------------
	//vec3 normal = N_bumped;
	TBN = mat3(IN.tangent, IN.binormal, IN.normal);
	vec3 normal = normalize(TBN * (texture(texture_normal1, IN.texCoord).rgb * 2.0 - 1.0));

	vec3 incident = L0;
	vec3 viewDir = normalize(cameraPos - oWorldPos);
	vec3 halfDir = normalize(incident + viewDir);

	float dist = length(lightPos - oWorldPos);//距离
	float atten = 1.0 - clamp(dist / lightRadius, 0.0, 1.0);//衰减

	float lambert = max(0.0, dot(incident, normal));//漫反射
	float rFactor = max(0.0, dot(halfDir, normal));
	float sFactor = pow(rFactor, 50.0);


	lambert *= L0Shadow;

	vec3 colour = (diffuse.rgb * lightColour.rgb);
	colour += (lightColour.rgb * sFactor) * 0.33;//0.33,0.55
// ----------------------------------------------------------

	// The total diffuse light exiting the surface表面出射的所有漫反射光线（其实是辐照度贴图）
	vec3 diffuseLight = vec3(0.0);
	
	vec4 irrad1tap = texture( blurredTex1, oTex );
	vec4 irrad2tap = texture( blurredTex2, oTex );
	vec4 irrad3tap = texture( blurredTex3, oTex );
	vec4 irrad4tap = texture( blurredTex4, oTex );
	vec4 irrad5tap = texture( blurredTex5, oTex );
	vec4 irrad6tap = texture( blurredTex6, oTex );
	
	diffuseLight += gaussWeights1 * irrad1tap.xyz;
	
	diffuseLight += gaussWeights2 * irrad2tap.xyz;
	diffuseLight += gaussWeights3 * irrad3tap.xyz;
	diffuseLight += gaussWeights4 * irrad4tap.xyz;
	diffuseLight += gaussWeights5 * irrad5tap.xyz;
	diffuseLight += gaussWeights6 * irrad6tap.xyz;
	
	vec3 normConst = gaussWeights1 + gaussWeights2 + gaussWeights3 + gaussWeights4 + gaussWeights5 + gaussWeights6;
	diffuseLight /= normConst; // Renormalize to white diffuse light


	//是否渲染半透明效果
	if(useTranslucent){
		vec2 stretchTap = texture( StretchTex, oTex ).xy;
		float stretchval = 0.5 * (stretchTap.x + stretchTap.y);
		vec4 a_values = vec4(0.433, 0.753, 1.412, 2.722);
		vec4 inv_a = -1.0 / (2.0 * a_values * a_values );
		float textureScale = 800.0 * 0.1 / stretchval;
	
		// Compute global scatter from modified TSM  
		float texDist, blend, blendFactor3, blendFactor4, blendFactor5;
		vec4 thickness_mm, fades;

		// 根据当前片段的纹理坐标索引背面离光最近点的纹理坐标
		vec3 TSMtap0 = texture( shadowTex, UV0).xyz; 
		texDist = length(oTex.xy - TSMtap0.yz);
		
		// Four average thicknesses through the object (in mm)  
		thickness_mm = DepthScale * RealDepth ( vec4( irrad2tap.w, irrad3tap.w,  irrad4tap.w, irrad5tap.w ), vec2(ZNear, ZFar));
		fades = exp( thickness_mm * thickness_mm * inv_a );  
		blend = textureScale *  texDist;

		blendFactor3 = saturate(blend / ( a_values.y * 6.0) );  
		blendFactor4 = saturate(blend / ( a_values.z * 6.0) );  
		blendFactor5 = saturate(blend / ( a_values.w * 6.0) );  
		
		/*
		diffuseLight += gaussWeights4  * fades.y * blendFactor3 * texture( blurredTex4, TSMtap0.yz ).xyz / normConst;  
		diffuseLight += gaussWeights5  * fades.z * blendFactor4 * texture( blurredTex5, TSMtap0.yz ).xyz / normConst; 
		diffuseLight += gaussWeights6  * fades.w * blendFactor5 * texture( blurredTex6, TSMtap0.yz ).xyz / normConst;
		*/
		diffuseLight +=  texture( blurredTex4, TSMtap0.yz ).xyz *5.0;  
		diffuseLight +=  texture( blurredTex5, TSMtap0.yz ).xyz *5.0; 
		diffuseLight +=  texture( blurredTex6, TSMtap0.yz ).xyz *5.0;

	}

	// Determine skin color from a diffuseColor map,加上漫反射光
	diffuseLight *=  pow(diffuse.rgb, vec3(1.0 - mix));

	// Constant for specular calculation，计算镜面光

	// Calculate vectors for light shading:
	vec3 lightVector = normalize(lightPos - oWorldPos);
	vec3 viewVector = normalize(cameraPos - oWorldPos);
	
	// calculate specular lighting------------------------------------使用KSK模型计算高光
	float specular = specularKSK(beckmannTex, normal, lightVector, viewVector, m);

	//注意：这里我没有加上阴影，阴影部分计算有点问题
	//FragColor = vec4(diffuseLight + vec3(specular), 1.0 );  
	FragColor = vec4(diffuseLight * lambert + vec3(specular), 1.0);  

	if (!useBlur) {
		//FragColor = diffuse + vec4(specular);//
		FragColor = diffuse * lambert+ vec4(specular);
		FragColor.a = 1.0f;
	}
}