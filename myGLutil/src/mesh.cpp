#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "shader.hpp"
#include "window.hpp"

#include "mesh.hpp"
static const char *vertexShaderAttribPrefix = 
R"DENOM(
const uint customAttribLocStart = 8;
layout(location=0) in vec3 inPos;
layout(location=1) in vec4 inColor;
layout(location=2) in vec3 inNormal;
layout(location=3) in vec2 inUV;
layout(location=4) in mat4 inTransform;

)DENOM";
static const char *fragmentShaderOutPrefix = 
R"DENOM(
out vec4 outColor;
out vec3 outNormal;

)DENOM";
mGLu::TransformBuffer mGLu::Mesh::defaultTransformBuffer(GL_STATIC_DRAW);
mGLu::Mesh::Mesh(GLenum meshUsage):
    bufferUsage(meshUsage)
{
    InitDefaultMeshVAO();
    InitDefaultTransformBuffer();
}
mGLu::Mesh::Mesh(const mGLu::Mesh& other):
	vertices(other.vertices),
	colors(other.colors),
	normals(other.normals),
	UVs(other.UVs),
	indices(other.indices),
    bufferUsage(other.bufferUsage)
{

}
mGLu::Mesh& mGLu::Mesh::operator=(const mGLu::Mesh& other)
{
    vertices = other.vertices;
	colors = other.colors;
	normals = other.normals;
	UVs = other.UVs;
	indices = other.indices;
    bufferUsage = other.bufferUsage;
    return *this;
}
mGLu::Mesh::Mesh(mGLu::Mesh&& other) noexcept:
	vertices(std::move(other.vertices)),
	colors(std::move(other.colors)),
	normals(std::move(other.normals)),
	UVs(std::move(other.UVs)),
	indices(std::move(other.indices)),
    posVBO(other.posVBO),
    colorVBO(other.colorVBO),
    normalVBO(other.normalVBO),
    uvVBO(other.uvVBO),
    EBO(other.EBO),
    maxIndexN(other.maxIndexN),
    currIndexN(other.currIndexN),
    maxVertexN(other.maxVertexN),
    currVertexN(other.currVertexN),
    bufferUsage(other.bufferUsage)
{
    other.posVBO = other.colorVBO = other.normalVBO = other.uvVBO = other.EBO = 0;
}
mGLu::Mesh::~Mesh()
{
    if(posVBO)
        glDeleteBuffers(1, &posVBO);
    if(colorVBO)
        glDeleteBuffers(1, &colorVBO);
    if(normalVBO)
        glDeleteBuffers(1, &normalVBO);
    if(uvVBO)
        glDeleteBuffers(1, &uvVBO);
    if(EBO)
        glDeleteBuffers(1, &EBO);
}
void mGLu::Mesh::Draw(Shader shader, const TransformBuffer& transforms, GLenum drawMode)
{
    const GLuint vao = GetVAO();

    glVertexArrayVertexBuffer(vao, posBinding, posVBO, 0, sizeof(glm::vec3));
    glVertexArrayVertexBuffer(vao, colorBinding, colorVBO, 0, sizeof(glm::vec4));
    glVertexArrayVertexBuffer(vao, normalBinding, normalVBO, 0, sizeof(glm::vec3));
    glVertexArrayVertexBuffer(vao, uvBinding, uvVBO, 0, sizeof(glm::vec2));
    glVertexArrayVertexBuffer(vao, transformBinding, transforms.transformVBO, 0, sizeof(glm::mat4));
    shader.Use();

    glBindVertexArray(vao);

    if(currIndexN)
    {
        glVertexArrayElementBuffer(vao, EBO);
        glDrawElementsInstanced(drawMode, currIndexN, GL_UNSIGNED_INT, nullptr, transforms. currTransformN);
        return;
    }
    //glDrawArrays(drawMode, 0, currVertexN);
    glDrawArraysInstanced(drawMode, 0, currVertexN, transforms.currTransformN);
    return;
}
GLuint mGLu::Mesh::GetVAO()
{
    return defaultMeshVAO;
}
mGLu::Shader mGLu::CreateMeshShader(const Window &window, const char* vsCode, const char* fsCode, const char* gsCode)
{
    std::string vsWPrefix, fsWPrefix;
    const char *passVS = nullptr, *passFS = nullptr;
    if(vsCode)
    {
        vsWPrefix += vertexShaderAttribPrefix;
        vsWPrefix += vsCode;
        passVS = vsWPrefix.data();
    }
    if(fsCode)
    {
        fsWPrefix += fragmentShaderOutPrefix;
        fsWPrefix += fsCode;
        passFS = fsWPrefix.data();
    }
    return Shader(window, passVS, passFS, gsCode);
}
void mGLu::Mesh::UpdateBuffers()
{
    currVertexN = vertices.size();
    currIndexN = indices.size();
    if(!posVBO)
    {
        glCreateBuffers(5, &posVBO);
    }

    if(colors.size() <  currVertexN)
        colors.reserve(currVertexN);
    if(normals.size() <  currVertexN)
        normals.reserve(currVertexN);
    if(UVs.size() <  currVertexN)
        UVs.reserve(currVertexN);

    if(maxVertexN < currVertexN)
    {
        glNamedBufferData(posVBO, currVertexN* sizeof(glm::vec3), vertices.data(), bufferUsage);
        glNamedBufferData(colorVBO, currVertexN * sizeof(glm::vec4), colors.data(), bufferUsage);
        glNamedBufferData(normalVBO, currVertexN * sizeof(glm::vec3), normals.data(), bufferUsage);
        glNamedBufferData(uvVBO, currVertexN * sizeof(glm::vec2), UVs.data(), bufferUsage);
        maxVertexN = currVertexN;
    }
    else
    {
        glNamedBufferSubData(posVBO, 0, currVertexN * sizeof(glm::vec3), vertices.data());
        glNamedBufferSubData(colorVBO, 0, currVertexN * sizeof(glm::vec4), colors.data());
        glNamedBufferSubData(normalVBO, 0, currVertexN * sizeof(glm::vec3), normals.data());
        glNamedBufferSubData(uvVBO, 0, currVertexN * sizeof(glm::vec2), UVs.data());
    }
    if(maxIndexN < currIndexN)
    {
        glNamedBufferData(EBO, currIndexN * sizeof(GLuint), indices.data(), bufferUsage);
        maxIndexN = currIndexN;
    }
    else
        glNamedBufferSubData(EBO, 0, currIndexN * sizeof(GLuint), indices.data());
}





GLuint mGLu::Mesh::defaultMeshVAO = 0;
void mGLu::Mesh::InitDefaultMeshVAO()
{
    if(defaultMeshVAO) return;
    GLuint vao;
	glCreateVertexArrays(1, &vao);

	glEnableVertexArrayAttrib(vao, posBinding); // POS
	glVertexArrayAttribFormat(vao, posBinding, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, posBinding, posBinding);

	glEnableVertexArrayAttrib(vao, colorBinding); // COL
	glVertexArrayAttribFormat(vao, colorBinding, 4, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, colorBinding, colorBinding);

	glEnableVertexArrayAttrib(vao, normalBinding); // NORMAL
	glVertexArrayAttribFormat(vao, normalBinding, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, normalBinding, normalBinding);

	glEnableVertexArrayAttrib(vao, uvBinding); // UV
	glVertexArrayAttribFormat(vao, uvBinding, 2, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, uvBinding, uvBinding);

	for (int i = 0; i < 4; i++) {	   // MODEL TRANSFORM MATRIX
        glEnableVertexArrayAttrib(vao, transformBinding + i);
        glVertexArrayAttribFormat(vao, transformBinding + i, 4, GL_FLOAT, GL_FALSE, i*sizeof(glm::vec4));
        glVertexArrayAttribBinding(vao, transformBinding + i, transformBinding);
    }
    glVertexArrayBindingDivisor(vao, transformBinding, 1);
	defaultMeshVAO = vao;
}
void mGLu::Mesh::InitDefaultTransformBuffer()
{
    static bool initDone = false;
    if(initDone) return;
    defaultTransformBuffer.transforms.resize(1);
    defaultTransformBuffer.transforms[0] = glm::mat4(1.f);
    defaultTransformBuffer.UpdateBuffer();
}
void mGLu::TransformBuffer::UpdateBuffer()
{
    if(!transformVBO)
        glCreateBuffers(1, &transformVBO);
    currTransformN = transforms.size();
    if(maxTransformN < currTransformN)
    {
        glNamedBufferData(transformVBO, currTransformN * sizeof(glm::mat4), transforms.data(), bufferUsage);
        maxTransformN = currTransformN;
    }
    else
        glNamedBufferSubData(transformVBO, 0, currTransformN * sizeof(glm::mat4), transforms.data());
}

void mGLu::Subdivide(mGLu::Mesh &mesh)
{
    long long lastI = mesh.indices.size();
    std::vector<long long> edgePointIndices(lastI * lastI, -1);
    std::vector<unsigned int> newIndices(lastI * 4);
    if(mesh.colors.size() < mesh.vertices.size())
        mesh.colors.resize(mesh.vertices.size(), glm::vec4(1.f));
    if(mesh.normals.size() < mesh.vertices.size())
        mesh.normals.resize(mesh.vertices.size(), glm::vec4(1.f));
    if(mesh.UVs.size() < mesh.vertices.size())
        mesh.UVs.resize(mesh.vertices.size(), glm::vec4(1.f));
    for(long long i = 0; i < lastI; i+=3)
    {
        unsigned int sI[6] = {mesh.indices[i], mesh.indices[i+1], mesh.indices[i+2]};
        
        unsigned int eI1 = sI[0], eI2 = sI[1];
        if(eI2 < eI1)
            std::swap(eI1, eI2);
        if(edgePointIndices[eI1 * lastI + eI2] == -1)
        {
            edgePointIndices[eI1 * lastI + eI2] = mesh.vertices.size();
            mesh.vertices.push_back((mesh.vertices[eI1] + mesh.vertices[eI2])*0.5f);
            mesh.colors.push_back((mesh.colors[eI1] + mesh.colors[eI2])*0.5f);
            mesh.normals.push_back((mesh.normals[eI1] + mesh.normals[eI2])*0.5f);
            mesh.normals.back()/=glm::length(mesh.normals.back());
            mesh.UVs.push_back((mesh.UVs[eI1] + mesh.UVs[eI2])*0.5f);
        }
        sI[3] = edgePointIndices[eI1 * lastI + eI2];

        eI1 = sI[1], eI2 = sI[2];
        if(eI2 < eI1)
            std::swap(eI1, eI2);
        if(edgePointIndices[eI1 * lastI + eI2] == -1)
        {
            edgePointIndices[eI1 * lastI + eI2] = mesh.vertices.size();
            mesh.vertices.push_back((mesh.vertices[eI1] + mesh.vertices[eI2])*0.5f);
            mesh.colors.push_back((mesh.colors[eI1] + mesh.colors[eI2])*0.5f);
            mesh.normals.push_back((mesh.normals[eI1] + mesh.normals[eI2])*0.5f);
            mesh.normals.back()/=glm::length(mesh.normals.back());
            mesh.UVs.push_back((mesh.UVs[eI1] + mesh.UVs[eI2])*0.5f);
        }
        sI[4] = edgePointIndices[eI1 * lastI + eI2];

        eI1 = sI[2], eI2 = sI[0];
        if(eI2 < eI1)
            std::swap(eI1, eI2);
        if(edgePointIndices[eI1 * lastI + eI2] == -1)
        {
            edgePointIndices[eI1 * lastI + eI2] = mesh.vertices.size();
            mesh.vertices.push_back((mesh.vertices[eI1] + mesh.vertices[eI2])*0.5f);
            mesh.colors.push_back((mesh.colors[eI1] + mesh.colors[eI2])*0.5f);
            mesh.normals.push_back((mesh.normals[eI1] + mesh.normals[eI2])*0.5f);
            mesh.normals.back()/=glm::length(mesh.normals.back());
            mesh.UVs.push_back((mesh.UVs[eI1] + mesh.UVs[eI2])*0.5f);
        }
        sI[5] = edgePointIndices[eI1 * lastI + eI2];

        newIndices[i*4 + 0] = sI[0];
        newIndices[i*4 + 1] = sI[3];
        newIndices[i*4 + 2] = sI[5];

        newIndices[i*4 + 3] = sI[3];
        newIndices[i*4 + 4] = sI[1];
        newIndices[i*4 + 5] = sI[4];

        newIndices[i*4 + 6] = sI[5];
        newIndices[i*4 + 7] = sI[4];
        newIndices[i*4 + 8] = sI[2];
        
        newIndices[i*4 + 9] = sI[3];
        newIndices[i*4 +10] = sI[4];
        newIndices[i*4 +11] = sI[5];
    }
    mesh.indices = std::move(newIndices);
}