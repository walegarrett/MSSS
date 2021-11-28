#include "Renderer.h"
#include <iostream>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include<FreeImage.h>
//预定义
#define ZNEAR		0.1f
#define ZFAR		100.0f
#define FOV			90.0f
#define MAP_SIZE	(2048)//1024
unsigned int loadTexture(const char *path);
/*
	构造函数
*/
Renderer::Renderer() {
	
}
/*
	析构函数
*/
Renderer::~Renderer(void) {
	deleteAll();
	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
}
bool Renderer::initialize() {
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

	// glfw window creation
	// --------------------
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "MSSS", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(window);
	/*glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);*/

	// 是否捕获鼠标
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return false;
	}

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	// load models

	//-------------------------------LUNGS1------------------------------------------------
	headMesh = new Model("resources/models/lungs/lungs1/lungs1.obj");//lungs1.obj
	mucusMesh = new Model("resources/models/lungs/lungs1/lungs1_1.obj");//粘液覆盖 lungs1_1.obj

	modelType = ModelType::Lungs1;

	seamMask = loadTexture("resources/models/liver/liverMask.png");
	mucusNoTransTexture = loadTexture("resources/texture/sss/head2.png");
	//-----------------------------------------------------------------------------------
	// draw in wireframe-------------------------------------------线框模式
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	//设置OpenGl山下文
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync


	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 150");

	// render loop
	// -----------
	//---------------------------------------一些预设置------------------------------------------
	preSetFrameBuffers();
	preSetShaders();
	rouTexture = loadTexture("D:/visual studio 2017 codes/repos/OpenGL/OpenGL/resources/models/liver/Roughness.png");

	//初始化成功
	return true;
}
void Renderer::RenderLoop() {
	//-------------------------------------------------------------------------------------------
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		// input
		processInput(window);

		//设置gui图形界面
		initImGUI();

		drawImGUI();

		//
		//------------------------------------------------------渲染函数----------------------------------------------
		if (!useTSMRender) {
			RenderScene();
		}
		else {

			TSMRenderScene();
		}

		//
		//-------------------------------------------------------------------------------------------------------------

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}
#pragma region frameBuffersAndShadersFunctions
void Renderer::preSetFrameBuffers() {
#pragma region beckmann texture buffer
	// diffuse texture
	generateTexture(beckmannTex, MAP_SIZE, MAP_SIZE);

	// frame buffer
	glGenFramebuffers(1, &beckmannFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, beckmannFBO);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, beckmannTex, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !beckmannTex)
	{
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#pragma endregion
#pragma region shadow map buffer

	// depth texture
	glGenTextures(1, &shadowTex);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, MAP_SIZE, MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);

	glBindTexture(GL_TEXTURE_2D, 0);

	// frame buffer
	glGenFramebuffers(1, &shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);
	//glDrawBuffer(GL_NONE);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#pragma endregion
#pragma region TSM buffer
	// frame buffer
	glGenFramebuffers(1, &TSMFBO);

	//半透明阴影贴图颜色纹理
	glGenTextures(1, &TSMTex);
	glBindTexture(GL_TEXTURE_2D, TSMTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, MAP_SIZE, MAP_SIZE, 0, GL_RGBA, GL_INT, NULL);

	glBindFramebuffer(GL_FRAMEBUFFER, TSMFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TSMTex, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// depth texture
	glGenTextures(1, &TSMDepth);
	glBindTexture(GL_TEXTURE_2D, TSMDepth);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, MAP_SIZE, MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);

	//glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, TSMFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, TSMDepth, 0);

	//glDrawBuffer(GL_NONE);

	/*if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		return;
	}*/

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#pragma endregion
#pragma region newStretch textures
	// frame buffer
	glGenFramebuffers(1, &newStretchFBO);
	for (int i = 0; i < 6; ++i)
	{
		generateTexture(newStretchTexture[i], MAP_SIZE, MAP_SIZE);
	}
#pragma endregion
#pragma region 1 non-convolved & 5 convolved irradiance textures
	generateTexture(nonBlurredTexture, MAP_SIZE, MAP_SIZE);

	for (int i = 0; i < 5; ++i)
	{
		generateTexture(blurredTexture[i], MAP_SIZE, MAP_SIZE);
	}
#pragma endregion
#pragma region stretch map buffer
	// depth texture
	generateTexture(stretchDepthTex, MAP_SIZE, MAP_SIZE, true);

	// colour texture
	generateTexture(stretchColourTex, MAP_SIZE, MAP_SIZE);

	// frame buffer
	glGenFramebuffers(1, &stretchFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, stretchFBO);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, stretchDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, stretchDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, stretchColourTex, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#pragma endregion
#pragma region unwrap buffer
	// depth texture
	generateTexture(unwrapDepthTex, MAP_SIZE, MAP_SIZE, true);

	// frame buffer
	glGenFramebuffers(1, &unwrapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, unwrapFBO);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, unwrapDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, unwrapDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, nonBlurredTexture, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#pragma endregion
#pragma region irradiance buffer
	// depth texture
	generateTexture(irradianceDepthTex, MAP_SIZE, MAP_SIZE, true);

	// frame buffer
	glGenFramebuffers(1, &irradianceFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, irradianceFBO);
	for (int i = 0; i < 6; ++i)
	{
		generateTexture(irradianceTex[i], MAP_SIZE, MAP_SIZE);
	}

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, irradianceDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, irradianceDepthTex, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#pragma endregion
#pragma region blur buffer
	// colour texture
	generateTexture(tempColourTex, MAP_SIZE, MAP_SIZE);

	// frame buffer
	glGenFramebuffers(1, &blurFBO);
#pragma endregion
#pragma region convolution buffer
	// colour texture
	generateTexture(convolutionTempColourTex, MAP_SIZE, MAP_SIZE);
	generateTexture(convolutionStretchTempColourTex, MAP_SIZE, MAP_SIZE);
#pragma endregion
#pragma region OpenGL & others variables
	glEnable(GL_DEPTH_TEST);
	projMatrix = glm::perspective(glm::radians(camera->Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
	//projMatrix = glm::perspective(ZNEAR, ZFAR, (float)width / (float)height, FOV);
	firstFrame = true;
	firstFrame1 = true;
	useBlur = true;
	useStretch = true;
	init = true;
	isOriginal = false;
	isMucusDrawed = false;
#pragma endregion
}
void Renderer::preSetShaders() {
#pragma region shaders
	//build and compile shaders
	//----------------------------------------------------------------------------------------
	// -----------------------------------------basic
	basicShader = new Shader("Shaders/basicVert.glsl", "Shaders/basicFrag.glsl");
	//basicShader = new Shader("Shaders/pbrVert.glsl", "Shaders/pbrFrag.glsl");
	//basicShader = new Shader("Shaders/noSSSVert.glsl", "Shaders/noSSSFrag.glsl");
	//------------------------------------------beckmann
	beckmannShader = new Shader("Shaders/beckmannVert.glsl", "Shaders/beckmannFrag.glsl");
	//-------------------------------------------shadow
	shadowShader = new Shader("Shaders/shadowVert.glsl", "Shaders/shadowFrag.glsl");
	//-------------------------------------------stretch
	stretchShader = new Shader("Shaders/stretchVert.glsl", "Shaders/stretchFrag.glsl");
	//--------------------------------------------unwrap
	unwrapShader = new Shader("Shaders/unwrapVert.glsl", "Shaders/unwrapFrag.glsl");
	//-----------------------------------------------blur
	blurShader = new Shader("Shaders/blurVert.glsl", "Shaders/blurFrag.glsl");
	//--------------------------------------------main
	mainShader = new Shader("Shaders/mainVert.glsl", "Shaders/mainFrag.glsl");
	//-------------------------------------------mucus
	mucusShader = new Shader("Shaders/mucusVert.glsl", "Shaders/mucusFrag.glsl");
	//-------------------------------------------TSM
	TSMShader = new Shader("Shaders/tsmVert.glsl", "Shaders/tsmFrag.glsl");
	newStretchShader = new Shader("Shaders/newStretchVert.glsl", "Shaders/newStretchFrag.glsl");
	irradianceShader = new Shader("Shaders/irradianceVert.glsl", "Shaders/irradianceFrag.glsl");
	convolutionShader = new Shader("Shaders/convolutionVert.glsl", "Shaders/convolutionFrag.glsl");
	finalShader = new Shader("Shaders/finalVert.glsl", "Shaders/finalFrag.glsl");
	debugShader = new Shader("Shaders/debugVert.glsl", "Shaders/debugFrag.glsl");

#pragma endregion
}
void Renderer::beckmannTextureBuffer() {
#pragma region beckmann texture buffer
	// diffuse texture
	generateTexture(beckmannTex, MAP_SIZE, MAP_SIZE);

	// frame buffer
	glGenFramebuffers(1, &beckmannFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, beckmannFBO);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, beckmannTex, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !beckmannTex)
	{
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#pragma endregion
}
void Renderer::shadowMapBuffer() {
#pragma region shadow map buffer
	// depth texture
	glGenTextures(1, &shadowTex);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, MAP_SIZE, MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);

	glBindTexture(GL_TEXTURE_2D, 0);

	// frame buffer
	glGenFramebuffers(1, &shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);
	glDrawBuffer(GL_NONE);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#pragma endregion
}
void Renderer::TSMBuffer() {
	// frame buffer
	glGenFramebuffers(1, &TSMFBO);

	//半透明阴影贴图颜色纹理
	glGenTextures(1, &TSMTex);
	glBindTexture(GL_TEXTURE_2D, TSMTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, MAP_SIZE, MAP_SIZE, 0, GL_RGBA, GL_INT, NULL);

	glBindFramebuffer(GL_FRAMEBUFFER, TSMFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TSMTex, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// depth texture
	glGenTextures(1, &TSMDepth);
	glBindTexture(GL_TEXTURE_2D, TSMDepth);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, MAP_SIZE, MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);

	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, TSMFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, TSMDepth, 0);
	glDrawBuffer(GL_NONE);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void Renderer::convolved() {
#pragma region 1 non-convolved & 5 convolved irradiance textures
	generateTexture(nonBlurredTexture, MAP_SIZE, MAP_SIZE);

	for (int i = 0; i < 5; ++i)
	{
		generateTexture(blurredTexture[i], MAP_SIZE, MAP_SIZE);
	}
#pragma endregion
}
void Renderer::stretchMapBuffer() {
#pragma region stretch map buffer
	// depth texture
	generateTexture(stretchDepthTex, MAP_SIZE, MAP_SIZE, true);

	// colour texture
	generateTexture(stretchColourTex, MAP_SIZE, MAP_SIZE);

	// frame buffer
	glGenFramebuffers(1, &stretchFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, stretchFBO);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, stretchDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, stretchDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, stretchColourTex, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#pragma endregion
}
void Renderer::unwrapBuffer() {
#pragma region unwrap buffer
	// depth texture
	generateTexture(unwrapDepthTex, MAP_SIZE, MAP_SIZE, true);

	// frame buffer
	glGenFramebuffers(1, &unwrapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, unwrapFBO);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, unwrapDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, unwrapDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, nonBlurredTexture, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#pragma endregion
}
void Renderer::blurBuffer() {
#pragma region blur buffer
	// colour texture
	generateTexture(tempColourTex, MAP_SIZE, MAP_SIZE);

	// frame buffer
	glGenFramebuffers(1, &blurFBO);
#pragma endregion
}
void Renderer::OpenGLAndOthersVariables() {
#pragma region OpenGL & others variables
	projMatrix = glm::perspective(glm::radians(camera->Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
	//projMatrix = glm::perspective(ZNEAR, ZFAR, (float)width / (float)height, FOV);
	firstFrame = true;
	useBlur = true;
	useStretch = true;
	init = true;

#pragma endregion
}
#pragma endregion

void Renderer::RenderScene()//---------------------------------------------------真实渲染场景，也就是渲染循环
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (firstFrame)
	{
		computeBeckmannTex();//一开始计算beckman纹理
		computeStretchMap();//这里计算拉伸矫正贴图

		firstFrame = false;
	}

	if (shadowDrawed) {
		shadowPass();
		//测试
		//debugPass();
	}
	unwrapMesh();
	blurPass();


	//是否加上次表面散射的效果
	if (!subsurfaceScatteringEnabled) {
		drawOriginalModel();
		// clean up
		glUseProgram(0);
	}
	else {
		//绘制SSS
		mainPass();
	}
	//是否需要绘制黏液层
	if (isMucusDrawed) {
		drawMucusLayer();
	}
	// clean up
	glUseProgram(0);
}
void Renderer::TSMRenderScene()//---------------------------------------------------真实渲染场景，也就是渲染循环
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (firstFrame1)
	{
		computeBeckmannTex();//一开始计算beckman纹理
		newStretchMapPass();//这里计算拉伸矫正贴图

		firstFrame1 = false;
	}

	TSMPass();

	//测试阴影贴图
	//debugPassTsm();

	////卷积辐照度贴图
	irradianceMapPass();


	////是否加上次表面散射的效果
	if (!subsurfaceScatteringEnabled) {
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);

		drawOriginalModel();
		// clean up
		glUseProgram(0);
	}
	else {
		//绘制SSS
		//启用混合造成半透明等混合效应
		//glEnable(GL_BLEND);//启动混合
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//设置相应的混合函数即源因子值和目标因子值
		RenderFinalTSM();
	}
	//是否需要绘制黏液层
	if (isMucusDrawed) {
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);

		drawMucusLayer();
	}
	// clean up
	glUseProgram(0);
}
void Renderer::initImGUI() {
	// 启动IMgui Frame框架.
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// ImGUI组件定义开始
	ImGui::Begin("Subsurface Scattering Demo");
	
	//分割线
	ImGui::Separator();
	//渲染器的种类
	ImGui::Checkbox("Using TSMRenderer", &useTSMRender);
	//是否使用次表面散射
	ImGui::Checkbox("Subsurface Scattering", &subsurfaceScatteringEnabled);
	//前向散射分量和后向散射分量
	ImGui::SliderFloat("Forward Scattering Mix", &forwardScatteringFactor, 0.00f, 1.00f);
	ImGui::SliderFloat("Backward Scattering Mix", &backwardScatteringFactor, 0.00f, 1.00f);
	//模糊操作
	ImGui::Checkbox("Blur Drawed", &useBlur);
	//渲染黏液层
	ImGui::Checkbox("Mucus Layer", &isMucusDrawed);
	//控制高光
	ImGui::SliderFloat("Roughness", &roughness, 0.00f, 1.00f);
	ImGui::SliderFloat("Base Reflectivity", &reflectivity, 0.00f, 1.00f);
	//是否绘制阴影
	ImGui::Checkbox("Shadow Map", &shadowDrawed);
	//是否绘制半透明效果
	ImGui::Checkbox("Translucent", &useTranslucent);
	//是否单独绘制背光
	ImGui::Checkbox("Translucent Only", &useTranslucentOnly);

	//分割线
	ImGui::Separator();

	//	纹理映射的方法
	if (ImGui::RadioButton("Spherical mapping", true)) {
		mappingType = 1;
	}else if (ImGui::RadioButton("PointCloud mapping", true)) {
		mappingType = 2;
	}else if (ImGui::RadioButton("Original mapping", true)) {
		mappingType = 3;
	}
	//纹理映射的参数
	ImGui::SliderFloat("mapping_r", &map_r, 0.00f, 200.00f);
	ImGui::SliderFloat("mapping_z1", &map_z1, 0.00f, 200.00f);

	ImGui::Separator();

	//CookTorrance模型相关参数
	ImGui::SliderFloat("O_METALIC", &o_metalic, 0.00f, 1.00f);
	ImGui::SliderFloat("O_ROUGHNESS", &o_roughness, 0.00f, 1.00f);

	//分割线
	ImGui::Separator();

	//渲染信息的显示
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	// ImGUI组件定义结束
	ImGui::End();

	//分割线
	ImGui::Separator();

	//模型的选择
	if (ImGui::RadioButton("Spleen", true)) {//黏液层的纹理映射方法设置为原始的映射
		modelType = ModelType::Spleen;
		headMesh = new Model("resources/models/spleen/spleen1/spleen4.obj");//
		mucusMesh = new Model("resources/models/spleen/spleen1/spleen4_1.obj");//
	}
	else if (ImGui::RadioButton("Lungs", true)) {
		modelType = ModelType::Lungs1;
		headMesh = new Model("resources/models/lungs/lungs1/lungs1.obj");//lungs1.obj
		mucusMesh = new Model("resources/models/lungs/lungs1/lungs1_1.obj");//粘液覆盖 lungs1_1.obj
	}
	else if (ImGui::RadioButton("Lungs2", true)) {//黏液层纹理使用的是原始坐标映射
		modelType = ModelType::Lungs2;
		headMesh = new Model("resources/models/lungs/lungs2/lungs2.obj");//
		mucusMesh = new Model("resources/models/lungs/lungs2/lungs2_1.obj");//
	}
	else if (ImGui::RadioButton("Liver1", true)) {
		modelType = ModelType::Liver1;
		headMesh = new Model("resources/models/liver/liver.obj");//liver.obj
		mucusMesh = new Model("resources/models/liver/liver1.obj");//粘液覆盖 liver1.obj
		seamMask = loadTexture("resources/models/liver/liverMask.png");
	}
	else if (ImGui::RadioButton("Liver3", true)) {
		modelType = ModelType::Liver3;
		headMesh = new Model("resources/models/livers/liver3/liver4.obj");//liver3.obj  liver4.obj
		mucusMesh = new Model("resources/models/livers/liver3/liver4_1.obj");//liver3_1.obj liver4_1.obj
	}
	else if (ImGui::RadioButton("Heart", true)) {
		modelType = ModelType::Heart;
		headMesh = new Model("resources/models/heart/heart.obj");//
		mucusMesh = new Model("resources/models/heart/heart1.obj");//
	}
	else if (ImGui::RadioButton("Heart2", true)) {//使用黏液层纹理时使用的是球面映射方法，r=2.156
		modelType = ModelType::Heart2;
		headMesh = new Model("resources/models/hearts/heart2/heart4.obj");//heart2.obj
		mucusMesh = new Model("resources/models/hearts/heart2/heart4_1.obj");//heart2_1.obj
	}

	//导出渲染图片
	if (ImGui::Button("save Rendering Result Image")) {
		string fileName = "results/";
		fileName += headMesh->getFileName();
		fileName = fileName.substr(0, fileName.find_first_of('.'));
		if (useTSMRender) {
			fileName += "-tsm";
		}
		else fileName += "-noTsm";
		if (subsurfaceScatteringEnabled) {
			fileName += "-sss";
		}
		else fileName += "-original";
		if (isMucusDrawed) {
			fileName += "-mucus";
		}
		else fileName += "-noMucus";
		if (useBlur) {
			fileName += "-useBlur";
		}
		else fileName += "-noBlur";
		if (useTranslucent) {
			fileName += "-translucent";
		}
		else fileName += "-noTranslucent";
		std::ostringstream ss;
		ss << forwardScatteringFactor;
		fileName += "-forward_";
		fileName += ss.str(); ss.str("");
		fileName += "-backward_";
		ss << backwardScatteringFactor;
		fileName += ss.str(); ss.str("");
		ss << roughness;
		fileName += "-roughness_";
		fileName += ss.str(); ss.str("");
		ss << reflectivity;
		fileName += "-reflectivity_";
		fileName += ss.str(); ss.str("");
		fileName += ".png";
		exportRenderingResultImage(fileName.c_str());
	}
	
}
void Renderer::drawImGUI() {
	ImGui::Render();
	int display_w, display_h;
	//glfwMakeContextCurrent(window);
	glfwGetFramebufferSize(window, &display_w, &display_h);
	glViewport(0, 0, display_w, display_h);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);


}
void Renderer::drawOriginalModel() {
	//
		//------------------------------------------------------original---------------------------------------------------
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);//0.0f, 0.0f, 0.0f, 0.0f//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//---------设置当前的着色器
	SetCurrentShader(basicShader);//

	if (true) {//!shadowDrawed
		projMatrix = glm::perspective(glm::radians(camera->Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);//----------NOT-----------
		viewMatrix = camera->GetViewMatrix();
		modelMatrix = mat4(1.0f);
	}
	SetShaderLight();
	/*设置光照*/
	currentShader->setVec3("lightPosition", lightPos);
	//currentShader->setVec3("lightPosition", vec3(10.0f, 10.0f, 10.0f));
	//currentShader->setVec3("lightColor", lightColour);
	currentShader->setVec3("lightColor", vec3(50.0f, 50.0f, 50.0f));
	//currentShader->setVec3("albedo", 0.5f, 0.0f, 0.0f);
	currentShader->setFloat("ao", 1.0f);
	currentShader->setFloat("metallic", o_metalic);//0.05
	currentShader->setFloat("roughness", o_roughness);//0.05

	currentShader->setVec3("camPos", camera->Position);
	currentShader->setVec3("cameraPos", camera->Position);

	UpdateShaderMatrices();
	//渲染模型
	drawModel(headMesh);
}
void Renderer::drawMucusLayer() {
	//---------------------------------------------新增---------------------------------
		//启用混合造成半透明等混合效应
	glEnable(GL_BLEND);//启动混合
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//设置相应的混合函数即源因子值和目标因子值
	//--------------------------------------------------------------
	//-----------------------------------------------------渲染黏液层-------------------------------
	SetCurrentShader(mucusShader);
	currentShader->setVec3("camPos", camera->Position);
	currentShader->setVec3("lightPosition", lightPos);
	glm::vec3 lightColor(3.0f, 3.0f, 3.0f);
	currentShader->setVec3("lightColor", lightColor);//光照颜色

	//设置纹理映射方式
	currentShader->setInt("mappingType", mappingType);
	currentShader->setFloat("r", map_r);
	currentShader->setFloat("z1", map_z1);

	//设置黏液层纹理
	if (isMucusDrawed && !subsurfaceScatteringEnabled) {
		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "mucusNoTransTexture"), 2);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, mucusNoTransTexture);
		isMucusNoTransTexture = true;
	}
	else {
		isMucusNoTransTexture = false;
	}
	currentShader->setBool("isMucusNoTransTexture", isMucusNoTransTexture);

	if (true) {//!shadowDrawed
		projMatrix = glm::perspective(glm::radians(camera->Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);//----------NOT-----------
		viewMatrix = camera->GetViewMatrix();
		modelMatrix = mat4(1.0f);
	}

	UpdateShaderMatrices();
	drawModel(mucusMesh);
}

void Renderer::drawModel()
{
	//----------------------------------------HAND----------------------------------------------------
	//注意这里的顺序是先位移再缩放最后再旋转。
	//modelMatrix = glm::mat4(1.0f);
	//modelMatrix = glm::rotate(modelMatrix, 45.0f, glm::vec3(1.0f, 0.0f, 0.0f));//旋转45度，注意度数必须是浮点数的形式
	//modelMatrix = glm::scale(modelMatrix, glm::vec3(4.0f, 4.0f, 4.0f));
	//modelMatrix = glm::translate(modelMatrix, glm::vec3(0.78f, -1.2f, -0.0f)); 

	//-----------------------------------------LIVER---------------------------------------------------
	/*modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.3f, 0.1f, 0.0f));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(0.004f, 0.004f, 0.004f));*/

	//-----------------------------------------head----------------------------------------------------
	//modelMatrix = glm::mat4(1.0f);
	//modelMatrix = glm::translate(modelMatrix, glm::vec3(0.5f, 0.0f, 0.0f));
	//modelMatrix = glm::scale(modelMatrix, glm::vec3(1.5f, 1.5f, 1.5f));

	//-----------------------------------------lungs1---------------------------------------------------
	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.3f, 0.1f, 0.0f));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(1.4f, 1.4f, 1.4f));
	//-----------------------旋转函数
	modelMatrix = glm::rotate(modelMatrix, radians0, glm::vec3(0.0f, 1.0f, 0.0f));//L绕y轴
	modelMatrix = glm::rotate(modelMatrix, radians1, glm::vec3(0.0f, 0.0f, 1.0f));//K绕z轴
	modelMatrix = glm::rotate(modelMatrix, radians2, glm::vec3(1.0f, 0.0f, 0.0f));//J绕x轴
	mat4 tempMatrix = shadowMatrix * modelMatrix;
	currentShader->setMat4("shadowMatrix", tempMatrix);//----------------------------not------------
	viewMatrix = camera->GetViewMatrix();
	UpdateShaderMatrices();

	headMesh->Draw1(currentShader);
}
void Renderer::drawModel(Model* headMesh)
{

	//----------------------------------------HAND----------------------------------------------------
	//注意这里的顺序是先位移再缩放最后再旋转。
	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::rotate(modelMatrix, 45.0f, glm::vec3(1.0f, 0.0f, 0.0f));//旋转45度，注意度数必须是浮点数的形式
	modelMatrix = glm::scale(modelMatrix, glm::vec3(4.0f, 4.0f, 4.0f));
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.78f, -1.2f, -0.0f));

	//根据模型类型设置模型矩阵
	if (modelType == ModelType::Liver1) {
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.3f, 0.1f, 0.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.002f, 0.002f, 0.002f));
	}
	else if (modelType == ModelType::Liver3) {
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.3f, 0.1f, 0.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.01f, 0.01f, 0.01f));
	}
	else if (modelType == ModelType::Lungs1) {
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.3f, -0.1f, 0.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(1.4f, 1.4f, 1.4f));
	}
	else if (modelType == ModelType::Lungs2) {
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.3f, 0.0f, 0.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.05f, 0.05f, 0.05f));
	}
	else if (modelType == ModelType::Spleen) {
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.3f, 0.0f, 0.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.01f, 0.01f, 0.01f));
	}
	else if (modelType == ModelType::Heart) {
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.3f, -0.1f, 0.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.01f, 0.01f, 0.01f));
	}
	else if (modelType == ModelType::Heart2) {
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.3f, -0.1f, 0.0f));//0.3 0.1
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.05f, 0.05f, 0.05f));//0.003
	}


	//-----------------------旋转函数
	modelMatrix = glm::rotate(modelMatrix, radians0, glm::vec3(0.0f, 1.0f, 0.0f));//L绕y轴
	modelMatrix = glm::rotate(modelMatrix, radians1, glm::vec3(0.0f, 0.0f, 1.0f));//K绕z轴
	modelMatrix = glm::rotate(modelMatrix, radians2, glm::vec3(1.0f, 0.0f, 0.0f));//J绕x轴
	mat4 tempMatrix = shadowMatrix * modelMatrix;
	currentShader->setMat4("shadowMatrix", tempMatrix);//----------------------------not------------
	viewMatrix = camera->GetViewMatrix();
	UpdateShaderMatrices();

	headMesh->Draw1(currentShader);
}
void Renderer::SetCurrentShader(Shader *s) {
	currentShader = s;
	s->use();//激活
}
// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void Renderer::processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera->ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera->ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera->ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera->ProcessKeyboard(RIGHT, deltaTime);
	//旋转函数
	if ((glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)) {
		radians0 = (radians0 + 1.0f);
		if (radians0 >= 360.0f) {
			radians0 = (radians0 - 360.0f);
		}
	}
	if ((glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)) {
		radians1 = (radians1 + 1.0f);
		if (radians1 >= 360.0f) {
			radians1 = (radians1 - 360.0f);
		}
	}
	if ((glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)) {
		radians2 = (radians2 + 1.0f);
		if (radians2 >= 360.0f) {
			radians2 = (radians2 - 360.0f);
		}
	}
	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
		useBlur = !useBlur;
	}
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
		useStretch = !useStretch;
	}
	if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
		isOriginal = !isOriginal;
	}
	if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
		isMucusDrawed = !isMucusDrawed;
	}
	//是否释放鼠标
	//if ((glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)) {
	//	if (isMouseMove) {
	//		glfwSetCursorPosCallback(window, mouse_callback);
	//		glfwSetScrollCallback(window, scroll_callback);

	//		// 是否捕获鼠标
	//		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	//	}
	//	else {
	//		glfwSetCursorPosCallback(window, NULL);
	//		glfwSetScrollCallback(window, scroll_callback);
	//		// 是否捕获鼠标
	//		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	//	}
	//	isMouseMove = !isMouseMove;
	//}
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void Renderer::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void Renderer::mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera->ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void Renderer::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera->ProcessMouseScroll(yoffset);
}
// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}
void Renderer::generateTexture(GLuint &into, float width, float height, bool depth_stencil)
{
	glGenTextures(1, &into);
	glBindTexture(GL_TEXTURE_2D, into);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0,
		depth_stencil ? GL_DEPTH24_STENCIL8 : GL_RGBA8,
		width, height, 0,
		depth_stencil ? GL_DEPTH_STENCIL : GL_RGBA,
		depth_stencil ? GL_UNSIGNED_INT_24_8 : GL_UNSIGNED_BYTE,
		NULL);

	glBindTexture(GL_TEXTURE_2D, 0);
}
void Renderer::deleteAll() {
	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// Beckmann texture buffer
	glDeleteTextures(1, &beckmannTex);
	glDeleteFramebuffers(1, &beckmannFBO);


	// Shadow map buffer
	glDeleteTextures(1, &shadowTex);
	glDeleteFramebuffers(1, &shadowFBO);


	// Textures
	glDeleteTextures(1, &nonBlurredTexture);
	glDeleteTextures(5, blurredTexture);


	// stretch buffer
	glDeleteTextures(1, &stretchColourTex);
	glDeleteTextures(1, &stretchDepthTex);
	glDeleteFramebuffers(1, &stretchFBO);


	// unwrap buffer
	glDeleteTextures(1, &unwrapDepthTex);
	glDeleteFramebuffers(1, &unwrapFBO);


	// blur buffer
	glDeleteTextures(1, &tempColourTex);
	glDeleteFramebuffers(1, &blurFBO);
}
void Renderer::UpdateShaderMatrices() {
	currentShader->setMat4("modelMatrix", modelMatrix);
	currentShader->setMat4("viewMatrix", viewMatrix);
	currentShader->setMat4("projMatrix", projMatrix);
	currentShader->setMat4("textureMatrix", textureMatrix);
}
void Renderer::computeBeckmannTex()
{
	glBindFramebuffer(GL_FRAMEBUFFER, beckmannFBO);
	glViewport(0.0f, 0.0f, MAP_SIZE, MAP_SIZE);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);//0.0f, 0.0f, 0.0f, 0.0f//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);//纯黑
	glClear(GL_COLOR_BUFFER_BIT);
	//shader
	SetCurrentShader(beckmannShader);//--------------------not-----------------------
	// matrices
	projMatrix = glm::ortho(-1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f);
	viewMatrix = mat4(1.0f);
	modelMatrix = mat4(1.0f);
	UpdateShaderMatrices();
	// draw call
	renderQuad();
	// clean up
	glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0.0f, 0.0f, (float)width, (float)height);
}
void Renderer::computeStretchMap() {
	//-------------------------------------------------------------computeStretchMap();---------------------------------
			// set up
	glBindFramebuffer(GL_FRAMEBUFFER, stretchFBO);
	glViewport(0.0f, 0.0f, MAP_SIZE, MAP_SIZE);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);//0.0f, 0.0f, 0.0f, 0.0f//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	// shader
	SetCurrentShader(stretchShader);
	// matrices---------------------------------------not---------------------------
	projMatrix = glm::perspective(glm::radians(camera->Zoom), 1.0f, 0.1f, 100.0f);
	//projMatrix = perspective(ZNEAR, ZFAR, 1.0f, FOV);
	viewMatrix = camera->GetViewMatrix();
	//viewMatrix = camera->BuildViewMatrix();---------------------not------------------
	modelMatrix = mat4(1.0f);
	UpdateShaderMatrices();
	// draw calls
	drawModel(headMesh);
	// clean up
	glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0.0f, 0.0f, (float)width, (float)height);
}
void Renderer::convolutionStretch(GLuint &sourceTex, GLuint &targetTex, int num)
{
	glPushAttrib(GL_VIEWPORT_BIT);
	glm::vec2 stepX = glm::vec2(1.0f, 0.0f) * (1.0f / MAP_SIZE);
	glm::vec2 stepY = glm::vec2(0.0f, 1.0f) * (1.0f / MAP_SIZE);
	currentShader->setVec2("Step", stepX.x, stepX.y);
	currentShader->setFloat("GaussWidth", sqrtf(gConvolutionScale[num]));

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, convolutionStretchTempColourTex, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sourceTex);
	renderQuad();

	currentShader->setVec2("Step", stepY.x, stepY.y);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, targetTex, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, convolutionStretchTempColourTex);
	renderQuad();

	glPopAttrib();
}
void Renderer::newStretchMapPass() {
	// set up
	glBindFramebuffer(GL_FRAMEBUFFER, newStretchFBO);
	//一开始将模型场景渲染到一张颜色纹理中
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, newStretchTexture[0], 0);
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, MAP_SIZE, MAP_SIZE);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);//0.0f, 0.0f, 0.0f, 0.0f//glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	SetCurrentShader(newStretchShader);
	float gStrechScale = 0.0014444444f;
	currentShader->setFloat("Scale", gStrechScale);

	UpdateShaderMatrices();

	drawModel(headMesh);

	glUseProgram(0);
	glPopAttrib();

	SetCurrentShader(convolutionShader);
	modelMatrix = mat4(1.0f);
	viewMatrix = camera->GetViewMatrix();
	viewMatrix = mat4(1.0f);
	//projMatrix = glm::perspective(glm::radians(camera->Zoom), 1.0f, 0.1f, 100.0f);
	projMatrix = ortho(-1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f);
	UpdateShaderMatrices();
	// shader textures
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "InputTex"), 0);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "stretchTex"), 2);

	currentShader->setBool("isBlurDiffusion", false);
	currentShader->setBool("isBlurStretch", true);



	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, stretchColourTex);

	//Utility::SaveTextureToPfm("StrechXY0.pfm", gStretchBuffer[0]->GetColorTex(), gWindowWidth, gWindowWidth);
	convolutionStretch(newStretchTexture[0], newStretchTexture[1], 0);
	convolutionStretch(newStretchTexture[1], newStretchTexture[2], 1);
	convolutionStretch(newStretchTexture[2], newStretchTexture[3], 2);
	convolutionStretch(newStretchTexture[3], newStretchTexture[4], 3);

	// clean up
	glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0.0f, 0.0f, (float)width, (float)height);
}

void Renderer::shadowPass()
{
	//---------------------------------------------------------shadowPass();--------------------------------------
		// set up
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glViewport(0, 0, MAP_SIZE, MAP_SIZE);
	glClear(GL_DEPTH_BUFFER_BIT);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	// shader
	SetCurrentShader(shadowShader);

	// matrices
	//projMatrix = perspective(ZNEAR, ZFAR, 1.0f, FOV);//fov表示视场角
	//---------------------------NOT-----------------------------------------
	projMatrix = glm::perspective(FOV, 1.0f, 0.1f, 100.0f);
	//projMatrix = glm::perspective(glm::radians(camera->Zoom), 1.0f, 0.1f, 100.0f);//----------not---------
	//viewMatrix = Matrix4::BuildViewMatrix(light->GetPosition(), lightTarget);//-----------------
	viewMatrix = glm::lookAt(lightPos, lightTarget, vec3(0.0f, 1.0f, 0.0f));
	modelMatrix = mat4(1.0f);
	//shadowMatrix = biasMatrix * (projMatrix * viewMatrix);
	shadowMatrix = biasMatrix * (projMatrix * viewMatrix);//--------------------not---------------------
	UpdateShaderMatrices();

	// draw calls
	drawModel(headMesh);

	// clean up
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);
}
//渲染半透明阴影贴图
void Renderer::TSMPass() {
	glBindFramebuffer(GL_FRAMEBUFFER, TSMFBO);

	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, MAP_SIZE, MAP_SIZE);

	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	// shader
	SetCurrentShader(TSMShader);

	//lightProjMatrix = glm::perspective(glm::radians(camera->Zoom), 1.0f, zNear, zFar);
	lightProjMatrix = glm::perspective(FOV, 1.0f, zNear, zFar);
	lightViewMatrix = glm::lookAt(lightPos, lightTarget, vec3(0.0f, 1.0f, 0.0f));
	modelMatrix = mat4(1.0f);
	//shadowMatrix = biasMatrix * (projMatrix * viewMatrix);

	currentShader->setFloat("ZNear", zNear);
	currentShader->setFloat("ZFar", zFar);
	currentShader->setBool("isShadowVSM", true);

	currentShader->setMat4("lightViewMatrix", lightViewMatrix);
	currentShader->setMat4("lightProjMatrix", lightProjMatrix);

	UpdateShaderMatrices();

	// draw calls
	drawModel(headMesh);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glPopAttrib();
	glUseProgram(0);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
}
//用于测试帧缓存
void Renderer::debugPass() {
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);//0.0f, 0.0f, 0.0f, 0.0f//glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	//glClearDepth(1.0f);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	SetCurrentShader(debugShader);
	currentShader->setFloat("ZNear", zNear);
	currentShader->setFloat("ZFar", zFar);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "depthMap"), 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	renderQuad();

}
//用于测试帧缓存
void Renderer::debugPassTsm() {
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);//0.0f, 0.0f, 0.0f, 0.0f//glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	//glClearDepth(1.0f);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	SetCurrentShader(debugShader);
	currentShader->setFloat("ZNear", zNear);
	currentShader->setFloat("ZFar", zFar);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "depthMap"), 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, TSMTex);
	renderQuad();

}
void Renderer::unwrapMesh()
{
	//----------------------------------------------------------unwrapMesh();----------------------------------------
		// set up
	glBindFramebuffer(GL_FRAMEBUFFER, unwrapFBO);
	glViewport(0.0f, 0.0f, MAP_SIZE, MAP_SIZE);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);//0.0f, 0.0f, 0.0f, 0.0f//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// shader
	SetCurrentShader(unwrapShader);

	// shader textures
	//glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);
	//glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "bumpTex"), 1);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "shadowTex"), 2);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	// shader light variables---------------------------not---------------------------
	SetShaderLight();
	//glUniform3fv(glGetUniformLocation(currentShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());*/
	currentShader->setVec3("cameraPos", vec3(0.0f, 0.0f, 3.0f));//--------------------not----------------------
	// matrices
	//projMatrix = perspective(ZNEAR, ZFAR, 1.0f, FOV);

	projMatrix = glm::perspective(glm::radians(camera->Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
	viewMatrix = camera->GetViewMatrix();
	modelMatrix = mat4(1.0f);
	UpdateShaderMatrices();

	/*
		设置前向散射系数
	*/
	currentShader->setFloat("mix", forwardScatteringFactor);
	// draw calls
	drawModel(headMesh);

	// clean up
	glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0.0f, 0.0f, (float)width, (float)height);
}
void Renderer::convolution(GLuint &sourceTex, GLuint &targetTex, int num)
{
	glPushAttrib(GL_VIEWPORT_BIT);
	glm::vec2 stepX = glm::vec2(1.0f, 0.0f) * (1.0f / MAP_SIZE);
	glm::vec2 stepY = glm::vec2(0.0f, 1.0f) * (1.0f / MAP_SIZE);
	currentShader->setVec2("Step", stepX.x, stepX.y);
	currentShader->setFloat("GaussWidth", sqrtf(gConvolutionScale[num]));

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, convolutionTempColourTex, 0);

	//设置相应的拉伸矫正纹理
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, newStretchTexture[num]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sourceTex);
	renderQuad();

	currentShader->setVec2("Step", stepY.x, stepY.y);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, targetTex, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, convolutionTempColourTex);
	renderQuad();

	glPopAttrib();
}
void Renderer::irradianceMapPass() {

	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, MAP_SIZE, MAP_SIZE);

	SetCurrentShader(irradianceShader);

	// set up
	glBindFramebuffer(GL_FRAMEBUFFER, irradianceFBO);
	//一开始将模型场景渲染到一张颜色纹理中
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, irradianceTex[0], 0);

	//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);//0.0f, 0.0f, 0.0f, 0.0f
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//绑定半透明阴影贴图纹理
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "shadowTex"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, TSMTex);

	lightProjMatrix = glm::perspective(FOV, 1.0f, zNear, zFar);
	//lightProjMatrix = glm::perspective(glm::radians(camera->Zoom), 1.0f, zNear, zFar);
	lightViewMatrix = glm::lookAt(lightPos, lightTarget, vec3(0.0f, 1.0f, 0.0f));
	currentShader->setMat4("lightViewMatrix", lightViewMatrix);
	currentShader->setMat4("lightProjMatrix", lightProjMatrix);

	currentShader->setVec3("cameraPos", vec3(0.0f, 0.0f, 3.0f));
	currentShader->setFloat("mix", forwardScatteringFactor);
	currentShader->setBool("isShadowVSM", true);

	currentShader->setFloat("ZNear", zNear);
	currentShader->setFloat("ZFar", zFar);

	//设置光照相关参数
	SetShaderLight();
	projMatrix = glm::perspective(glm::radians(camera->Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
	viewMatrix = camera->GetViewMatrix();
	modelMatrix = mat4(1.0f);
	//设置三个矩阵
	UpdateShaderMatrices();

	//渲染模型
	drawModel(headMesh);

	glUseProgram(0);
	glPopAttrib();

	//---------------卷积计算
	SetCurrentShader(convolutionShader);

	modelMatrix = mat4(1.0f);
	viewMatrix = mat4(1.0f);
	projMatrix = ortho(-1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f);
	UpdateShaderMatrices();
	// shader textures
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "InputTex"), 0);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "stretchTex"), 2);

	currentShader->setBool("isBlurDiffusion", true);
	currentShader->setBool("isBlurStretch", false);


	//convolve 5 times!!!!!
	convolution(irradianceTex[0], irradianceTex[1], 0);
	convolution(irradianceTex[1], irradianceTex[2], 1);
	convolution(irradianceTex[2], irradianceTex[3], 2);
	convolution(irradianceTex[3], irradianceTex[4], 3);
	convolution(irradianceTex[4], irradianceTex[5], 4);

	// clean up
	glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0.0f, 0.0f, (float)width, (float)height);
}
//-------------------------------------------------------------------模糊部分
void Renderer::blurPass()
{
	// set up
	glBindFramebuffer(GL_FRAMEBUFFER, blurFBO);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);//0.0f, 0.0f, 0.0f, 0.0f//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);
	glViewport(0.0f, 0.0f, MAP_SIZE, MAP_SIZE);

	// shader
	SetCurrentShader(blurShader);

	// shader textures
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "stretchTex"), 2);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, stretchColourTex);

	// shader variables
	glUniform2f(glGetUniformLocation(currentShader->GetProgram(), "pixelSize"), 1.0f / MAP_SIZE, 1.0f / MAP_SIZE);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "useStretch"), useStretch);

	// matrices
	modelMatrix = mat4(1.0f);
	viewMatrix = mat4(1.0f);
	projMatrix = ortho(-1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f);
	UpdateShaderMatrices();

	// blur 1
	uvPass(nonBlurredTexture, blurredTexture[0]);
	// blur 2
	uvPass(blurredTexture[0], blurredTexture[1]);
	// blur 3
	uvPass(blurredTexture[1], blurredTexture[2]);
	// blur 4
	uvPass(blurredTexture[2], blurredTexture[3]);
	// blur 5
	uvPass(blurredTexture[3], blurredTexture[4]);

	// clean up
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);
	glEnable(GL_DEPTH_TEST);
	glViewport(0.0f, 0.0f, (float)width, (float)height);
}

//---------------------------------------------------------------------------------------------纹理空间部分
void Renderer::uvPass(GLuint &sourceTex, GLuint &targetTex)
{
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "xAxis"), true);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tempColourTex, 0);

	//------------------------设置纹理，这步很重要，因为四边形是自己手动创建的，所以一开始的纹理需要自己设置---------------
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sourceTex);
	/*quad->SetTexture(sourceTex);
	quad->Draw();*/
	renderQuad();
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "xAxis"), false);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, targetTex, 0);

	//-------------------------------------------------设置纹理，这步很重要------------------------------------------------
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tempColourTex);
	/*quad->SetTexture(tempColourTex);
	quad->Draw();*/
	renderQuad();
}
//--------------------------------------------------------------------------------主渲染部分
void Renderer::mainPass()
{
	//---------------------------------------------------------mainPass()-----------------------------------
		// shader
	SetCurrentShader(mainShader);

	// shader textures
	//glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);
	//glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "bumpTex"), 1);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "shadowTex"), 2);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "beckmannTex"), 3);

	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "blurredTex1"), 5);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "blurredTex2"), 6);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "blurredTex3"), 7);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "blurredTex4"), 8);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "blurredTex5"), 9);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "blurredTex6"), 10);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, beckmannTex);

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, nonBlurredTexture);
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, blurredTexture[0]);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, blurredTexture[1]);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, blurredTexture[2]);
	glActiveTexture(GL_TEXTURE9);
	glBindTexture(GL_TEXTURE_2D, blurredTexture[3]);
	glActiveTexture(GL_TEXTURE10);
	glBindTexture(GL_TEXTURE_2D, blurredTexture[4]);

	// shader variables
	//glUniform3fv(glGetUniformLocation(currentShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
	currentShader->setVec3("cameraPos", vec3(0.0f, 0.0f, 3.0f));//------------not---------------
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "useBlur"), useBlur);

	// shader light variables
	SetShaderLight();//---------------------------not-----------------------------

	// matrices
	//projMatrix = perspective(ZNEAR, ZFAR, (float)width / (float)height, FOV);
	//--------------------------------------------NOT------------------------------------------------
	projMatrix = glm::perspective(glm::radians(camera->Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);//----------NOT-----------
	viewMatrix = camera->GetViewMatrix();
	modelMatrix = mat4(1.0f);
	UpdateShaderMatrices();

	/*
		设置后向散射系数
	*/
	currentShader->setFloat("mix", backwardScatteringFactor);
	currentShader->setFloat("m", roughness);
	currentShader->setFloat("reflectivity", reflectivity);
	currentShader->setBool("isShadowVSM", true);

	// draw calls
	drawModel(headMesh);//绘制原来的模型
	//drawLight();
}

void Renderer::RenderFinalTSM()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);//0.0f, 0.0f, 0.0f, 0.0f
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);


	SetCurrentShader(finalShader);

	// shader textures
	//glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);
	//glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "bumpTex"), 1);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "shadowTex"), 2);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "beckmannTex"), 3);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "StretchTex"), 4);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "blurredTex1"), 5);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "blurredTex2"), 6);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "blurredTex3"), 7);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "blurredTex4"), 8);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "blurredTex5"), 9);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "blurredTex6"), 10);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "seamMask"), 11);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, TSMTex);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, beckmannTex);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, newStretchTexture[2]);//指定拉伸矫正纹理
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, irradianceTex[0]);
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, irradianceTex[1]);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, irradianceTex[2]);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, irradianceTex[3]);
	glActiveTexture(GL_TEXTURE9);
	glBindTexture(GL_TEXTURE_2D, irradianceTex[4]);
	glActiveTexture(GL_TEXTURE10);
	glBindTexture(GL_TEXTURE_2D, irradianceTex[5]);

	glActiveTexture(GL_TEXTURE11);
	glBindTexture(GL_TEXTURE_2D, seamMask);

	// shader variables
	currentShader->setVec3("cameraPos", vec3(0.0f, 0.0f, 3.0f));//------------not---------------
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "useBlur"), useBlur);


	lightProjMatrix = glm::perspective(FOV, 1.0f, zNear, zFar);
	//lightProjMatrix = glm::perspective(glm::radians(camera->Zoom), 1.0f, zNear, zFar);
	lightViewMatrix = glm::lookAt(lightPos, lightTarget, vec3(0.0f, 1.0f, 0.0f));
	currentShader->setMat4("LightView", lightViewMatrix);
	currentShader->setMat4("LightProj", lightProjMatrix);

	currentShader->setVec3("cameraPos", vec3(0.0f, 0.0f, 3.0f));
	currentShader->setFloat("mix", forwardScatteringFactor);
	currentShader->setBool("isShadowVSM", true);
	currentShader->setBool("useTranslucent", useTranslucent);
	currentShader->setBool("useTranslucentOnly", useTranslucentOnly);

	currentShader->setFloat("mix", backwardScatteringFactor);
	currentShader->setFloat("m", roughness);
	currentShader->setFloat("reflectivity", reflectivity);

	currentShader->setFloat("ZNear", zNear);
	currentShader->setFloat("ZFar", zFar);

	projMatrix = glm::perspective(glm::radians(camera->Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
	viewMatrix = camera->GetViewMatrix();
	modelMatrix = mat4(1.0f);

	UpdateShaderMatrices();
	SetShaderLight();

	drawModel(headMesh);//绘制原来的模型

	//glUseProgram(0);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);


	//Utility::SaveTextureToPfm("final.pfm",gStretchBuffer[4]->GetColorTex(), gWindowWidth, gWindowWidth);
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void Renderer::renderQuad()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}
void Renderer::SetShaderLight()
{
	currentShader->setVec3("lightPos", lightPos);
	currentShader->setVec4("lightColour", lightColour);
	currentShader->setFloat("lightRadius", lightRadius);
}
//导出渲染结果到目标png图像中
void Renderer::exportRenderingResultImage(const char * fileName)
{
	unsigned char *mpixels = new unsigned char[SCR_WIDTH * SCR_HEIGHT * 4];//WIDTH和HEIGHT为所要保存的屏幕图像的宽度与高度
	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, mpixels);
	glReadBuffer(GL_BACK);
	for (int i = 0; i < (int)SCR_WIDTH*SCR_HEIGHT * 4; i += 4)
	{
		mpixels[i] ^= mpixels[i + 2] ^= mpixels[i] ^= mpixels[i + 2];
	}
	FIBITMAP* bitmap = FreeImage_Allocate(SCR_WIDTH, SCR_HEIGHT, 32, 8, 8, 8);

	for (int y = 0; y < FreeImage_GetHeight(bitmap); y++)
	{
		BYTE *bits = FreeImage_GetScanLine(bitmap, y);
		for (int x = 0; x < FreeImage_GetWidth(bitmap); x++)
		{
			bits[0] = mpixels[(y*SCR_WIDTH + x) * 4 + 0];
			bits[1] = mpixels[(y*SCR_WIDTH + x) * 4 + 1];
			bits[2] = mpixels[(y*SCR_WIDTH + x) * 4 + 2];
			bits[3] = 255;
			bits += 4;
		}

	}
	bool bSuccess = FreeImage_Save(FIF_PNG, bitmap, fileName, PNG_DEFAULT);
	FreeImage_Unload(bitmap);
}
