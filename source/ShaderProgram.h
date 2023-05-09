#include "common.h"
#include "VertexAttribute.h"
#include "Uniform.h"

class ShaderProgram {
    private:
        unsigned int programId = 0;

        unsigned int maximumElements = NUMBER_OF_TILES;
        unsigned int elementIndexBuffer;
        unsigned int numberOfElements = 0;
        std::unique_ptr<unsigned int[]> elementIndexArray;
        std::vector<VertexAttribute> vertexAttributes;
        std::vector<Uniform> uniforms;

        static GLuint LoadShader(GLenum type, const char* shaderSrc);
        void SetAttributes();
        void SetUniforms();
        static GLuint CreateProgramObject(GLuint vertexShader, GLuint fragmentShader);

    public:
        ShaderProgram(std::string vSource, std::string fSource); //, unsigned int maxElements = NUMBER_OF_TILES);
        ~ShaderProgram();
        void AddAttribute(VertexAttribute attr);
        void AddUniform(Uniform uniform);
        void DrawElements(const std::shared_ptr<unsigned int[]>& elementsArray, unsigned int buffer, int size, GLenum mode);
        void DrawArrays(int count, GLenum mode);
};