#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <cstdio>

#include "camera.hpp"

mGLu::Camera::Camera(int _xOffset, int _yOffset, int _width, int _height, bool useCustomFBO) : 
    xOffset(_xOffset), yOffset(_yOffset),
    width(_width), height(_height)
{
    if(useCustomFBO)
    {
        glCreateTextures(GL_TEXTURE_2D, 3,  &colorTex);
        glCreateFramebuffers(1, &fbo);
        
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glBindTexture(GL_TEXTURE_2D, colorTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0);

        glBindTexture(GL_TEXTURE_2D, normalTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0);

        glBindTexture(GL_TEXTURE_2D, depthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0);

        GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, drawBuffers);
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::fputs("Framebuffer incomplete", stderr);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}
void mGLu::Camera::SetSize(int _width, int _height)
{
    width = _width;
    height = _height;
}
void mGLu::Camera::SetOffset(int _xOffset, int _yOffset)
{
    xOffset = _xOffset;
    yOffset = _yOffset;
}
GLuint mGLu::Camera::GetColorTexture()
{
    return colorTex;
}
GLuint mGLu::Camera::GetDepthTexture()
{
    return depthTex;
}
GLuint mGLu::Camera::GetNormalTexture()
{
    return normalTex;
}