#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <cstring>
#include <string>

#include "window.hpp"

#include "shader.hpp"
std::unordered_map<GLuint, unsigned int> mGLu::Shader::instanceCount{};

static GLuint __CreateShader(const char *vsCode, const char *fsCode, const char *gsCode)
{
	unsigned int vs = 0, fs = 0, gs = 0;
	GLuint ID = 0;
	if (vsCode)
	{
		vs = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vs, 1, &vsCode, NULL);
		glCompileShader(vs);
		GLint compileStatus, logLen;
		glGetShaderiv(vs, GL_COMPILE_STATUS, &compileStatus);
		glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &logLen);
		if (logLen > 0)
		{
			char log[logLen + 1];
			glGetShaderInfoLog(vs, logLen + 1, 0, log);
			std::fprintf(stderr, "Vertex Shader Compilation Error: %s", log);
		}
	}
	if (fsCode)
	{
		fs = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fs, 1, &fsCode, NULL);
		glCompileShader(fs);
		GLint compileStatus, logLen;
		glGetShaderiv(fs, GL_COMPILE_STATUS, &compileStatus);
		glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &logLen);
		if (logLen > 0)
		{
			char log[logLen + 1];
			glGetShaderInfoLog(vs, logLen + 1, 0, log);
			std::fprintf(stderr, "Fragment Shader Compilation Error: %s", log);
		}
	}
	if (gsCode)
	{
		gs = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(gs, 1, &gsCode, NULL);
		glCompileShader(gs);
		GLint compileStatus, logLen;
		glGetShaderiv(gs, GL_COMPILE_STATUS, &compileStatus);
		glGetShaderiv(gs, GL_INFO_LOG_LENGTH, &logLen);
		if (logLen > 0)
		{
			char log[logLen + 1];
			glGetShaderInfoLog(vs, logLen + 1, 0, log);
			std::fprintf(stderr, "Geometry Shader Compilation Error: %s", log);
		}
	}
	ID = glCreateProgram();
	if (vs)
	{
		glAttachShader(ID, vs);
	}
	if (fs)
	{
		glAttachShader(ID, fs);
	}
	if (gs)
	{
		glAttachShader(ID, gs);
	}
	glLinkProgram(ID);
	GLint linkStatus, logLen;
	glGetProgramiv(ID, GL_LINK_STATUS, &linkStatus);
	glGetProgramiv(ID, GL_INFO_LOG_LENGTH, &logLen);
	if (logLen > 0)
	{
		char log[logLen + 1];
		glGetProgramInfoLog(ID, logLen + 1, 0, log);
		std::fprintf(stderr, "Geometry Shader Linking Error: %s", log);
	}
	//puts(vsCode);
	//puts(fsCode);
	return ID;
}

mGLu::Shader::Shader()
{
	
}
mGLu::Shader::Shader(const Shader& other) :
	ID(other.ID)
{
	if (ID != 0)
	{
		++instanceCount[ID];
	}
}
mGLu::Shader& mGLu::Shader::operator=(const Shader& other)
{
	ID = other.ID;
	if (ID != 0)
	{
		++instanceCount[ID];
	}
	return *this;
}
mGLu::Shader::Shader(Shader&& other) noexcept:
	ID(other.ID)
{
	other.ID = 0;
	if (ID != 0)
	{
		++instanceCount[ID];
	}
}
mGLu::Shader::~Shader()
{
	if (!ID)
		return;
	if (--instanceCount[ID] == 0)
	{
		instanceCount.erase(ID);
		
		glDeleteProgram(ID);
	}
	
}
mGLu::Shader::Shader(const char* const vsCode, const char* const fsCode,
			const char* const gsCode)
{
	ID = __CreateShader(vsCode, fsCode, gsCode);
	if (ID != 0)
	{
		++instanceCount[ID];
	}
}
mGLu::Shader::Shader(const Window &window, const char* const vsCode, const char* const fsCode, const char* const gsCode)
{
	std::string vsWPrefix, fsWPrefix, gsWPrefix;
	const char *passVS = nullptr, *passFS = nullptr, *passGS = nullptr;
	if(vsCode)
	{
		vsWPrefix += window.GetShaderPrefix(nullptr);
		vsWPrefix += vsCode;
		passVS = vsWPrefix.data();
	}
	if(fsCode)
	{
		fsWPrefix += window.GetShaderPrefix(nullptr);
		fsWPrefix += fsCode;
		passFS = fsWPrefix.data();
	}
	if(gsCode)
	{
		gsWPrefix += window.GetShaderPrefix(nullptr);
		gsWPrefix += gsCode;
		passGS = gsWPrefix.data();
	}
	ID = __CreateShader(passVS, passFS, passGS);
	if (ID != 0)
	{
		++instanceCount[ID];
	}
}
GLuint mGLu::Shader::GetID() const
{
	return ID;
}
void mGLu::Shader::Use() const
{
	glUseProgram(ID);
}
