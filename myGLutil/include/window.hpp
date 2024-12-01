#pragma once
#include <chrono>
#include <unordered_map>
#include <stdexcept>
#include <cstdio>

#include "shader.hpp"

namespace mGLu
{
	class Camera;
	class Window
	{
	private:
		float ratio;
		glm::vec2 size;
		glm::vec2 mousePos = {0.f,0.f}, prevMousePos = {0.f, 0.f};
		glm::vec2 mouseScroll;
		std::chrono::time_point<std::chrono::high_resolution_clock> lastFrameTime, currFrameTime, startTime;
		float deltaTime, mainLoopTime;
		bool m_shouldClose = false;
		GLFWwindow* window = nullptr;
		static std::unordered_map<GLFWwindow*, glm::vec2> _mouseScroll;
		static void glfw_scroll_callback(GLFWwindow*, double, double);
		std::string shaderPrefix;
		struct _GlobalShaderVars
		{
			friend class Window;
		private:
			GLuint ubo;
			static constexpr unsigned int uboBindingPoint = 0;
			struct _GlobalShaderVarsData
			{
				friend class Window;
			private:
				glm::mat4 projection{};
				glm::mat4 view{};
				float time = 0.0f;
				float deltaTime = 0.0f;
			} data;
			void Init();
			void UpdateData();
		};
		_GlobalShaderVars sharedShaderVars;


	public:
		const unsigned int GLmaj, GLmin;
		Window(unsigned int width, unsigned int height, const char* title, bool fullscreen, unsigned int maj, unsigned int min, bool debug = true);
		Window(const Window&) = delete;
		virtual ~Window();
		void StartMainLoop();

		void UpdateSharedShaderVars();
		void UseCamera(Camera &camera);

		virtual const char* GetShaderPrefix(std::size_t *outPrefixLength) const;
		GLFWwindow* GetWindow() const { return window; }
		inline float DeltaTime() const { return deltaTime; }
		inline float GetTime() const { return mainLoopTime; }
		float GetAspectRatio() const { return ratio; }
		glm::vec2 GetSize() const { return size; }
		glm::vec2 GetMousePos() const { return mousePos; }
		glm::vec2 GetMouseMove() const { return mousePos - prevMousePos; }
		glm::vec2 GetMouseScroll() const { return mouseScroll; }
		int KeyInputState(int key) const;
		int MouseButtonState(int button) const;
	protected:
		void Close() { m_shouldClose = true; }
		virtual void Start(){};
		virtual void Update(){};
	};
}
