#pragma once
#include "random.hpp"
#include <algorithm>
#include <functional>
const char *ballVScode = R"DENOM(
vec3 CalcOtherP(vec3 bPos, float bRad, vec3 P, vec3 V)
{
    vec3 cV = bPos - P;
    vec3 cProj = P + V * dot(V, cV);

    vec3 P_ = 2*cProj - P;
    
    return P_;
}
out vec3 viewNormal;
out vec3 viewNormalB;
out vec3 viewPos;
out vec3 viewPosB;
out vec3 ballCol;
void main()
{

    mat3 normalMat = transpose(inverse(mat3(mGLuGlobal.view)));
    viewNormal = normalize(normalMat*inPos);

    viewPos = vec3(mGLuGlobal.view * vec4(inPos*instanceScale + instancePos, 1));

    gl_Position = mGLuGlobal.projection * vec4(viewPos,1);
    ballCol = instanceCol;

    const vec3 camDir = normalize(viewPos);
    const vec3 ballViewPos = vec3(mGLuGlobal.view * vec4(instancePos,1));
    
    viewPosB = CalcOtherP(ballViewPos, instanceScale, viewPos, camDir);
    viewNormalB = -normalize(viewPosB - ballViewPos);
    
}
)DENOM";
const char *ballFScode = R"DENOM(
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
in vec3 viewNormal;
in vec3 viewNormalB;
in vec3 viewPos;
in vec3 viewPosB;
in vec3 ballCol;

out vec4 outCol;
out vec4 outAlpha;

layout(location = 0) uniform vec3 ballSpec = vec3(0.6);
layout(location = 1) uniform vec3 ambient = vec3(0.005);
layout(location = 2) uniform float ballAbsorbance = 0.2f;
layout(location = 3) uniform float ballGloss = 32;
layout(location = 4) uniform float transparentBallDiffuseAlpha = 0.1f;

layout(location = 5) uniform vec3 waterCol = vec3(0.36, 0.61, 1);
layout(location = 6) uniform vec3 waterDiffAlpha = vec3(0);
layout(location = 7) uniform vec3 waterSpec = vec3(0.7);
layout(location = 9) uniform float waterAbsorbance = 0.02f;
layout(location = 10) uniform float waterGloss = 64;

layout(location = 11) uniform bool transparentBalls = true;
layout(location = 12) uniform bool usePhong = false;
layout(location = 13) uniform bool doWaterOcclusion = true;
void main()
{   
    outCol = vec4(0);
    outAlpha = vec4(1);

    vec3 viewDir = normalize(-viewPos);
    
    vec3 diffuseAlpha = vec3(1);
    if(transparentBalls)
        diffuseAlpha = vec3(transparentBallDiffuseAlpha);
    Lighting lightA, lightB;
    lightA.diffuse = lightA.specular = lightB.diffuse = lightB.specular = vec3(0);

    for(int i = 0; i < pointLightN; i++)
    {
    
        vec3 lPos = vec3(mGLuGlobal.view * vec4(pointLights[i].pos,1));
        vec3 lCol = pointLights[i].col * pointLights[i].intensity;

        vec3 lDirA = lPos - viewPos;
        float lDistanceA = length(lDirA);
        lDirA /= lDistanceA;
        float lDistanceSqrA = lDistanceA * lDistanceA;
        float fallofA = 1./(lDistanceSqrA+1);

        vec3 lightRestB = LightRest(lCol, ballSpec, ballCol, diffuseAlpha);

        vec3 lDirB = lPos - viewPosB;
        float lDistanceB = length(lDirB);
        lDirB /= lDistanceB;
        float lDistanceSqrB = lDistanceB * lDistanceB;
        float fallofB = 1./(lDistanceSqrB+1);

        Lighting newLightA, newLightB;

        if(usePhong)
        {
            newLightA = CalcPhong(
                viewDir,
                lCol*fallofA, lDirA, ambient,
                viewNormal, ballCol, ballSpec, ballGloss);

            newLightB = CalcPhong(
                viewDir,
                lightRestB*fallofB, lDirB, ambient,
                viewNormalB, waterCol, waterSpec, waterGloss);
        }
        else
        {
            newLightA = CalcLighting(
                viewDir,
                lCol*fallofA, lDirA, ambient,
                viewNormal, ballCol, ballSpec, ballGloss);

            newLightB = CalcLighting(
                viewDir,
                lightRestB*fallofB, lDirB, ambient,
                viewNormalB, waterCol, waterSpec, waterGloss);
        }
        
        lightA.diffuse += newLightA.diffuse;
        lightA.specular += newLightA.specular;


        lightB.diffuse += newLightB.diffuse;
        lightB.specular += newLightB.specular;
    }
    

    ColorAlpha blendA = Blend(vec3(1), lightA.specular, lightA.diffuse, diffuseAlpha);

    float rayBallXd = length(viewPos - viewPosB);
    vec3 ballABSegmentOpacity = BeerLambertOpacity(ballAbsorbance * (vec3(1) - ballCol), rayBallXd);

    ColorAlpha blendB = Blend(vec3(1), lightB.specular, lightB.diffuse, waterDiffAlpha);

    ColorAlpha blendAB = Blend(blendA.color, blendA.alpha, blendB.color, blendB. alpha);
    
    vec3 waterOpacity = BeerLambertOpacity(waterAbsorbance * (vec3(1) - waterCol), length(viewPos));

    ColorAlpha finalBlend;
    if(doWaterOcclusion)
    {
        finalBlend.color = (vec3(1)-waterOpacity)*blendAB.color;
        finalBlend.alpha = blendAB.alpha;
    }
    else
        finalBlend = blendAB;
    
    outCol = vec4(finalBlend.color,1);
    outAlpha = vec4(finalBlend.alpha,1);
    
}

)DENOM";

void GenerateSphere(std::vector<glm::vec3> &posOut, std::vector<unsigned int> &indexOut, unsigned int subdivision = 2)
{
    const float f = (1 + 2.236067977f) / 2;
    posOut = {
        
    {-1, f, 0}, 
    {1, f, 0}, 
    {-1, -f, 0}, 
    {1, -f, 0}, 
    {0, -1, f}, 
    {0, 1, f}, 
    {0, -1, -f}, 
    {0, 1, -f}, 
    {f, 0, -1}, 
    {f, 0, 1}, 
    {-f, 0, -1}, 
    {-f, 0, 1}
    };
    indexOut = {
        0, 11, 5, 0, 5, 1, 0, 1, 7, 0, 7, 10, 0, 10, 11, 
    11, 10, 2, 5, 11, 4, 1, 5, 9, 7, 1, 8, 10, 7, 6, 
    3, 9, 4, 3, 4, 2, 3, 2, 6, 3, 6, 8, 3, 8, 9, 
    9, 8, 1, 4, 9, 5, 2, 4, 11, 6, 2, 10, 8, 6, 7
    };
    for(unsigned int i = 0; i < subdivision; i++)
    {
        mGLu::Subdivide(indexOut, posOut);
        for(glm::vec3 &vert : posOut)
            vert = vert/glm::length(vert);
    }
}
class BallHandler
{
    mGLu::FixedBuffer instanceBuffer;
    mGLu::Drawable ball;

    const glm::vec3 minAquarium, maxAquarium;
    float minBallScale, maxBallScale;
    const unsigned int maxBallCount;

    const float ballVelocity = 2.f;
    
    struct __instanceData
    {
        glm::vec3 pos;
        float scale, initScale;
        glm::vec3 col;
        
    };
    Random rng;
    const mGLu::Window &window;
public:
    std::vector<__instanceData> instanceData;
    float minSpawnTime, maxSpawnTime;

    BallHandler(const mGLu::Window *window, unsigned int seed, 
                glm::vec3 _minAquarium, glm::vec3 _maxAquarium, float _minSpawnTime, float _maxSpawnTime, float _minBallScale, float _maxBallScale,
                unsigned int _maxBallCount, unsigned int ballSubdivision = 6):
        minAquarium(_minAquarium),
        maxAquarium(_maxAquarium),
        minSpawnTime(_minSpawnTime),
        maxSpawnTime(_maxSpawnTime),
        minBallScale(_minBallScale),
        maxBallScale(_maxBallScale),
        maxBallCount(_maxBallCount),
        rng(seed),
        window(*window)
    {
        mGLu::VAO vao;
        GLuint vertexBind = vao.AddAttrib(GL_FLOAT, 3, "inPos");
        GLuint instancePosBind = vao.AddAttrib(GL_FLOAT, 3, "instancePos", 1);
        GLuint instanceScaleBind = vao.AddAttrib(GL_FLOAT, 1, "instanceScale", 1);
        GLuint instanceColBind = vao.AddAttrib(GL_FLOAT, 3, "instanceCol", 1);
        ball.vao = vao;

        std::vector<glm::vec3> vertices;
        std::vector<GLuint> indices;
        GenerateSphere(vertices, indices, ballSubdivision);
        
        ball.buffers.push_back(mGLu::FixedBuffer(vertices.size(), vertices.data()));
        ball.SetBinding(vertexBind, 0, 0, sizeof(glm::vec3));
        
        ball.indexBuffer = mGLu::FixedBuffer(indices.size(), indices.data());
        

        instanceBuffer = mGLu::FixedBuffer(maxBallCount * sizeof(__instanceData), nullptr, GL_DYNAMIC_STORAGE_BIT);
        ball.buffers.push_back(instanceBuffer);
        ball.SetBinding(instancePosBind, 1, offsetof(__instanceData, pos), sizeof(__instanceData));
        ball.SetBinding(instanceScaleBind, 1, offsetof(__instanceData, scale), sizeof(__instanceData));
        ball.SetBinding(instanceColBind, 1, offsetof(__instanceData, col), sizeof(__instanceData));

        ball.shader = mGLu::Shader(*window, (vao.GetShaderPrefix() + ballVScode).c_str(), (std::string(lightBufferPrefixCode) + ballFScode).c_str());

    }
    void Update(glm::vec3 cameraPos)
    {
        static float nextBallSpawnTime = 0.f;
        if(nextBallSpawnTime < window.GetTime())
        {
            nextBallSpawnTime = window.GetTime() + (maxSpawnTime-minSpawnTime)*rng.random() + minSpawnTime;

            if(instanceData.size() < maxBallCount)
            {
                __instanceData ballData;

                ballData.initScale = ballData.scale = (maxBallScale - minBallScale) * rng.random() + minBallScale;
                
                for(int i = 0; i < 3; i++)
                    ballData.pos[i] = (maxAquarium[i] - minAquarium[i] - 2.f * ballData.initScale * 1.3f) * rng.random() + minAquarium[i] + ballData.initScale * 1.3f;

                ballData.pos.y = minAquarium.y - ballData.initScale;
                
                ballData.col = {rng.random(), rng.random(), rng.random()};
                ballData.col = glm::normalize(ballData.col);

                instanceData.push_back(ballData);
            }
            
        }


        for(int i = 0; i < instanceData.size(); )
        {
            if((instanceData[i].pos.y += ballVelocity * window.DeltaTime()) + instanceData[i].scale > maxAquarium.y)
            {
                instanceData[i] = instanceData.back();
                instanceData.pop_back();
            }
            else
            {
                instanceData[i].scale = instanceData[i].initScale * (1.f + 0.3f * (instanceData[i].pos.y - minAquarium.y) / (maxAquarium.y - minAquarium.y));
                ++i;
            }
        }

        std::function<bool(__instanceData, __instanceData)> instanceCompare;

        std::sort(instanceData.begin(), instanceData.end(), [cameraPos](__instanceData a, __instanceData b){
            float weightA = glm::distance(a.pos, cameraPos) - a.scale;
            float weightB = glm::distance(b.pos, cameraPos) - b.scale;
            return weightA > weightB;
        });
        instanceBuffer.SetData(0, instanceData.size(), instanceData.data());
    }
    void Draw()
    {
        ball.DrawIndexedInstanced(instanceData.size(), ball.indexBuffer.GetSize()/sizeof(GLuint), GL_TRIANGLES, GL_UNSIGNED_INT);
    }
    void ToggleTransparentBalls()
    {
        static bool currState = true;
        ball.shader.Use();
        glUniform1i(11, currState = !currState);
    }
    void ToggleUsePhong()
    {
        static bool currState = false;
        ball.shader.Use();
        glUniform1i(12, currState = !currState);
    }
    void ToggleDoWaterOcclusion()
    {
        static bool currState = true;
        ball.shader.Use();
        glUniform1i(13, currState = !currState);
    }
};