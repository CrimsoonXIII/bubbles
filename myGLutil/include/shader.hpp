#pragma once
#include <cstdio>
#include <unordered_map>
namespace mGLu
{
	class Window;
	class Shader
	{
		friend class Window;
	private:
		static std::unordered_map<GLuint, unsigned int> instanceCount;
		GLuint ID = 0;
		Shader(const char* const vsCode, const char* const fsCode, const char* const gsCode = nullptr);
		
		static const char *globalVarsShaderPrefix;
		static const std::size_t globalVarsShaderPrefixLength;

		static const char *vertexShaderAttribPrefix;
		static const std::size_t vertexShaderAttribPrefixLength;

		static const char *fragmentShaderOutPrefix;
		static const std::size_t fragmentShaderOutPrefixLength;

	public:
		Shader();
		Shader(const Window &window, const char* const vsCode, const char* const fsCode, const char* const gsCode = nullptr);
		Shader(const Shader& other);
		Shader& operator=(const Shader& other);
		Shader(Shader&& other) noexcept;
		~Shader();

		GLuint GetID() const;
		void Use() const;
	};
}