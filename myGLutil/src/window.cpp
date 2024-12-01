#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <cstring>
#include <stdexcept>

#include "shader.hpp"
#include "camera.hpp"

#include "window.hpp"
static const char* __DefaultWindowShaderPrefix = R"DENOM(
#version XX0
struct _mGLuGlobal{
		mat4 projection;
		mat4 view;
		float time;
		float deltaTime;
	};
layout(std140, binding = 0) uniform sharedShaderVars {
	_mGLuGlobal mGLuGlobal;
};

)DENOM";
static std::size_t __DefaultWindowShaderPrefixLength = sizeof(__DefaultWindowShaderPrefixLength)-1;
std::unordered_map<GLFWwindow*, glm::vec2> mGLu::Window::_mouseScroll{};

static void glfw_error_callback(int error, const char* description)
{
	std::fprintf(stderr, "GLFW error: %s\n", description);
}
static void GLAPIENTRY gl_error_callback(
		GLenum source,
		GLenum type,
		GLuint id,
		GLenum severity,
		GLsizei length,
		const GLchar* message,
		const void* userParam)
{
	if(type == GL_DEBUG_TYPE_ERROR)
		fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
			(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
			type, severity, message);
}
void mGLu::Window::glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	Window::_mouseScroll[window] += glm::vec2((float)xoffset, (float)yoffset);
}
struct GLFW_init_handler
{
	static bool initDone;
	GLFW_init_handler()
	{
		if(initDone) return;
		glfwSetErrorCallback(glfw_error_callback);
		if (!glfwInit())
			throw std::runtime_error("GLFW Initialization failed!");
		initDone = true;
	}
	~GLFW_init_handler()
	{
		if(initDone)
			glfwTerminate();
	}
};
bool GLFW_init_handler::initDone = false;

mGLu::Window::Window(unsigned int _width, unsigned int _height, const char* title, bool fullscreen, unsigned int maj, unsigned int min, bool debug) : 
	size({(float)_width, (float)_height}),
	ratio((float)_width/_height),
	GLmaj(maj),
	GLmin(min),
	shaderPrefix(__DefaultWindowShaderPrefix)
{
	static GLFW_init_handler __glfwInitGlobal;
	shaderPrefix[10] = '0' + maj;
	shaderPrefix[11] = '0' + min;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, maj);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, min);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, debug ? GLFW_TRUE : GLFW_FALSE);
	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);
	glfwWindowHint(GLFW_SAMPLES, 8);
	window = glfwCreateWindow(_width, _height, title, fullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		throw std::runtime_error("GLFW window creation failed!");
	}

	glfwMakeContextCurrent(window);

	glewInit();

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(gl_error_callback, nullptr);

	glfwSwapInterval(1);

	_mouseScroll[window] = {0,0};

}
mGLu::Window::~Window()
{
	_mouseScroll.erase(window);
}
void mGLu::Window::StartMainLoop()
{

	glfwSetScrollCallback(window, glfw_scroll_callback);
	sharedShaderVars.Init();

	Start();
	startTime = std::chrono::high_resolution_clock::now();
	lastFrameTime = std::chrono::high_resolution_clock::now();
	while (!glfwWindowShouldClose(window) && !m_shouldClose)
	{
		currFrameTime = std::chrono::high_resolution_clock::now();
		deltaTime = std::chrono::duration<float>(currFrameTime - lastFrameTime).count();
		mainLoopTime = std::chrono::duration<float>(currFrameTime - startTime).count();
		lastFrameTime = currFrameTime;

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		size = {(float)width, (float)height};
		ratio = (float)width / height;

		double mousePosX = 0.0, mousePosY = 0.0;
		glfwGetCursorPos(window, &mousePosX, &mousePosY);
		prevMousePos = mousePos;
		mousePos = {(float)mousePosX/width*2-1, -((float)mousePosY/height*2-1)};
		mouseScroll = _mouseScroll[window];
		_mouseScroll[window] = {0,0};
		Update();
		glfwSwapBuffers(window);
		glfwPollEvents();
		
	}
}
void mGLu::Window::UpdateSharedShaderVars()
{
	sharedShaderVars.data.deltaTime = DeltaTime();
	sharedShaderVars.data.time = GetTime();
	sharedShaderVars.UpdateData();
}
void mGLu::Window::UseCamera(mGLu::Camera &camera)
{
	glBindFramebuffer(GL_FRAMEBUFFER, camera.fbo);
    glViewport(camera.xOffset, camera.yOffset, camera.width, camera.height);
	glScissor(camera.xOffset, camera.yOffset, camera.width, camera.height);
    sharedShaderVars.data.projection = camera.projection;
    sharedShaderVars.data.view = camera.view;
}

int mGLu::Window::KeyInputState(int key) const
{
	return glfwGetKey(window, key);
}
int mGLu::Window::MouseButtonState(int button) const
{
	return glfwGetMouseButton(window, button);
}


void mGLu::Window::_GlobalShaderVars::Init()
{
	glCreateBuffers(1, &ubo);
	glNamedBufferStorage(ubo, sizeof(_GlobalShaderVarsData), nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferBase(GL_UNIFORM_BUFFER, uboBindingPoint, ubo);
}
void mGLu::Window::_GlobalShaderVars::UpdateData()
{
	glNamedBufferSubData(ubo, 0, sizeof(_GlobalShaderVarsData), &data);
}
const char* mGLu::Window::GetShaderPrefix(std::size_t *shaderPrefixLength) const
{
	if(shaderPrefixLength)
		*shaderPrefixLength = __DefaultWindowShaderPrefixLength;
	return shaderPrefix.data();
}