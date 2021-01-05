struct Light
{
	vec3 Position;	// Positon in world space
	vec3 Color;	
	float Amount;
	vec2 NearFarPlane;
};	

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
