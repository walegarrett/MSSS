 #version 330

// #include "Common.include.glsl"

uniform mat4 modelMatrix;// World Transform

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;
uniform sampler2D shadowTex;

uniform vec3 lightPos;
uniform vec4 lightColour;
uniform float lightRadius;

uniform vec3 cameraPos;

uniform bool isShadowVSM;
uniform float mix;// = 0.4;//0.4

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

uniform float ZNear;
uniform float ZFar;

//------------------------------------------------------------
#define GlossyNumMipmap 8
#define SHADOW_MAP_SIZE 1024
#define SHADOW_BIAS 0.001
#define saturate(value) clamp(value, 0.0, 1.0)


float NormalizedDepth(float depth, float zNear, float zFar)
{
	return (depth - zNear) / (zFar - zNear);
}

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
	//depth表示当前深度
    return (texture(shadowTex, UV).r < depth) ? 0.0 : 1.0;
}

float Shadow1(vec2 UV, sampler2D shadowTex, float depth)
{
	//depth表示当前深度
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

//-----------------------------------------------------------
void main()
{
	// lighting parameters
	vec3 L0 = normalize( lightPos - oWorldPos ); // point light 0 light vector

	// compute world normal
	vec3 N_nonBumped = normalize( oWorldNormal );

	// compute bumped world normal
    vec3 objNormal = texture( texture_normal1, oTex ).xyz * vec3( 2.0, 2.0, 2.0 ) - vec3( 1.0, 1.0, 1.0 );  
	mat3 TBN = mat3(IN.tangent, IN.binormal, IN.normal);
    vec3 N_bumped = normalize( mat3(modelMatrix) * objNormal );
	//vec3 N_bumped = normalize( TBN * objNormal );

	// N dot L
	float bumpDot_L0 = dot( N_bumped, L0 );

	//float L0atten = 600 * 600 / dot( LightPos - oWorldPos, LightPos - oWorldPos);
	float L0atten = 1.0;

	//当前的阴影深度
	float depth0 = NormalizedDepth(oLightDist, ZNear, ZFar);
	//用于从半透明阴影贴图中索引离光最近的深度，坐标
	vec2 UV0 = ShadowTexCoord(oShadowCoord);


	float L0Shadow = chebyshevUpperBound(depth0, texture2D(shadowTex, UV0).ra);
	if(isShadowVSM){
		L0Shadow = chebyshevUpperBound(depth0, texture2D(shadowTex, UV0).ra);
	}else{
		L0Shadow = Shadow(UV0, shadowTex, depth0 - SHADOW_BIAS);
	}

	//-------------------------------计算光照
	vec4 diffuse = texture(texture_diffuse1, oTex);

	//vec3 normal = N_bumped;
	TBN = mat3(IN.tangent, IN.binormal, IN.normal);
	vec3 normal = normalize(TBN * (texture(texture_normal1, IN.texCoord).rgb * 2.0 - 1.0));


	vec3 incident = L0;
	float lambert = max(0.0, dot(incident, normal));
	
	//加上阴影---------阴影计算错误
	lambert *= L0Shadow;
	//lambert *=  Shadow(UV0, shadowTex, depth0 - SHADOW_BIAS);
	//lambert *= 1.0f;


	//--
	vec3 viewDir = normalize(cameraPos - oWorldPos);
	vec3 halfDir = normalize(incident + viewDir);
	
	float fresnel = FresnelReflectance(halfDir, viewDir, 0.028);

    vec3 finalCol = lambert * pow(diffuse.rgb, vec3(mix)); 


	//----------------------------------------------------------------------------------------------
	// Compute thickness
	float distanceToLight = depth0; // depth0 range 0...1

	vec4 TSMTap = texture(shadowTex, UV0);
	
	vec3 objNormalBack = texture( texture_normal1, TSMTap.yz).xyz * vec3( 2.0, 2.0, 2.0 ) - vec3( 1.0, 1.0, 1.0 );  
    vec3 N_bumpedBack = normalize( mat3(modelMatrix) * objNormalBack ); 
	//TBN = mat3(IN.tangent, IN.binormal, IN.normal);
	//vec3 N_bumpedBack = normalize( TBN * objNormalBack ); 

	float backFacingEst = saturate( -dot( N_bumpedBack, N_bumped ) );  
    float thicknessToLight = distanceToLight - TSMTap.x;  
	
	// Set a large distance for surface points facing the light  
    if( bumpDot_L0 > 0.0 )  
    {  
		thicknessToLight = 100.0; //100.0 
    } 
	 
    float correctedThickness = saturate( -bumpDot_L0 ) * thicknessToLight;  
    float finalThickness = mix( thicknessToLight, correctedThickness, backFacingEst );  
   
   //测试
    //finalCol = diffuse.rgb;//可以成功
	FragColor = vec4(finalCol, finalThickness);
}
