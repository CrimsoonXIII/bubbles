#pragma once
#include "shader.hpp"
#include "buffer.hpp"
#include "vao.hpp"
#include <vector>
typedef unsigned long long ull;
namespace mGLu
{
	unsigned long long EdgeToKey(unsigned int i1, unsigned int i2)
	{
		if(i1>i2) std::swap(i1,i2);
		return (unsigned long long)i1 | (((unsigned long long)(i2))<<32);
	}
	template<typename Integral, typename... V_T>
	void Subdivide(std::vector<Integral> &indices, std::vector<V_T>&... vertexData)
	{
		std::vector<Integral> newIndices;
		newIndices.reserve(indices.size()*4);
		unsigned int newIndex = 0;
		([&]{
			if(newIndex < vertexData.size());
				newIndex = vertexData.size();
		}(),...);
		([&]{
			vertexData.resize(newIndex);
		}(),...);
		std::unordered_map<unsigned long long, unsigned int> vertexMap;
		for(unsigned int i = 0; i < indices.size(); i+=3)
		{
			unsigned long long mapKeys[3] = 
			{
				EdgeToKey(indices[i], indices[i+1]),
				EdgeToKey(indices[i+1], indices[i+2]),
				EdgeToKey(indices[i+2], indices[i])
			};
			unsigned int newVertsI[3];

			if(vertexMap.contains(mapKeys[0]))
				newVertsI[0] = vertexMap[mapKeys[0]];
			else
			{
				newVertsI[0] = vertexMap[mapKeys[0]] = newIndex++;
				([&]{
					vertexData.push_back((vertexData[indices[i]] + vertexData[indices[i+1]])/2.f);
				}(),...);
			}

			if(vertexMap.contains(mapKeys[1]))
				newVertsI[1] = vertexMap[mapKeys[1]];
			else
			{
				newVertsI[1] = vertexMap[mapKeys[1]] = newIndex++;
				([&]{
					vertexData.push_back((vertexData[indices[i+1]] + vertexData[indices[i+2]])/2.f);
				}(),...);
			}

			if(vertexMap.contains(mapKeys[2]))
				newVertsI[2] = vertexMap[mapKeys[2]];
			else
			{
				newVertsI[2] = vertexMap[mapKeys[2]] = newIndex++;
				([&]{
					vertexData.push_back((vertexData[indices[i+2]] + vertexData[indices[i]])/2.f);
				}(),...);
			}
			newIndices.push_back(indices[i]);
			newIndices.push_back(newVertsI[0]);
			newIndices.push_back(newVertsI[2]);

			newIndices.push_back(newVertsI[0]);
			newIndices.push_back(indices[i+1]);
			newIndices.push_back(newVertsI[1]);

			newIndices.push_back(newVertsI[0]);
			newIndices.push_back(newVertsI[1]);
			newIndices.push_back(newVertsI[2]);

			newIndices.push_back(newVertsI[2]);
			newIndices.push_back(newVertsI[1]);
			newIndices.push_back(indices[i+2]);
		}
		indices = newIndices;
	}
	class Drawable
	{
		struct BufferBinding
		{
			int bufferIndex = -1; 	// index of buffer in buffers vector
			GLintptr offset; 			// offset of first attrib we bind to in buffer
			GLsizei stride; 			// distance between attribs we bind to
		};
		std::vector<BufferBinding> bindingData; // we store binding data for binding point i in bindingData[i]
		GLsizei InferVertexCount() // infers amout of vertices to render based on attrib strides and buffer sizes of defined, non empty buffers
		{
			unsigned int vertexCount;
			for(unsigned int bindingI = 0; bindingI < bindingData.size(); bindingI++)
			{
				unsigned int bufferIndex = bindingData[bindingI].bufferIndex;
				if(bufferIndex == -1 || buffers[bufferIndex].GetSize() == 0)
					continue;
				GLsizeiptr bufferSize = buffers[bufferIndex].GetSize();
				unsigned int stridesInBuffer = bufferSize / bindingData[bindingI].stride;
				if(stridesInBuffer < vertexCount) vertexCount = stridesInBuffer;
			}
			return vertexCount;
		}
		GLsizei InferIndexCount(GLenum indexType)
		{
			unsigned int indexTypeSize = sizeof(GLuint);
			switch (indexType)
			{
			case GL_UNSIGNED_INT:
				indexTypeSize =  sizeof(GLuint);
				break;
			case GL_UNSIGNED_SHORT:
				indexTypeSize =  sizeof(GLushort);
				break;
			case GL_UNSIGNED_BYTE:
				indexTypeSize =  sizeof(GLubyte);
				break;
			}
			return indexBuffer.GetSize() / indexTypeSize;
		}
		void UpdateBindingsSize()
		{
			GLuint t = vao.GetBindingCount();
			if(t != bindingData.size())
				bindingData.resize(t);
		}
		void BindToVAO()
		{
			for(GLuint bindingI = 0; bindingI < bindingData.size(); bindingI++)
			{
				vao.BindBufferToAttrib(bindingI, buffers[bindingData[bindingI].bufferIndex].GetName(), bindingData[bindingI].offset, bindingData[bindingI].stride);
			}
		}
	public:
		VAOview vao;
		std::vector<Buffer> buffers;
		Buffer indexBuffer;
		Shader shader;
		Drawable()
		{
			
		}
		Drawable(const Drawable& other): 
			bindingData(other.bindingData),
			vao(other.vao),
			buffers(other.buffers),
			indexBuffer(other.indexBuffer)
		{

		}
		bool SetBinding(GLuint bindingPoint, unsigned int bufferIndex, GLintptr offset, GLsizei stride)
		{
			UpdateBindingsSize();
			if(bindingPoint >= bindingData.size())
			{
				std::fputs("Drawable: Error: Tried to bind to set bindingData for non-existant binding point!\n", stderr);
				return false;
			}
			bindingData[bindingPoint] = {(int)bufferIndex, offset, stride};
			return true;
		}
		void Draw(GLsizei vertexCount = 0, GLsizei firstVertex = 0, GLenum draw_mode = GL_TRIANGLES) // if vertexCount is not passed it will be infered from attrib strides and sizes of buffers (which has some overhead)
		{
			BindToVAO();
			
			shader.Use();

			glBindVertexArray(vao.GetName());
			glDrawArrays(draw_mode, firstVertex, vertexCount?vertexCount : InferVertexCount());
		}
		void DrawIndexed(GLsizei indexCount = 0, GLenum draw_mode = GL_TRIANGLES, GLenum indexType = GL_UNSIGNED_INT) // if indexCount is not provided it is infered
		{
			BindToVAO();
			
			vao.BindElementBuffer(indexBuffer.GetName());
			shader.Use();

			glBindVertexArray(vao.GetName());
			glDrawElements(draw_mode, indexCount ? indexCount : InferIndexCount(indexType), indexType, nullptr);
		}
		void DrawInstanced(GLsizei instanceCount, GLsizei vertexCount = 0, GLsizei firstVertex = 0, GLenum draw_mode = GL_TRIANGLES) // if vertexCount is not passed it will be infered from attrib strides and sizes of buffers (which has some overhead)
		{
			BindToVAO();
			
			shader.Use();

			glBindVertexArray(vao.GetName());
			glDrawArraysInstanced(draw_mode, firstVertex, vertexCount?vertexCount : InferVertexCount(), instanceCount);
		}
		void DrawIndexedInstanced(GLsizei instanceCount, GLsizei indexCount = 0, GLenum draw_mode = GL_TRIANGLES, GLenum indexType = GL_UNSIGNED_INT) // if indexCount is not provided it is infered
		{
			BindToVAO();
			
			vao.BindElementBuffer(indexBuffer.GetName());
			shader.Use();

			glBindVertexArray(vao.GetName());
			glDrawElementsInstanced(draw_mode, indexCount ? indexCount : InferIndexCount(indexType), indexType, nullptr, instanceCount);
		}
	};
}