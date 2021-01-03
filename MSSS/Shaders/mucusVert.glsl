#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;

out vec2 TexCoords;
out vec3 normalDirection;
out vec3 tangentDirection;
out vec3 WorldPos;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

float ans=7e-5;
float PI=3.14159265359;
float fix=0.5;//1.5


float r=110.0;//heart1:12.0 heart2:3.0 kidney:1.2 lungs1:ORIGINAL lungs2:2.7 heart:15.0 liver:110.0  liver3:8.0
float diff=0.5/r;
float z1=130.8;//heart1:30.0 heart2:3.1 kidney:6.0 lungs1:ORIGINAL lungs2:4.5 heart:25.0 liver:130.0 liver3:4.8


void main()
{
    float scale=0.0002;//normal:0.2 heart:0.002 kidney:0.002 lungs1:0.0002 heart:0.002
	vec3 covering=aPos.xyz+aNormal*scale;

	float mid=(2*z1*covering.z-2*z1*z1)*(2*z1*covering.z-2*z1*z1)-4*(covering.x*covering.x+covering.y*covering.y+(covering.z-z1)*(covering.z-z1))*(z1*z1-r*r);
	float k=(-2*z1*covering.z+2*z1*z1-sqrt(mid))/(2*(covering.x*covering.x+covering.y*covering.y+(covering.z-z1)*(covering.z-z1)));
	float sx=(k*covering.x);
	float sy=(k*covering.y);
	float sz=(k*(covering.z-z1)+z1);

	if(covering.z>=0.0){//sz
		//r=70.0
		TexCoords.x=covering.x/sqrt(r*r+r*covering.z)*0.5+fix;//*0.5+fix
		TexCoords.y=1.0-(covering.y/sqrt(r*r+r*covering.z) );//*0.5+fix

		//针对球体-点云模型球面映射方法,改变z1>r，r设置为110,covering.z>=0.0改为sz>=0.0
		//TexCoords.x=sx/sqrt(1.0+sz)*0.5+fix;
		//TexCoords.y=1.0-sy/sqrt(1.0+sz);//*0.5-fix

	}else{
		//r=70.0
		TexCoords.x=covering.x/sqrt(r*r-r*covering.z)*0.5+fix;
		TexCoords.y=1.0-(covering.y/sqrt(r*r-r*covering.z) );//*0.5+fix

		//针对球体-点云模型球面映射方法，改变z1>r，r设置为110
		//TexCoords.x=sx/sqrt(1.0-sz)*0.5+fix;
		//TexCoords.y=1.0-sy/sqrt(1.0-sz);//*0.5-fix

	}

	mat4 model_inverse;
	model_inverse=inverse(modelMatrix);
	normalDirection = normalize(vec3(vec4(aNormal, 0.0) * model_inverse));
    tangentDirection = normalize(vec3(modelMatrix * vec4(vec3(aTangent), 0.0)));
	WorldPos = vec3(modelMatrix * vec4(covering, 1.0));
	TexCoords=aTexCoords;//----------------------原生的纹理坐标
    gl_Position = projMatrix * viewMatrix * modelMatrix * vec4(covering, 1.0);
}