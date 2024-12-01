#pragma once

#include "window.hpp"
namespace mGLu
{
    class Camera
    {
    private:
        GLuint fbo = 0, colorTex = 0, depthTex = 0, normalTex = 0;
        //bool ownColor = false, ownDepth = false, ownNormal = false;
        int width, height, xOffset, yOffset;
    public:
        glm::mat4 view{1.0f}, projection{1.0f};
        Camera(int xOffset, int yOffset, int width, int height, bool useCustomFBO = false);
        //Camera(int xOffset, int yOffset, int width, int height);
        Camera(const Camera&) = delete;
        void SetSize(int width, int height);
        void SetOffset(int xOffset, int yOffset);
        glm::ivec2 GetSize() { return {width, height}; }
        float GetRatio() { return (float)width/height; }
        GLuint GetColorTexture();
        GLuint GetDepthTexture();
        GLuint GetNormalTexture();
        friend void Window::UseCamera(Camera&);
    };
}