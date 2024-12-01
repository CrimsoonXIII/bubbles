#pragma once
#include "myGLutil/myGLutil.hpp"
const char *aquariumVScode = R"DENOM(
out vec3 worldPos;
out vec3 viewPos;
void main()
{
    worldPos = inPos;
    viewPos = vec3(mGLuGlobal.view * vec4(inPos,1));
    gl_Position = mGLuGlobal.projection * vec4(viewPos,1);
}
)DENOM";
const char *aquariumFScode = R"DENOM(
float BeerLambertOpacity(float a, float d)
{
    return 1-exp(-a*d);
}
vec3 BeerLambertOpacity(vec3 a, float d)
{
    return vec3(
        BeerLambertOpacity(a.x, d),
        BeerLambertOpacity(a.y, d),
        BeerLambertOpacity(a.z, d)
    );
}
struct Lighting
{
    vec3 diffuse;
    vec3 specular;
};
Lighting CalcLighting(vec3 vDir,
                  vec3 lCol, vec3 lDir, vec3 lAmb,
                  vec3 sNorm, vec3 sDiff, vec3 sSpec, float sGloss)
{
    Lighting res;
    vec3 receivedLight = (vec3( max(dot(sNorm, lDir), 0.0) )) * lCol;
    res.diffuse = (receivedLight + lAmb) * sDiff * (vec3(1) - sSpec);
    res.diffuse = min(vec3(1), res.diffuse);

    vec3 R = 2. * sNorm * dot(sNorm, lDir) - lDir;
    res.specular = (pow(max(dot(vDir, R), 0.0), sGloss) * receivedLight + lAmb) * sSpec;
    res.specular = min(vec3(1), res.specular);

    return res;
}
Lighting CalcPhong(vec3 vDir,
                  vec3 lCol, vec3 lDir, vec3 lAmb,
                  vec3 sNorm, vec3 sDiff, vec3 sSpec, float sGloss)
{
    Lighting res;
    vec3 receivedLight = (lAmb + vec3( max(dot(sNorm, lDir), 0.0) )) * lCol;
    res.diffuse = receivedLight * sDiff;
    res.diffuse = min(vec3(1), res.diffuse);

    vec3 R = 2. * sNorm * dot(sNorm, lDir) - lDir;
    res.specular = (pow(max(dot(vDir, R), 0.0), sGloss)) * sSpec * lCol;
    res.specular = min(vec3(1), res.specular);

    return res;
}
vec3 LightRest(vec3 lCol, vec3 sSpec, vec3 sDiff, vec3 sDiffAlpha)
{
    return lCol * (vec3(1) - sSpec - sDiff * sDiffAlpha);
}
vec3 CalcOtherP(vec3 bPos, float bRad, vec3 P, vec3 V)
{
    vec3 cV = bPos - P;
    vec3 cProj = P + V * dot(V, cV);

    vec3 P_ = 2*cProj - P;
    
    return P_;
}
struct ColorAlpha
{
    vec3 color, alpha;
};
ColorAlpha Blend(vec3 colA, vec3 alphaA, vec3 colB, vec3 alphaB)
{
    ColorAlpha res;
    alphaA = min(vec3(1), alphaA);
    alphaB = min(vec3(1), alphaB);
    vec3 transA = vec3(1) - alphaA;
    vec3 transB = vec3(1) - alphaB;
    res.alpha = vec3(1) - transA * transB;

    vec3 premultB = colB * alphaB;
    res.color = colA * alphaA + premultB * transA;
    
    res.color.x = res.alpha.x > 0 ? res.color.x/res.alpha.x : 0;
    res.color.y = res.alpha.y > 0 ? res.color.y/res.alpha.y : 0;
    res.color.z = res.alpha.z > 0 ? res.color.z/res.alpha.z : 0;

    return res;
}

layout(location = 0) uniform float gridCellSize = 2.5;
layout(location = 1) uniform vec3 wallColor1 = vec3(0.5);
layout(location = 2) uniform vec3 wallColor2 = vec3(0.3);
layout(location = 3) uniform vec3 spec = vec3(0.0);
layout(location = 4) uniform vec3 ambient = vec3(0.005);
layout(location = 5) uniform float gloss = 4;
layout(location = 6) uniform float waterAbsorbance = 0.02f;
layout(location = 7) uniform vec3 waterCol = vec3(0.36, 0.61, 1);
layout(location = 9) uniform bool usePhong = false;
layout(location = 10) uniform bool doWaterOcclusion = true;
in vec3 worldPos;
in vec3 viewPos;

out vec4 outCol;
out vec4 outAlpha;

void main()
{
    const vec3 worldNormal = normalize(cross(dFdx(worldPos), dFdy(worldPos)));
    const vec3 viewNormal = normalize(cross(dFdx(viewPos), dFdy(viewPos)));
    ivec3 gridPos = ivec3(floor(worldPos/gridCellSize));
    int gridPosSum = gridPos.x + gridPos.y + gridPos.z;
    vec3 posInGridCell = (worldPos/gridCellSize - gridPos)*2 - vec3(1);
    vec3 GridCellWallAxisD = vec3(1) - abs(posInGridCell);
    
    vec3 tileEdgeAxisD = GridCellWallAxisD * (vec3(1) - worldNormal) + abs(worldNormal);
    float tileEdgeWallD = min(min(tileEdgeAxisD.x, tileEdgeAxisD.y), tileEdgeAxisD.z);
    
    float blendFactor = pow(tileEdgeWallD, 0.5);

    const vec3 color = blendFactor * wallColor1 + (1.-blendFactor) * wallColor2;

    const vec3 viewDir = normalize(-viewPos);

    Lighting light;
    light.diffuse = light.specular = vec3(0);

    for(uint i = 0; i < pointLightN; i++)
    {
        vec3 lPos = vec3(mGLuGlobal.view * vec4(pointLights[i].pos,1));
        vec3 lCol = pointLights[i].col * pointLights[i].intensity;
        vec3 lDir = lPos - viewPos;
        float lDistance = length(lDir);
        lDir /= lDistance;
        float lDistanceSqr = lDistance * lDistance;
        float fallof = 1/(lDistanceSqr+1);

        Lighting newLight;
        if(usePhong)
            newLight = CalcPhong(
                viewDir,
                lCol * fallof, lDir, ambient,
                viewNormal, color, spec, gloss);
        else
            newLight = CalcLighting(
                viewDir,
                lCol * fallof, lDir, ambient,
                viewNormal, color, spec, gloss);
        light.diffuse += newLight.diffuse;
        light.specular += newLight.specular;
    }
    
    vec3 waterOpacity = BeerLambertOpacity(waterAbsorbance * (vec3(1) - waterCol), length(viewPos));
    ColorAlpha blend = Blend(vec3(1), light.specular, light.diffuse, vec3(1));
    if(doWaterOcclusion)
        blend.color = (vec3(1)-waterOpacity)*blend.color;
    outCol = vec4(blend.color,1);
    outAlpha = vec4(blend.alpha, 1);
}   
)DENOM";
class Aquarium
{
    const glm::vec3 min, max;
    mGLu::Drawable box;
public:
    Aquarium(const mGLu::Window *window, glm::vec3 _min, glm::vec3 _max):
        min(_min), max(_max)
    {
        mGLu::VAO vao;
        GLuint posBinding = vao.AddAttrib(GL_FLOAT, 3, "inPos");
        box.vao = vao;
        glm::vec3 vertices[8] = 
        {
            {min.x, max.y, min.z},
            {max.x, max.y, min.z},
            {max.x, max.y, max.z},
            {min.x, max.y, max.z},
            {min.x, min.y, min.z},
            {max.x, min.y, min.z},
            {max.x, min.y, max.z},
            {min.x, min.y, max.z},
        };

        unsigned int indices[36] =
        {
            0, 1, 2,
            0, 2, 3,

            6, 5, 4,
            7, 6, 4,

            0, 3, 7,
            0, 7, 4,

            6, 2, 1,
            5, 6, 1,

            1, 0, 4,
            1, 4, 5,

            7, 3, 2,
            6, 7, 2
        };
        box.buffers.push_back(mGLu::FixedBuffer(8, vertices));
        box.indexBuffer = mGLu::FixedBuffer(36, indices);
        box.SetBinding(posBinding, 0, 0, sizeof(glm::vec3));
        box.shader = mGLu::Shader(*window, (vao.GetShaderPrefix() + aquariumVScode).c_str(), (std::string(lightBufferPrefixCode) + aquariumFScode).c_str());
    }
    void Draw()
    {
        box.DrawIndexed(36);
    }
    void ToggleUsePhong()
    {
        static bool currState = false;
        box.shader.Use();
        glUniform1i(9, currState = !currState);
    }
    void ToggleDoWaterOcclusion()
    {
        static bool currState = true;
        box.shader.Use();
        glUniform1i(10, currState = !currState);
    }
};