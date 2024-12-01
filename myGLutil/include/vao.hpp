#pragma once
#include <GL/glew.h>
#include <vector>
#include <cstdio>
#include <array>
#include <string>
#include "common.hpp"

namespace mGLu
{
    class VAOview
    {
        unsigned int *const refCount = nullptr;
        GLuint name = 0;
    protected:
        GLuint *const bindingCount = nullptr;
        VAOview(int):
            refCount(new unsigned int(1)),
            bindingCount(new GLuint(0))
        {
            glCreateVertexArrays(1, &name);
        }
    public:
        VAOview()
        {

        }
        VAOview(const VAOview& other):
            name(other.name),
            refCount(other.refCount),
            bindingCount(other.bindingCount)
        {

        }
        VAOview& operator=(const VAOview& other)
        {
            if(name && --*refCount == 0)
            {
                glDeleteVertexArrays(1, &name);
                delete refCount;
                delete bindingCount;
            }
            name = other.name;
            const_cast<unsigned int*&>(refCount) = other.refCount;
            const_cast<GLuint*&>(bindingCount) = other.bindingCount;
            ++*refCount;
            return *this;
        }
        virtual ~VAOview()
        {
            if(name && --*refCount == 0)
            {
                glDeleteVertexArrays(1, &name);
                delete refCount;
                delete bindingCount;
            }
        }
        void BindBufferToAttrib(GLuint bindingPoint, GLuint vbo, GLintptr offset, GLsizei stride)
        {
            glVertexArrayVertexBuffer(name, bindingPoint, vbo, offset, stride);
        }
        void BindElementBuffer(unsigned int ebo)
        {
            glVertexArrayElementBuffer(name, ebo);
        }
        GLuint GetName()
        {
            return name;
        }
        GLuint GetBindingCount()
        {
            if(!name) return 0;
            return *bindingCount;
        }
    };
    class VAO : public VAOview
    {
        std::string shaderPrefix;
        GLuint nextIndex= 0;
    public:
        VAO():
            VAOview(0)
        {

        }
        GLuint AddAttrib(GLenum type, unsigned int size, std::string shaderVarName, unsigned int divisor = 0)
        {
            glEnableVertexArrayAttrib(GetName(), nextIndex); 
            glVertexArrayAttribFormat(GetName(), nextIndex, size, type, GL_FALSE, 0);
            glVertexArrayAttribBinding(GetName(), nextIndex, *bindingCount);
            glVertexArrayBindingDivisor(GetName(), *bindingCount, divisor);
            
            static constexpr unsigned int shaderCodeSize = 255;
            char shaderCode[shaderCodeSize];
            int parseLen = std::snprintf( shaderCode, shaderCodeSize, "layout(location = %u) in %s %s;\n", nextIndex, GetGLSLtype(size, type), shaderVarName.data());
            if(parseLen >= shaderCodeSize)
                fputs("VAO error: parsed shaderCode is to big!\n", stderr);
            shaderPrefix += shaderCode;
            ++nextIndex;
            return (*bindingCount)++;
        }
        GLuint AddFloatMatAttrib(unsigned int cols, unsigned int rows, std::string shaderVarName, unsigned int divisor)
        {
            for (int i = 0; i < cols; i++) {	   // MODEL TRANSFORM MATRIX
                glEnableVertexArrayAttrib(GetName(), nextIndex + i);
                glVertexArrayAttribFormat(GetName(), nextIndex + i, rows, GL_FLOAT, GL_FALSE, i*4*sizeof(float));
                glVertexArrayAttribBinding(GetName(), nextIndex + i, *bindingCount);
            }
            glVertexArrayBindingDivisor(GetName(), *bindingCount, divisor);

            static constexpr unsigned int shaderCodeSize = 255;
            char shaderCode[255] = "layout(location=LLLL) T N;";
            int parseLen = std::snprintf( shaderCode, shaderCodeSize, "layout(location = %u) in mat%ux%u %s;\n", nextIndex, cols, rows, shaderVarName.data());
            if(parseLen >= shaderCodeSize)
                fputs("VAO error: parsed shaderCode is to big!\n", stderr);
            shaderPrefix += shaderCode;
            nextIndex += rows;
            
            return (*bindingCount)++;
        }
        GLuint AddDoubleMatAttrib(unsigned int cols, unsigned int rows, std::string shaderVarName, unsigned int divisor)
        {
            for (int i = 0; i < rows; i++) {	   // MODEL TRANSFORM MATRIX
                glEnableVertexArrayAttrib(GetName(), nextIndex + i);
                glVertexArrayAttribFormat(GetName(), nextIndex + i, cols, GL_DOUBLE, GL_FALSE, i*4*sizeof(float));
                glVertexArrayAttribBinding(GetName(), nextIndex + i, *bindingCount);
            }
            glVertexArrayBindingDivisor(GetName(), *bindingCount, divisor);

            static constexpr unsigned int shaderCodeSize = 255;
            char shaderCode[255] = "layout(location=LLLL) T N;";
            
            int parseLen = std::snprintf( shaderCode, shaderCodeSize, "layout(location = %u) in dmat%ux%u %s;\n", nextIndex, cols, rows, shaderVarName.data());
            if(parseLen >= shaderCodeSize)
                fputs("VAO error: parsed shaderCode is to big!\n", stderr);
            shaderPrefix += shaderCode;
            nextIndex += rows;
            
            return (*bindingCount)++;
        }
        const std::string& GetShaderPrefix()
        {
            return shaderPrefix;
        }
    };
}
