#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Shader.h>
#include <Camera.h>
#include <Model.h>
#include "InputSystem.h"
using namespace glm;
class Renderer {
public:
	Renderer();
	virtual ~Renderer(void);
	virtual void processInput(GLFWwindow *window);
	bool initialize();
	virtual void RenderLoop();
	//virtual void UpdateScene(float msec);
protected:
#pragma region allTheFunctions
	
	void framebuffer_size_callback(GLFWwindow* window, int width, int height);
	void mouse_callback(GLFWwindow* window, double xpos, double ypos);
	void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	void generateTexture(GLuint &into, float width, float height, bool depth_stencil = false);
	void deleteAll();
	void preSetFrameBuffers();
	void preSetShaders();
	void beckmannTextureBuffer();
	void shadowMapBuffer();
	void TSMBuffer();
	void convolved();
	void stretchMapBuffer();
	void unwrapBuffer();
	void blurBuffer();
	void OpenGLAndOthersVariables();
	void SetCurrentShader(Shader* s);
	void UpdateShaderMatrices();
	void RenderScene();
	void TSMRenderScene();
	void computeBeckmannTex();
	void computeStretchMap();
	void shadowPass();
	void TSMPass();
	void convolutionStretch(GLuint &sourceTex, GLuint &targetTex, int num);
	void newStretchMapPass();
	void convolution(GLuint &sourceTex, GLuint &targetTex, int num);
	void irradianceMapPass();
	void unwrapMesh();
	void blurPass();
	void uvPass(GLuint &sourceTex, GLuint &targetTex);
	void RenderFinalTSM();
	void mainPass();
	void debugPass();
	void debugPassTsm();
	void drawModel();
	void drawModel(Model* mesh);
	void renderQuad();
	void SetShaderLight();
	void drawOriginalModel();
	void drawMucusLayer();
	void drawImGUI();
	void initImGUI();
	void exportRenderingResultImage(const char * fileName);
#pragma endregion
	GLFWwindow* window;
	InputSystem inputSystem;
	//模型的类型
	enum ModelType
	{
		Lungs1, Lungs2, Liver1, Liver3, Heart, Heart2, Spleen
	};
#pragma region matrixs
	//一些矩阵
	mat4 projMatrix;		//Projection matrix
	mat4 modelMatrix;	//Model matrix. NOT MODELVIEW
	mat4 viewMatrix;		//View matrix
	mat4 textureMatrix;	//Texture matrix
	mat4 shadowMatrix;
	//光照空间的变换矩阵
	mat4 lightProjMatrix;		//Projection matrix
	mat4 lightViewMatrix;		//View matrix
	float biasValues[16] = {
		0.5, 0.0, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.0, 0.5, 0.0,
		0.5, 0.5, 0.5, 1.0
	};
	//mat4 biasMatrix=make_mat4(biasValues);
	mat4 biasMatrix = mat4(1.0f);//-----------------------------------not--------------
	float gConvolutionScale[5] = { /*0.0064, */0.0484f, 0.187f, 0.567f, 1.99f, 7.41f };

#pragma endregion
#pragma region widthAndHeight
	// settings
	const unsigned int SCR_WIDTH = 1800;//1800
	const unsigned int SCR_HEIGHT = 900;//900
	int		width = 1800;			//Render area width (not quite the same as window width)1900
	int		height = 900;			//Render area height (not quite the same as window height)1024


	float zNear = 0.1f;
	float zFar = 100.0f;
#pragma endregion
	// camera
	//Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
	Camera* camera = new Camera(vec3(0.364f, -0.030f, 0.482f));
	float lastX = SCR_WIDTH / 2.0f;
	float lastY = SCR_HEIGHT / 2.0f;
	bool firstMouse = true;
	// timing
	float deltaTime = 0.0f;
	float lastFrame = 0.0f;
	//旋转角度
	float radians0 = 0;
	float radians1 = 0;
	float radians2 = 0;
	Model* headMesh;//----------------------这是物体的
	Model* mucusMesh;//----------------------这是物体的
	//Model *lightMesh;//----------------------这是光照的
#pragma region shaders
	Shader *currentShader;//----------------当前使用的着色器
	// Shaders
	Shader *basicShader;
	Shader *lightShader;
	mat4 lightMatrix;
	Shader *beckmannShader;
	Shader *shadowShader;
	Shader *stretchShader;
	Shader *unwrapShader;
	Shader *blurShader;
	Shader *mainShader;
	Shader *mucusShader;
	Shader *TSMShader;//半透明阴影贴图
	Shader *newStretchShader;
	Shader *irradianceShader;
	Shader *convolutionShader;
	Shader *finalShader;
	Shader *debugShader;
#pragma endregion

#pragma region preDefine
	// Beckmann Texture buffer
	GLuint beckmannFBO;
	GLuint beckmannTex;

	// stretch buffer
	GLuint stretchFBO;
	GLuint stretchColourTex;
	GLuint stretchDepthTex;
	// Shadow map
	GLuint shadowFBO;
	GLuint shadowTex;
	// TSM map
	GLuint TSMFBO;
	GLuint TSMTex;
	GLuint TSMDepth;

	// 1 non-convolved & 5 convolved irradiance textures
	GLuint nonBlurredTexture;
	GLuint blurredTexture[5];
	// unwrap buffer
	GLuint unwrapFBO;
	GLuint unwrapDepthTex;

	//newStretchMap buffer
	GLuint newStretchFBO;
	GLuint newStretchTexture[6];
	//irradiance buffer
	GLuint irradianceFBO;
	GLuint irradianceTex[6];
	GLuint irradianceDepthTex;

	//convolution
	GLuint convolutionTempColourTex;
	GLuint convolutionStretchTempColourTex;
	// blur buffer
	GLuint blurFBO;
	GLuint tempColourTex;
	/*
		每个模型对应的mask贴图
	*/
	GLuint seamMask;

	// bool variables
	bool firstFrame;
	bool firstFrame1;
	bool useBlur;//是否使用模糊
	bool useStretch;
	bool useTranslucent = true;//是否使用半透明阴影贴图
	bool useTranslucentOnly = false;//单独使用半透明
	bool useTSMRender = true;//渲染器的类型：带有半透明效果
	bool	init;	//Did the renderer initialise properly?
	bool isOriginal;
	bool isMucusDrawed = false;//是否绘制黏液层
	//imGui相关参数
	bool subsurfaceScatteringEnabled = true;
	float forwardScatteringFactor = 0.4;
	float backwardScatteringFactor = 0.6;
	bool taaEnabled = true;
	float roughness = 0.85;
	float reflectivity = 0.158;
	bool isMouseMove = true;
	bool shadowDrawed = true;//是否显示阴影
	int modelType;//模型的种类
	//纹理映射的方法
	int mappingType = 1;
	float map_r = 2.156;//110.0
	float map_z1 = 3.1;//130.0
	//黏液层纹理的选择
	bool isMucusNoTransTexture = false;
	//Cook-Torrance BRDF参数
	float o_metalic = 0.170;
	float o_roughness = 0.523f;
#pragma endregion
	// load textures
	// -------------
	unsigned int rouTexture;//--------------------------
	unsigned int mucusNoTransTexture;//表示没有透明度的第一个黏液层纹理
#pragma region light
// light position光照位置2.0099986f, 0.0f, 2.6999984f右前方
	vec3 lightPos = vec3(2.0099986f, 0.0f, 2.6999984f);
	float lightRadius = 10.0f;
	// light target光照目标方向以及颜色
	vec3 lightTarget = vec3(0.0f, 0.0f, 0.0f);
	//100.0f, 100.0f, 100.0f, 100.0f
	vec4 lightColour = vec4(1.0f, 1.0f, 1.0f, 1.0f);//1.0f, 1.0f, 1.0f, 1.0f
#pragma endregion
};