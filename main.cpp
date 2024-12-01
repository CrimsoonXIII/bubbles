#define GLM_ENABLE_EXPERIMENTAL
#define _USE_MATH_CONSTANTS
#include <myGLutil.hpp>
#include <cmath>
#include <unordered_map>
#include <glm/gtx/transform.hpp>

const char *lightBufferPrefixCode = R"DENOM(
struct PointLight
{
    vec3 col;
    vec3 pos;
    float intensity;
};
layout (std430, binding = 1) buffer LIGHTS
{
    uint pointLightN;
    PointLight pointLights[];
}; 
)DENOM";

#include "aquarium.hpp"
#include "ballHandler.hpp"
#include "playerModel.hpp"


struct __Light_Buffer_Data
{
private:
    static constexpr GLuint __maxLightN = 20;
public:
    alignas(4) GLuint lightN = 0;
    struct 
    {
        alignas(16)glm::vec3 col;
        alignas(16)glm::vec3 pos;
        alignas(4)float intensity;
    } lights[__maxLightN];
} lightBufferData;

class MainWindow : public mGLu::Window
{
    friend class BallHandler;
    mGLu::FixedBuffer lightsBuffer;
    const unsigned int lightCount = 4;
    
    const glm::vec3 aquariumMin = glm::vec3(-25.f, -15.f, -37.5f), aquariumMax = glm::vec3(25.f,15.f, 37.5f);

    const float ballsPerSquareUnit_Second_Level = 0.01f;

    const float ballSpawnSpeedMargin = 0.2f;

    const float finishColllisionRadius = 5.0f;

    unsigned int levelCounter = 1;
    
    const unsigned long long initPointBounty = 10000;
    unsigned long long currPointBounty = initPointBounty;
    float pointsPerLevelMultiplier = 1.5f;
    unsigned long long pointsCounter = 0;
    float levelStartTime;

    float FOV = (float)M_PI*0.5f;
    const float FOV_increment = (float)M_PI/18;

    float ballSpawnSpeed = (aquariumMax.x - aquariumMin.x)*(aquariumMax.y - aquariumMin.y)*ballsPerSquareUnit_Second_Level;

    float minBallDelay = 1.f/ballSpawnSpeed * (1.f - ballSpawnSpeedMargin);
    float maxBallDelay = 1.f/ballSpawnSpeed * (1.f + ballSpawnSpeedMargin);

    

    const glm::vec3 startPlayerPos = {0.f,0.f,0.f};
    glm::vec3 playerPos = startPlayerPos;
    glm::vec2 playerRot = {0.f,0.f};
    float playerRadius = 0.5f;
    mGLu::Camera mainCamera, secondaryCamera;

    const float moveSpeed = 10.f, rotSpeed = 3.14f, mouseSensi = 1.f;
    
    bool playerLightOn = true;

    bool mainLightOn = false;
    
    bool useSecondaryCamera = false;

    BallHandler ballHandler;
    Aquarium aquarium;
    PlayerModel playerModel;

    void ProcessInputs();

    void UpdateCameraMatrix();

    bool CheckPlayerWinLevel();
    void HandlePlayerWinLevel();

    bool CheckPlayerDeath();
    void HandlePlayerDeath();

    void UpdateLights();

    void Start()
    {
        if (glfwRawMouseMotionSupported())
        {
            glfwSetInputMode(GetWindow(), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
            glfwSetInputMode(GetWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }    
        glEnable(GL_CULL_FACE);
        glEnable(GL_MULTISAMPLE);
        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);

        
        lightBufferData.lightN = lightCount;
        lightBufferData.lights[0] = {{1.f,0.f,0.f}, {-12.5f, 12.5f,-35.0f}, 600.f};
        lightBufferData.lights[1] = {{0.f,1.f,0.f}, { 12.5f, 12.5f, 35.0f}, 600.f};
        lightBufferData.lights[2] = {{1.f,1.f,1.f}, {  0.5f, 12.5f,  0.0f}, 600.f};
        lightBufferData.lights[3] = {{1.f,0.f,1.f}, playerPos, 100.f};
        
        lightsBuffer = mGLu::FixedBuffer(1, &lightBufferData,  GL_DYNAMIC_STORAGE_BIT);
        lightsBuffer.BindToSSBO(1);
        
        glClearColor(0.5f, 0.5f, 0.5f, 1.f);
        levelStartTime = GetTime();

        glm::vec2 winSize = this->GetSize();
        mainCamera.projection = glm::perspective(FOV, winSize.x/winSize.y, 0.1f, 1000.f);
        secondaryCamera.view = glm::lookAt(glm::vec3(20.f, 30.f, 0.f), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
    }
    void Update()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        if(useSecondaryCamera)
            UseCamera(secondaryCamera);
        else
            UseCamera(mainCamera);

        
        aquarium.Draw();

        playerModel.Draw();

        ballHandler.Update(playerPos);
        ballHandler.Draw();

        ProcessInputs();

        if(CheckPlayerWinLevel())
            HandlePlayerWinLevel();

        if(CheckPlayerDeath())
            HandlePlayerDeath();
        
        
        UpdateLights();
        UpdateCameraMatrix();
        UpdateSharedShaderVars();

    }
public:
    MainWindow(unsigned int width, unsigned int height, bool fullscreen, unsigned int seed):
        Window(width, height, "title", fullscreen, 4, 3),
        mainCamera(0, 0, width, height),
        secondaryCamera(0, 0, width, height),
        ballHandler(this, seed, aquariumMin, aquariumMax, minBallDelay, maxBallDelay, 0.3f, 1.f, 2000, 4),
        aquarium(this, aquariumMin, aquariumMax),
        playerModel(this, playerRadius, playerPos, glm::vec3(0.f), 4)
    {

    }
};


//  === MAIN === 


int main()
{
    MainWindow window(2000,900,false, time(nullptr));
    window.StartMainLoop();
    return 0;
}









//  === DEFINITIONS ===

void MainWindow::ProcessInputs()
{
    if(GetMouseMove() != glm::vec2(0))
    {
        playerRot.x += GetMouseMove().y * rotSpeed * mouseSensi;
        playerRot.y -= GetMouseMove().x * rotSpeed * mouseSensi;
        if(playerRot.x > 3.1415f/2) playerRot.x = 3.1415f/2;
        if(playerRot.x < -3.1415f/2) playerRot.x = -3.1415f/2;
    }
    glm::mat4 rotMat = glm::rotate(playerRot.y, glm::vec3(0.f,1.f,0.f)) * glm::rotate(playerRot.x, glm::vec3(1.f,0.f,0.f));
    if(useSecondaryCamera)
        rotMat = glm::rotate((float)M_PI*0.5f, glm::vec3(0.f,1.f,0.f));
    glm::vec3 rightVec = rotMat * glm::vec4(1.f, 0.f, 0.f, 1.f);
    glm::vec3 fwdVec = rotMat * glm::vec4(0.f, 0.f, -1.f, 1.f), upVec = glm::vec4(0.f, 1.f, 0.f, 1.f);
    if(KeyInputState(GLFW_KEY_W))
        playerPos+=fwdVec*moveSpeed*DeltaTime();
    if(KeyInputState(GLFW_KEY_S))
        playerPos-=fwdVec*moveSpeed*DeltaTime();
    if(KeyInputState(GLFW_KEY_D))
        playerPos+=rightVec*moveSpeed*DeltaTime();
    if(KeyInputState(GLFW_KEY_A))
        playerPos-=rightVec*moveSpeed*DeltaTime();
    if(KeyInputState(GLFW_KEY_SPACE))
        playerPos+=upVec*moveSpeed*DeltaTime();
    if(KeyInputState(GLFW_KEY_LEFT_SHIFT))
        playerPos-=upVec*moveSpeed*DeltaTime();

    playerModel.pos = playerPos;

    static bool prevFState = false;
    bool currFState = KeyInputState(GLFW_KEY_F);
    if( currFState && !prevFState)
        playerLightOn = !playerLightOn;
    prevFState = currFState;

    static bool prevLState = false;
    bool currLState = KeyInputState(GLFW_KEY_L);
    if( currLState && !prevLState)
        mainLightOn = !mainLightOn;
    prevLState = currLState;

    static bool prevTState = false;
    bool currTState = KeyInputState(GLFW_KEY_T);
    if( currTState && !prevTState)
        ballHandler.ToggleTransparentBalls();
    prevTState = currTState;

    static bool prevPState = false;
    bool currPState = KeyInputState(GLFW_KEY_P);
    if( currPState && !prevPState)
    {
        ballHandler.ToggleUsePhong();
        aquarium.ToggleUsePhong();
        playerModel.usePhong = !playerModel.usePhong;
    }
    prevPState = currPState;

    static bool prevOState = false;
    bool currOState = KeyInputState(GLFW_KEY_O);
    if( currOState && !prevOState)
    {
        ballHandler.ToggleDoWaterOcclusion();
        aquarium.ToggleDoWaterOcclusion();
        playerModel.doWaterOcclusion = !playerModel.doWaterOcclusion;
    }
    prevOState = currOState;

    static bool prevPlusState = false;
    bool currPlusState = KeyInputState(GLFW_KEY_EQUAL) || KeyInputState(GLFW_KEY_KP_ADD);
    if( currPlusState && !prevPlusState)
    {
        FOV -= FOV_increment;
        if(FOV < FOV_increment*3) FOV = FOV_increment*3;
    }
    prevPlusState = currPlusState;

    static bool prevMinusState = false;
    bool currMinusState = KeyInputState(GLFW_KEY_MINUS) || KeyInputState(GLFW_KEY_KP_SUBTRACT);
    if( currMinusState && !prevMinusState)
    {
        FOV += FOV_increment;
        if(FOV > FOV_increment*16) FOV = FOV_increment*16;
    }
    prevMinusState = currMinusState;

    static bool prevTabState = false;
    bool currTabState = KeyInputState(GLFW_KEY_TAB);
    if( currTabState && !prevTabState)
        useSecondaryCamera = !useSecondaryCamera;
    prevTabState = currTabState;

    for(int i = 0; i < 3; i++)
        playerPos[i] = std::min(std::max(playerPos[i], aquariumMin[i] + playerRadius), aquariumMax[i] - playerRadius);
}
void MainWindow::UpdateCameraMatrix()
{
    mainCamera.view = glm::rotate(playerRot.x, glm::vec3(1.f,0.f,0.f));
    mainCamera.view = glm::rotate(playerRot.y, glm::vec3(0.f,1.f,0.f)) * mainCamera.view;
    mainCamera.view = glm::translate(playerPos) * mainCamera.view;
    mainCamera.view = glm::inverse(mainCamera.view);

    glm::vec2 winSize = this->GetSize();
    mainCamera.SetSize(winSize.x, winSize.y);


    mainCamera.projection = glm::perspective(FOV, winSize.x/winSize.y, 0.1f, 1000.f);
    secondaryCamera.projection = glm::perspective(FOV, winSize.x/winSize.y, 0.1f, 1000.f);
}

bool MainWindow::CheckPlayerWinLevel()
{
    unsigned int finishLightIndex = levelCounter % 2;

    return glm::length(playerPos - lightBufferData.lights[finishLightIndex].pos) < finishColllisionRadius + playerRadius;
}

void MainWindow::HandlePlayerWinLevel()
{
    std::swap(lightBufferData.lights[0].col, lightBufferData.lights[1].col);

    float levelEndTime = GetTime();
    float completeTime = levelEndTime - levelStartTime;
    unsigned long long pointsWorth = (float)currPointBounty / completeTime;
    currPointBounty *= pointsPerLevelMultiplier;
    printf("Level %u completed in %f seconds for %llu points!\n", levelCounter, completeTime, pointsWorth);
    pointsCounter += pointsWorth;
    levelStartTime = levelEndTime;
    levelCounter++;
    ballHandler.minSpawnTime = minBallDelay;
    ballHandler.maxSpawnTime = maxBallDelay;
}
bool MainWindow::CheckPlayerDeath()
{
    for(auto instance : ballHandler.instanceData)
    {
        if(glm::length(playerPos - instance.pos) < playerRadius + instance.scale)
            return true;
    }
    return false;
}
void MainWindow::HandlePlayerDeath()
{
    printf("You died! Score: %llu\n\n", pointsCounter);
    pointsCounter = 0;
    if(levelCounter % 2 == 2)
        std::swap(lightBufferData.lights[0].col, lightBufferData.lights[1].col);
    levelCounter = 1;
    currPointBounty = initPointBounty;
    playerPos = startPlayerPos;
    ballHandler.instanceData.clear();

}
void MainWindow::UpdateLights()
{
    lightBufferData.lights[lightCount - 1].pos = playerPos;
    lightBufferData.lightN = playerLightOn ? lightCount : lightCount - 1;
    
    lightBufferData.lights[2].intensity = 600 * mainLightOn;

    lightsBuffer.SetData(0,1, &lightBufferData);
}