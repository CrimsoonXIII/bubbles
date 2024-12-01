#pragma once
namespace mGLu
{
    /*typedef char *const typeID_t;
    template<typename T> 
    constexpr char const* typeID()
    {
        static const char t = 0;
        return &t;
    }*/

    constexpr const char* GetGLSLtype(unsigned int size, GLenum type)
    {
        switch(type)
        {
        case GL_FIXED:
        case GL_FLOAT:
            switch(size)
            {
            case 1:
                return "float";
            case 2:
                return "vec2";
            case 3:
                return "vec3";
            case 4:
                return "vec4";
            default:
                return nullptr;
            }
        case GL_DOUBLE:
            switch(size)
            {
            case 1:
                return "double";
            case 2:
                return "dvec2";
            case 3:
                return "dvec3";
            case 4:
                return "dvec4";
            default:
                return nullptr;
            }
        case GL_INT:
        case GL_SHORT:
        case GL_BYTE:
            switch(size)
            {
            case 1:
                return "int";
            case 2:
                return "ivec2";
            case 3:
                return "ivec3";
            case 4:
                return "ivec4";
            default:
                return nullptr;
            }
        case GL_UNSIGNED_INT:
        case GL_UNSIGNED_SHORT:
        case GL_UNSIGNED_BYTE:
            switch(size)
            {
            case 1:
                return "uint";
            case 2:
                return "uvec2";
            case 3:
                return "uvec3";
            case 4:
                return "uvec4";
            default:
                return nullptr;
            }
        
        default:
            return nullptr;
        }
    }
}
