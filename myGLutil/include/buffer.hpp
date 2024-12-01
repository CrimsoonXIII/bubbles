#pragma once
#include <GL/glew.h>
#include <cstdio>
namespace mGLu
{
    class Buffer
    {
        unsigned int *refCount = nullptr;
    protected:
        GLsizeiptr *size = 0;
        GLuint name = 0;
        Buffer(GLsizeiptr bufferSize):
            size(new GLsizeiptr(bufferSize)),
            refCount(new unsigned int(1))
        {
            glCreateBuffers(1, (GLuint*)&name);
        };
    public:
        Buffer()
        {
            
        };
        Buffer(const Buffer& other):
            size(other.size),
            name(other.name),
            refCount(other.refCount)
        {
            if(name)
                ++(*refCount);
        }
        Buffer& operator=(const Buffer& other)
        {
            if(name && --(*refCount) == 0)
            {
                delete refCount;
                delete size;
                glDeleteBuffers(1, &name);
            }

            size = other.size;
            refCount = other.refCount;
            name = other.name;
            ++(*refCount);
            return *this;
        }
        virtual ~Buffer()
        {
            if(name && --(*refCount) == 0)
            {
                delete refCount;
                delete size;
                glDeleteBuffers(1, &name);
            }
        }
        inline bool IsDefined()
        {
            return name;
        }
        inline GLuint GetName()
        {
            return name;
        }
        inline GLsizeiptr GetSize()
        {
            return *size;
        }
        bool SetData(GLintptr offset, GLsizeiptr dataSize, const void* data)
        {
            if(GetSize() < offset + dataSize)
            {
                std::fputs("Buffer: Error: tried setting data where offset + dataSize > size of buffer\n", stderr);
                return false;
            }
            glNamedBufferSubData(name, offset, GetSize(), data);
            return true;
        }
        template<typename T>
        bool SetData(GLintptr elemOffset, GLsizeiptr dataElemCount, const T* data)
        {
            return SetData(sizeof(T[elemOffset]), dataElemCount * sizeof(T), (const void*)data);
        }
        void BindToSSBO(GLuint bindingIndex, GLsizeiptr _size = 0, GLintptr _offset = 0)
        {
            glBindBufferRange(GL_SHADER_STORAGE_BUFFER, bindingIndex, name, _offset, _size?_size:*size);
        }
    };
    class FixedBuffer : public Buffer
    {
    public:
        FixedBuffer(): Buffer(){}
        FixedBuffer(const Buffer& other):
            Buffer(other)
        {

        }
        FixedBuffer& operator=(const Buffer& other)
        {
            Buffer::operator=(other);
            return *this;
        }
        FixedBuffer(GLsizeiptr bufferSize, const void* data, GLbitfield flags = 0) : 
            Buffer(bufferSize)
        {
            glNamedBufferStorage(name, GetSize(), data, flags);
        }
        template<typename T>
        FixedBuffer(unsigned int elemCount, const T* data, GLbitfield flags = 0) : 
            FixedBuffer(sizeof(T[elemCount]), (const void*) data, flags)
        {

        }
    };
    class FlexBuffer : public Buffer
    {
        GLenum usage;
    public:
        FlexBuffer(GLsizeiptr bufferSize, const void* data, GLenum bufferUsage = GL_DYNAMIC_DRAW):
            Buffer(bufferSize),
            usage(bufferUsage)
        {
            glNamedBufferData(name, bufferSize, data, usage);
        }
        template<typename T>
        FlexBuffer(unsigned int elemCount, const T* data, GLbitfield flags = 0) : 
            FlexBuffer(sizeof(T[elemCount]), (const void*) data, flags)
        {

        }
        FlexBuffer(const FlexBuffer& other):
            Buffer(other)
        {
            
        }
        FlexBuffer& operator=(const FlexBuffer& other)
        {
            Buffer::operator=(other);
            usage = other.usage; 
            return *this;
        }
        void ReallocBuffer(GLsizeiptr bufferSize, const void* data, GLenum bufferUsage = 0) // if bufferUsage is not specified, earlier one is used
        {
            if(bufferUsage)
                usage = bufferUsage;
            glNamedBufferData(name, *size = bufferSize, data, usage);
        }
        template<typename T>
        void ReallocBuffer(unsigned int elemCount, const T* data, GLenum bufferUsage = 0) // if bufferUsage is not specified, earlier one is used
        {
            if(bufferUsage)
                usage = bufferUsage;
            glNamedBufferData(name, *size = sizeof(T[elemCount]), (const void*)data, usage);
        }
    };
}