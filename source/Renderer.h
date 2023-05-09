#include "common.h"
#include "EntityManager.h"
#include "ShaderProgram.h"

class Renderer 
{
    bool _isInitialized;

    std::unique_ptr<ShaderProgram> shaders[SHADER_TYPE_COUNT] = {};
    float gridPositions[POSITIONS_LENGTH] = {};
    std::shared_ptr<float[POSITIONS_LENGTH]> renderPositions;
    std::shared_ptr<float[POSITIONS_LENGTH]> animationPositions;
    std::shared_ptr<float[ORIENTATIONS_LENGTH]> orientations;
    std::shared_ptr<float[ANGLES_LENGTH]> angles;
    std::shared_ptr<float[NUMBER_OF_TILES]> pipeTypes;
    std::shared_ptr<float[8]> corners;
    std::shared_ptr<unsigned int[6]> cornerIndexArray;
    posf origo;
    float x_tile_size;
    float y_tile_size;

    std::shared_ptr<unsigned int[NUMBER_OF_TILES]> elementIndexArrays[SHADER_TYPE_COUNT];
    unsigned int elementBuffers[SHADER_TYPE_COUNT];
    int numberOfShaderType[SHADER_TYPE_COUNT];
    GLuint programObjects[SHADER_TYPE_COUNT];
    posf movementRemaining[NUMBER_OF_TILES];

    // uniforms
    int timeLocation;
    float time;
    posf lightPosition;
    float angleRemaining;
    float angle;

    float partialRotationRemaining;

    GLuint LoadShader(GLenum type, const char* shaderSrc);
    GLuint CreateProgramObject(GLuint vertexShader, GLuint fragmentShader);
    int* GetPipesActiveIndices(bool isPipeActive[]);

    public:
        Renderer();

        void Initialize();
        void InitializeScreenPositions();
        void DrawGrid();
        void DrawShaderType(ShaderType type);
        RotationCounts HandleAngle(RotationCounts rotationCount);
        void HandleMovement(posf deltaPos, int gridIndex, bool& isMovementOn);
        void HandlePartialAngle(float& partialAngle, int& rotationStarted, float& amountRemaining);
        void UpdateGraphicsData(std::unique_ptr<EntityManager>& em); // todo: take in grid data
        void Draw();
};