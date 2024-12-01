#pragma once
#include <vector>
namespace mGLu
{
	class TransformBuffer
	{
		friend class Mesh;
		GLenum bufferUsage;
	public:
		std::vector<glm::mat4> transforms{};
		TransformBuffer(GLenum _bufferUsage = GL_DYNAMIC_DRAW): bufferUsage(_bufferUsage) {}
		TransformBuffer(const TransformBuffer&) = delete;
		void UpdateBuffer();
	private:
		GLuint transformVBO = 0;
		std::size_t maxTransformN = 0;
		std::size_t currTransformN = 0;
	};

	class Mesh
	{
	protected:
		static TransformBuffer defaultTransformBuffer;
	public:
		GLenum bufferUsage;
		std::vector<glm::vec3> vertices{};
		std::vector<glm::vec4> colors{};
		std::vector<glm::vec3> normals{};
		std::vector<glm::vec2> UVs{};

		std::vector<GLuint> indices{};

		Mesh(GLenum meshUsage = GL_DYNAMIC_DRAW); // meshUsage - buffer usage parameter for each and every buffer of the mesh
		Mesh(const Mesh& other);
		Mesh& operator=(const Mesh& other);
		Mesh(Mesh&& other) noexcept;
		virtual ~Mesh();
		void Draw(Shader shader, const TransformBuffer& transforms = defaultTransformBuffer ,GLenum drawMode = GL_TRIANGLES);
		void UpdateBuffers();
	protected:
		virtual GLuint GetVAO();
		static const char* vertexShaderAttribPrefix;
		static const char* fragmentShaderOutputPrefix;
		GLuint 	posVBO = 0, 
				colorVBO = 0, 
				normalVBO = 0, 
				uvVBO = 0,
				EBO = 0;
	private:
		std::size_t	maxVertexN = 0,
					maxIndexN = 0;
		std::size_t currVertexN = 0,
					currIndexN = 0;
		void InitDefaultTransformBuffer();
		static const GLuint	posBinding = 0,
							colorBinding = 1,
							normalBinding = 2,
							uvBinding = 3,
							transformBinding = 4;
		static GLuint defaultMeshVAO;
		static void InitDefaultMeshVAO();
	};

	Shader CreateMeshShader(const Window &window, const char* vsCode, const char* fsCode, const char* gsCode = nullptr);
	void Subdivide(Mesh &mesh);
}