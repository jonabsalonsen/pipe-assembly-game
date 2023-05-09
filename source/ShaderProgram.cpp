#include "ShaderProgram.h"

#include <sstream>

GLuint ShaderProgram::LoadShader(GLenum type, const char* shaderSrc) 
{
    GLuint shader;
    GLint compiled;

    GlCall(shader = glCreateShader(type));
    if(shader == 0)
        return 0;

    GlCall(glShaderSource(shader, 1, &shaderSrc, NULL));
    GlCall(glCompileShader(shader));
    GlCall(glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled));
    
    if(!compiled) 
    {
        GLint infoLen = 0;
        GlCall(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen));
        if(infoLen > 1)
        {
            char infoLog[infoLen];
            GlCall(glGetShaderInfoLog(shader, infoLen, NULL, infoLog));
            std::cout << std::string(infoLog, infoLog + infoLen) << "\n";
        }
        GlCall(glDeleteShader(shader));
        return 0;
    }
    return shader;
}

GLuint ShaderProgram::CreateProgramObject(GLuint vertexShader, GLuint fragmentShader) 
{
    bool error = false;

    GLuint programObject;
    GLint linked;

    GlCall(programObject = glCreateProgram());
    if(programObject == 0)
        return 0;

    GlCall(glAttachShader(programObject, vertexShader));
    GlCall(glAttachShader(programObject, fragmentShader));


    if (error) {
        GlCall(glDeleteProgram(programObject));
        return 0;
    }
    
    GlCall(glLinkProgram(programObject));
    GlCall(glGetProgramiv(programObject, GL_LINK_STATUS, &linked));
    if(!linked) 
    {
        GLint infoLen = 0;
        GlCall(glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen));
        if(infoLen > 1)
        {
            char* infoLog = static_cast<char *>(malloc(sizeof(char) * infoLen));
            GlCall(glGetProgramInfoLog(programObject, infoLen, NULL, infoLog));
            fprintf(stderr, "Error linking program:\n%s\n", infoLog);
            free(infoLog);
        }
        GlCall(glDeleteProgram(programObject));
        return 0;
    }

    return programObject;
}

void ShaderProgram::SetAttributes()
{
    for(auto& va : vertexAttributes)
    {
        GlCall(glBindBuffer(GL_ARRAY_BUFFER, va.buffer));
        GlCall(glBufferData(GL_ARRAY_BUFFER, va.size * NUMBER_OF_TILES * sizeof(float), va.data.get(), GL_STATIC_DRAW));
        GlCall(unsigned int index = glGetAttribLocation(programId, va.name));
        GlCall(glEnableVertexAttribArray(index));
        GlCall(glVertexAttribPointer(index, va.size, GL_FLOAT, GL_FALSE, 0,0));
    }
}

void ShaderProgram::SetUniforms()
{
    for (Uniform u : uniforms) {
        GlCall(int location = glGetUniformLocation(programId, u.name));
        switch (u.size) {
            case 1:
                GlCall(glUniform1f(location, u.data[0]));
                break;
            case 2:
                GlCall(glUniform2f(location, u.data[0], u.data[1]));
                break;
            case 3:
                GlCall(glUniform3f(location, u.data[0], u.data[1], u.data[2]));
                break;
            case 4:
                GlCall(glUniform4f(location, u.data[0], u.data[1], u.data[2], u.data[3]));
                break;
        }
    }
}

void ShaderProgram::DrawElements(const std::shared_ptr<unsigned int[]>& elementsArray, unsigned int buffer, int count, GLenum mode) {
    
    if (programId == 0)
        return;

    GlCall(glUseProgram(programId));

    SetAttributes();
    SetUniforms();

    GlCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer));
    GlCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, NUMBER_OF_TILES * sizeof(unsigned int), elementsArray.get(), GL_DYNAMIC_DRAW));
    GlCall(glDrawElements(mode, count, GL_UNSIGNED_INT, nullptr));
}

void ShaderProgram::DrawArrays(int count, GLenum mode) {
    
    GlCall(glUseProgram(programId));

    SetAttributes();
    SetUniforms();

    GlCall(glDrawArrays(mode, 0, count));
}

ShaderProgram::ShaderProgram(std::string vSource, std::string fSource)//, unsigned int maxElements) 
{
    GLuint vShader = LoadShader(GL_VERTEX_SHADER, vSource.c_str());
    GLuint fShader = LoadShader(GL_FRAGMENT_SHADER, fSource.c_str());

    programId = CreateProgramObject(vShader, fShader);
    vertexAttributes = std::vector<VertexAttribute>();
    uniforms = std::vector<Uniform>();
}

ShaderProgram::~ShaderProgram() 
{
    glDeleteProgram(programId);
}

void ShaderProgram::AddAttribute(VertexAttribute va) 
{
    vertexAttributes.push_back(va);
}

void ShaderProgram::AddUniform(Uniform uniform) 
{
    uniforms.push_back(uniform);
}