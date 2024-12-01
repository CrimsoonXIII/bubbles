#pragma once

const char *playerVSCode = R"DENOM(
layout(location = 0) uniform vec3 pos = vec3(0);
layout(location = 1) uniform float scale = 1;
out vec3 viewNormal;
out vec3 viewPos;
void main()
{
    mat3 normalMat = mat3(mGLuGlobal.view);
    viewNormal = normalize(normalMat * inPos);
    const vec3 worldPos = pos + inPos*scale;
    viewPos = vec3(mGLuGlobal.view * vec4(worldPos, 1));
    gl_Position = mGLuGlobal.projection * vec4(viewPos,1);
}
)DENOM";
const char *playerFSCode = R"DENOM(
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

layout(location = 2) uniform vec3 spec = vec3(0.6);
layout(location = 3) uniform vec3 ambient = vec3(0.005);
layout(location = 4) uniform float gloss = 32;
layout(location = 5) uniform vec3 color = vec3(1);
layout(location = 6) uniform bool usePhong = false;
layout(location = 7) uniform bool doWaterOcclusion = true;
layout(location = 8) uniform float waterAbsorbance = 0.02f;
layout(location = 9) uniform vec3 waterCol = vec3(0.36, 0.61, 1);

in vec3 viewNormal;
in vec3 viewPos;

out vec4 outCol;
out vec4 outAlpha;
void main()
{
    vec3 viewDir = normalize(-viewPos);
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
    outAlpha = vec4(blend.alpha,1);
}
)DENOM";
class PlayerModel
{
    mGLu::Drawable model;
public:
    bool doWaterOcclusion = true;
    bool usePhong = false;
    glm::vec3 pos, col;
    float scale;
    PlayerModel(const mGLu::Window *window, float _scale, glm::vec3 _pos, glm::vec3 _col, unsigned int subdiv):
        pos(_pos),
        col(_col),
        scale(_scale)
    {
        mGLu::VAO vao;
        GLuint posBinding = vao.AddAttrib(GL_FLOAT, 3, "inPos");

        model.vao = vao;

        std::vector<glm::vec3> vertices;
        std::vector<GLuint> indices;
        GenerateSphere(vertices, indices, subdiv);
        model.buffers.push_back(mGLu::FixedBuffer(vertices.size(), vertices.data()));
        model.indexBuffer = mGLu::FixedBuffer(indices.size(), indices.data());

        model.SetBinding(posBinding, 0, 0, sizeof(glm::vec3));

        model.shader = mGLu::Shader(*window, (vao.GetShaderPrefix() + playerVSCode).c_str(), (std::string(lightBufferPrefixCode) + playerFSCode).c_str());
        model.shader.Use();
        printf("%f\n", scale);
        
    }
    void Draw()
    {
        
        model.shader.Use();
        glUniform1f(1, scale);
        glUniform3f(5, col.x, col.y, col.z);
        glUniform3f(0, pos.x, pos.y, pos.z);
        glUniform1i(6, usePhong);
        glUniform1i(7, doWaterOcclusion);
        model.DrawIndexed(model.indexBuffer.GetSize()/sizeof(GLuint));
    }
};