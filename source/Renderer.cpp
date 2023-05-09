#include "Renderer.h"

Renderer::Renderer() 
{
    x_tile_size = 2.0f * static_cast<float>(TILE_SIZE) / static_cast<float>(PIXEL_WIDTH);
    y_tile_size = 2.0f / static_cast<float>(TILES_ROWS);

    renderPositions = std::shared_ptr<float[POSITIONS_LENGTH]>(new float[POSITIONS_LENGTH]);
    
    corners = std::shared_ptr<float[8]>(new float[8] {
                        1., -1.,
                        -1., -1., 
                        -1.,  1., 
                        1.,  1.
    });
    orientations = std::shared_ptr<float[ORIENTATIONS_LENGTH]>(new float[ORIENTATIONS_LENGTH]);
    angles = std::shared_ptr<float[ANGLES_LENGTH]>(new float[ANGLES_LENGTH]);
    pipeTypes = std::shared_ptr<float[NUMBER_OF_TILES]>(new float[NUMBER_OF_TILES]);
    cornerIndexArray = std::shared_ptr<unsigned int[6]>(new unsigned int[6]{0,1,2,2,3,0});

    timeLocation = 0;
    time = 0.f;
    angleRemaining = 0.;
    angle = 0.;
    partialRotationRemaining = 0.f;
    _isInitialized = false;

    Initialize();
}

void Renderer::Initialize() 
{   
    const char vShaderStr[] =
        "precision mediump float;                                               \n"
        "attribute vec4 vPosition;                                              \n"
        "attribute float vOrientation;                                          \n"
        "attribute float vAngle;                                                \n"
        "attribute float vPipeType;                                             \n"
        "uniform vec2 u_origo;                                                  \n"
        "varying float fOrientation;                                            \n"
        "varying float fAngle;                                                  \n"
        "varying float fPipeType;                                               \n"
        "uniform vec4 u_translation;                                            \n"
        "void main()                                                            \n"
        "{                                                                      \n"
        "   fOrientation = vOrientation;                                        \n"
        "   fAngle = vAngle;                                                    \n"
        "   fPipeType = vPipeType;                                              \n"
        "   float cosTheta = cos(vAngle);                                       \n"
        "   float sinTheta = sin(vAngle);                                       \n"
        "   mat2 rotMat = mat2(cosTheta,sinTheta,-sinTheta,cosTheta);           \n"
        "   float ratio = " STR(PIXEL_WIDTH) "./" STR(PIXEL_HEIGHT) ".;         \n"
        "   vec2 origo = u_origo / vec2(1., ratio);                             \n"
        "   //vec2 origo = u_origo;                                             \n"
        "   vec2 position = vPosition.xy / vec2(1., ratio);                     \n"
        "   vec2 pos2D = rotMat * (position - origo) + origo;                   \n"
        "   pos2D *= vec2(1.,ratio);                                            \n"
        "   gl_Position = vec4(pos2D, 0., 1.);                                  \n"
        "   float tileSize = float(" STR(TILE_SIZE) ");                         \n"
        "   gl_PointSize = 2.*tileSize;                                         \n"
        "}                                                                      \n";

    const char fPipeShadowShader[] = 
        "precision mediump float;\n"
        "uniform vec2 u_resolution;                                             \n"
        "uniform vec2 u_lightPosition;                                          \n"
        "uniform float u_angle;                                                 \n"
        "varying float fOrientation;                                            \n"
        "varying float fAngle;                                                  \n"
        "varying float fPipeType;                                               \n"
        "vec2 rotate(vec2 uv, float radians) {\n"
        "    return vec2(cos(radians)*uv.x-sin(radians)*uv.y, sin(radians)*uv.x+cos(radians)*uv.y);\n"
        "}\n"
        "float softBitCrunch(float x, float bits) {\n"
        "    float w = pow(1.-fract(x*bits),.4);\n"
        "    return (1. - w + floor(x*bits))/bits;\n"
        "}\n"
        "vec4 bitCrunchV4(vec4 color, float bits) {\n"
        "    return vec4(softBitCrunch(color.r,bits),softBitCrunch(color.g,bits),softBitCrunch(color.b,bits),softBitCrunch(color.a,bits));\n"
        "}\n"
        "float createMask(vec2 uv, vec2 lightPosition, float orientation, float pipeType) {\n"
        "    float mask = 1.;\n"
        "    uv -= .5;\n"
        "    uv = rotate(uv,  orientation*3.141592/2.+fAngle);\n"
        "    if (pipeType > 0.5) {\n"
        "        mask = uv.x < -.5 || uv.y < -.5 || uv.y > .5 || uv.x > .35 ? 0. : mask;\n"
        "        mask = uv.y - sqrt(.0225-(uv.x+.5)*(uv.x+.5)) < -0.5 || uv.y - sqrt(.7225-(uv.x+.5)*(uv.x+.5)) > -0.5 ? 0. : mask;\n"
        "    } else {\n"
        "        mask = uv.y > 0.5 ? 0. : mask;\n"
        "        mask = uv.x < -.35 || uv.y < -.5 || uv.y > .5 || uv.x > .35 ? 0. : mask;\n"
        "    }\n"
        "    return mask;\n"
        "}\n"
        "void main() {\n"
        "    vec2 global_uv = (gl_FragCoord.xy)/" STR(PIXEL_HEIGHT) ".;\n"
        "    vec3 lightTransform = vec3(u_lightPosition + global_uv, 1.);\n"
        "    vec2 uv = 2.*(gl_PointCoord.xy-.25);"
        "    uv.x = softBitCrunch(uv.x,8.);\n"
        "    uv.y = softBitCrunch(uv.y,8.);\n"
        "    float shadowAlpha = 0.;\n"
        "    float factor = 0.;\n"
        "    vec2 projected = vec2(0.);\n"
        "    for (int i = 1; i < 5; i++) {\n"
        "        factor += 1.;\n"
        "        projected = uv+lightTransform.xy*vec2(-0.2,0.2)*factor;\n"
        "        shadowAlpha += clamp(createMask(projected,u_lightPosition,fOrientation,fPipeType),0.,1.)*.2;\n"
        "    }\n"
        "    shadowAlpha = clamp(shadowAlpha, 0.,1.);\n"
        "    vec4 shadowMask = vec4(0.);\n"
        "    shadowMask.a = clamp(shadowAlpha,0.,1.);\n"
        "    vec4 color = bitCrunchV4(shadowMask,8.);\n"
        "    gl_FragColor = vec4(color);\n"
        "}\n";

    const char fPipeShader[] =
        "precision mediump float;\n"
        "uniform vec2 u_resolution;                                             \n"
        "uniform vec2 u_lightPosition;                                          \n"
        "uniform float u_angle;                                                 \n"
        "varying float fOrientation;                                            \n"
        "varying float fAngle;                                                  \n"
        "varying float fPipeType;                                               \n"
        "vec2 rotate(vec2 uv, float radians) {\n"
        "    return vec2(cos(radians)*uv.x-sin(radians)*uv.y, sin(radians)*uv.x+cos(radians)*uv.y);\n"
        "}\n"
        "float softBitCrunch(float x, float bits) {\n"
        "    float w = pow(1.-fract(x*bits),.4);\n"
        "    return (1. - w + floor(x*bits))/bits;\n"
        "}\n"
        "vec4 bitCrunchV4(vec4 color, float bits) {\n"
        "    return vec4(softBitCrunch(color.r,bits),softBitCrunch(color.g,bits),softBitCrunch(color.b,bits),softBitCrunch(color.a,bits));\n"
        "}\n"
        "float lineSegment(vec2 v1, vec2 v2, vec2 uv, float thickness) {\n"
        "    vec2 d = abs(v2 - v1);\n"
        "    float l2 = d.y*d.y + d.x*d.x;\n"
        "    float t = max(0., min(1., dot(uv - v1, v2 - v1) / l2));\n"
        "    vec2 projection = v1 + t * (v2 - v1);\n"
        "    vec2 dist = (uv - projection)/thickness;\n"
        "    return dist.x + dist.y;\n"
        "}\n"
        "vec3 getNormals(float pipeType, vec2 uv, float orientation) {\n"
        "    vec3 normals = vec3(0.);\n"
        "    float l = 0.;\n"
        "    float theta = 0.;\n"
        "    vec2 v = vec2(0.);\n"
        "    if (pipeType < 0.5) {\n"
        "        vec2 point1 = vec2(0.);\n"
        "        vec2 point2 = vec2(0.);\n"
        "        uv -= .5;\n"
        "        point1.y -= .5;\n"
        "        point2.y += .5;\n"
        "        normals.x = lineSegment(point1,point2, uv,.35);\n"
        "        normals.z = sin(3.1415-acos(lineSegment(point1,point2, uv,.35)));\n"
        "        normals = uv.y > 0.5 ? vec3(0.) : normals;\n"
        "        normals = uv.x < -.35 || uv.y < -.5 || uv.y > .5 || uv.x > .35 ? vec3(0.) : normals;\n"
        "    } else {\n"
        "        float r = .5;\n"
        "        float w = .35;\n"
        "        l = (length(uv)-r)/w;\n"
        "        theta = atan(uv.y, uv.x);\n"
        "        v = l*vec2(cos(theta),sin(theta));\n"
        "        normals.xy = v;\n"
        "        normals.z = sqrt(1.-l*l);\n"
        "        uv -= .5;\n"
        "        normals = uv.x < -.5 || uv.y < -.5 || uv.y > .5 || uv.x > .35 ? vec3(0.) : normals;\n"
        "        normals = uv.y - sqrt(.0225-(uv.x+.5)*(uv.x+.5)) < -0.5 || uv.y - sqrt(.7225-(uv.x+.5)*(uv.x+.5)) > -0.5 ? vec3(0.) : normals;\n"
        "    }\n" 
        "    normals.xy = rotate(normals.xy, -orientation*3.141592/2.-fAngle);\n"
        "    return normals;\n"
        "}\n"
        "float createMask(vec2 uv, vec2 lightPosition, float orientation, float pipeType) {\n"
        "    float mask = 1.;\n"
        "    uv -= .5;\n"
        "    uv = rotate(uv,  orientation*3.141592/2.+fAngle);\n"
        "    if (pipeType > 0.5) {\n"
        "        mask = uv.x < -.5 || uv.y < -.5 || uv.y > .5 || uv.x > .35 ? 0. : mask;\n"
        "        mask = uv.y - sqrt(.0225-(uv.x+.5)*(uv.x+.5)) < -0.5 || uv.y - sqrt(.7225-(uv.x+.5)*(uv.x+.5)) > -0.5 ? 0. : mask;\n"
        "    } else {\n"
        "        mask = uv.y > 0.5 ? 0. : mask;\n"
        "        mask = uv.x < -.35 || uv.y < -.5 || uv.y > .5 || uv.x > .35 ? 0. : mask;\n"
        "    }\n"
        "    return mask;\n"
        "}\n"
        "void main() {\n"
        "    vec2 global_uv = (gl_FragCoord.xy)/" STR(PIXEL_HEIGHT) ".;\n"
        "    vec3 lightTransform = vec3(u_lightPosition + global_uv, 1.);\n"
        "    vec2 uv = 2.*(gl_PointCoord.xy+-.25);\n"
        "    uv.x = softBitCrunch(uv.x,8.);\n"
        "    uv.y = softBitCrunch(uv.y,8.);\n"
        "    float shadowAlpha = 0.;\n"
        "    float factor = 0.;\n"
        "    vec2 projected = vec2(0.);\n"
        "    for (int i = 1; i < 5; i++) {\n"
        "        factor += 1.;\n"
        "        projected = uv+lightTransform.xy*vec2(-0.5,0.5)*factor;\n"
        "        shadowAlpha += clamp(createMask(projected,u_lightPosition,fOrientation,fPipeType),0.,1.)*.2;\n"
        "    }\n"
        "    shadowAlpha = clamp(shadowAlpha, 0.,1.);\n"
        "    float mask = createMask(uv, u_lightPosition, fOrientation, fPipeType);\n"
        "    uv -=.5;\n"
        "    float totalAngle = fOrientation*3.141592/2.+fAngle;\n"
        "    uv = rotate(uv, totalAngle);\n"
        "    uv +=.5;\n"
        "    vec3 normals = getNormals(fPipeType, uv, fOrientation);  \n"
        "    normals.x *= -1.;\n"
        "    vec4 color =  vec4(0.4, 1., 0.3, 1.);\n"
        "    color.a = 1.;\n"
        "    vec2 uv1 = vec2(uv.x - gl_FragCoord.x + 400., uv.y + gl_FragCoord.y - 320.);\n"
        "    float light = dot(lightTransform, normals)/length(lightTransform);\n"
        "    color.rgb = mix(color.rgb,vec3(light),0.5);\n"
        "    color = clamp(color, 0., 1.);\n"
        "    vec4 shadowMask = vec4(0.);\n"
        "    shadowMask.a = clamp(shadowAlpha-mask,0.,1.);\n"
        "    color *= mask;\n"
        "    color = bitCrunchV4(color,8.);\n"
        "    gl_FragColor = vec4(color);\n"
        "}\n";
    
    const char vTileShaderStr[] =
        "precision mediump float;                                               \n"
        "attribute vec2 vCorners;                                               \n"
        "void main()                                                            \n"
        "{                                                                      \n"
        "   gl_Position = vec4(vCorners, 0., 1.); // + vec4(0.16,-0.2,0.,0.);   \n"
        "}                                                                      \n";

    const char fShinyTileShaderStr[] =
        "precision mediump float;\n"
        "uniform vec2 u_resolution;\n"
        "uniform vec2 u_lightPosition;\n"
        "float rand(float n) {\n"
        "    return fract(sin(n) * 34590.4532);\n"
        "}\n"
        "float rand(vec2 n) { \n"
        "	return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453)*2.-1.;\n"
        "}\n"
        "float noise(vec2 uv) {\n"
        "    const vec2 d = vec2(0., 1.0);\n"
        "    vec2 b = floor(uv), f = smoothstep(vec2(0.0), vec2(1.0), fract(uv));\n"
        "    return mix(mix(rand(b), rand(b + d.yx), f.x), mix(rand(b + d.xy), rand(b + d.yy), f.x), f.y);\n"
        "}\n"
        "float bitCrunch(float color, float bits) {\n"
        "    return floor(color*bits)/bits;\n"
        "}\n"
        "float softBitCrunch(float x, float bits) {\n"
        "    float w = pow(1.-fract(x*bits),.2);\n"
        "    return (1. - w + floor(x*bits))/bits;\n"
        "}\n"
        "vec4 bitCrunchV4(vec4 color, float bits) {\n"
        "    return vec4(softBitCrunch(color.r,bits),softBitCrunch(color.g,bits),softBitCrunch(color.b,bits),softBitCrunch(color.a,bits));\n"
        "}\n"
        "void main() {\n"
        "   vec2 uv = (gl_FragCoord.xy)/" STR(PIXEL_HEIGHT) ".;\n"
        "   vec3 lightTransform = vec3(u_lightPosition+uv, .5);                                \n"
        "   uv -= .5;\n"
        "   uv *= " STR(TILES_ROWS) ".;\n"
        "   vec2 uv2 = uv;\n"
        "   uv.x = mix(softBitCrunch(uv.x,16.), bitCrunch(uv.x, 16.),.0);                   \n"
        "   uv.y = mix(softBitCrunch(uv.y,16.),bitCrunch(uv.y, 16.),.0);                    \n"
        "   float brightness = 1.5;                                                         \n"
        "   float light = brightness/(length(lightTransform));                              \n"
        "   uv = fract(uv);\n"
        "   vec2 uv1 = uv2;\n"
        "   vec3 normals = vec3(0.,0.,1.);\n"
        "   float angle = .25;\n"
        "   float offset = .08;\n"
        "   float padding = 0.01;\n"
        "   normals.x = uv.x > 2.*offset && uv.x < 3. * offset && uv.x < 1. -uv.y && uv.x < uv.y ? -1. : normals.x;\n"
        "   normals.x = uv.x < 1. - 2.*offset && uv.x > 1. - 3. * offset && uv.x > 1. -uv.y && uv.x > uv.y ? 1. : normals.x;\n"
        "   normals.y = uv.y > 2.*offset && uv.y < 3. * offset && uv.y < 1. -uv.x && uv.y < uv.x ? -1. : normals.y;\n"
        "   normals.y = uv.y < 1. - 2.*offset && uv.y > 1. - 3. * offset && uv.y > 1. -uv.x && uv.y > uv.x ? 1. : normals.y;\n"
        "   normals.x = uv.x < 0. + offset && uv.x-padding < uv.y    && uv.x - padding < 1. - uv.y ? 1. : normals.x;\n"
        "   normals.x = uv.x > 1. - offset && uv.x+padding > 1.-uv.y && uv.x + padding > uv.y ? -1.     : normals.x;\n"
        "   normals.y = uv.y < 0. + offset && uv.y-padding < uv.x    && uv.y - padding < 1. - uv.x ? 1. : normals.y;\n"
        "   normals.y = uv.y > 1. - offset && uv.y+padding > 1.-uv.x && uv.y + padding > uv.x ? -1.     : normals.y;\n"
        "   normals.xy = uv.x < padding || uv.x > 1. - padding || uv.y < padding || uv.y > 1. - padding ? vec2(0.) : normals.xy;\n"
        "   vec3 bumps = vec3(0.,0.,1.);\n"
        "   float seed = 233.1222331;\n"
        "   const int num = 3;\n"
        "   float divisor = float(num);\n"
        "   for (int i = 0; i < num; i++) {\n"
        "       float I = float(i);\n"
        "       float seed = 222.4546;\n"
        "       float scale =  I*10.123;\n"
        "       bumps.x += .1*noise(uv2);\n"
        "       bumps.y += .1*noise(uv2+2.);\n"
        "       uv2 += rand(rand(bumps.xy));\n"
        "   }\n"
        "   bumps = normalize(bumps);\n"
        "   normals = mix(normals,bumps,0.3);\n"
        "   float colorR = dot(lightTransform*.8, normals*vec3(.9,1.,.9))*light;\n"
        "   float colorG = dot(lightTransform*.7, normals*vec3(1.,.8,1.))*light;\n"
        "   float colorB = dot(lightTransform*.4, normals*vec3(.4,.5,1.))*light;\n"
        "   vec4 color = vec4(colorR, colorG, colorB, 1.);\n"
        "   color = bitCrunchV4(color,8.);\n"
        "   gl_FragColor = color;\n"
        "}\n";

    shaders[static_cast<int>(ShaderType::PIPE)] = std::make_unique<ShaderProgram>(vShaderStr, fPipeShader);
    shaders[static_cast<int>(ShaderType::PIPE_SHADOW)] = std::make_unique<ShaderProgram>(vShaderStr, fPipeShadowShader);
    shaders[static_cast<int>(ShaderType::BACKGROUND)] = std::make_unique<ShaderProgram>(vTileShaderStr, fShinyTileShaderStr);

    shaders[static_cast<int>(ShaderType::PIPE)]->AddAttribute(VertexAttribute("vPosition", 2, NUMBER_OF_TILES, renderPositions));
    shaders[static_cast<int>(ShaderType::PIPE)]->AddAttribute(VertexAttribute("vOrientation", 1, NUMBER_OF_TILES, orientations));
    shaders[static_cast<int>(ShaderType::PIPE)]->AddAttribute(VertexAttribute("vAngle", 1, NUMBER_OF_TILES, angles));
    shaders[static_cast<int>(ShaderType::PIPE)]->AddAttribute(VertexAttribute("vPipeType", 1, NUMBER_OF_TILES, pipeTypes));
    shaders[static_cast<int>(ShaderType::PIPE)]->AddUniform(Uniform("u_origo", 2, origo.array));
    shaders[static_cast<int>(ShaderType::PIPE)]->AddUniform(Uniform("u_lightPosition", 2, lightPosition.array));

    shaders[static_cast<int>(ShaderType::PIPE_SHADOW)]->AddAttribute(VertexAttribute("vPosition", 2, NUMBER_OF_TILES, renderPositions));
    shaders[static_cast<int>(ShaderType::PIPE_SHADOW)]->AddAttribute(VertexAttribute("vOrientation", 1, NUMBER_OF_TILES, orientations));
    shaders[static_cast<int>(ShaderType::PIPE_SHADOW)]->AddAttribute(VertexAttribute("vAngle", 1, NUMBER_OF_TILES, angles));
    shaders[static_cast<int>(ShaderType::PIPE_SHADOW)]->AddAttribute(VertexAttribute("vPipeType", 1, NUMBER_OF_TILES, pipeTypes));
    shaders[static_cast<int>(ShaderType::PIPE_SHADOW)]->AddUniform(Uniform("u_origo", 2, origo.array));
    shaders[static_cast<int>(ShaderType::PIPE_SHADOW)]->AddUniform(Uniform("u_lightPosition", 2, lightPosition.array));

    shaders[static_cast<int>(ShaderType::BACKGROUND)]->AddAttribute(VertexAttribute("vCorners", 2, 4, corners));
    shaders[static_cast<int>(ShaderType::BACKGROUND)]->AddUniform(Uniform("u_lightPosition", 2, lightPosition.array));
    
    InitializeScreenPositions();

    for (int i = 0; i < SHADER_TYPE_COUNT; i++) {
        std::shared_ptr<unsigned int[NUMBER_OF_TILES]> indices(new unsigned int[NUMBER_OF_TILES]);
        elementIndexArrays[i] = indices;
    }

    const int numElementArrays = SHADER_TYPE_COUNT;

    for(int i = 0; i < SHADER_TYPE_COUNT; i++)
    {
        GlCall(glGenBuffers(1, &elementBuffers[i]));
        GlCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffers[i]));
        GlCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, NUMBER_OF_TILES * sizeof(unsigned int), elementIndexArrays[i].get(), GL_DYNAMIC_DRAW));
    }

    _isInitialized = true;
}

void Renderer::InitializeScreenPositions() {
    int i = 0;
    for (int x = 0; x < TILES_COLUMNS; x++) {
        for (int y = 0; y < TILES_ROWS; y++) {
            i = x + y*TILES_COLUMNS;
            movementRemaining[i].x = 0.0f;
            movementRemaining[i].y = 0.0f;
            gridPositions[2*i]     = -1. + (x+.5)*x_tile_size;
            gridPositions[2*i + 1] =  1. - (y+.5)*y_tile_size;
        }
    }
}

void Renderer::DrawGrid() {
    
    int typeIndex = static_cast<int>(ShaderType::BACKGROUND);
    if (!shaders[typeIndex])
        return;

    shaders[typeIndex]->DrawElements(cornerIndexArray, elementBuffers[typeIndex], 6, GL_TRIANGLES);
}

void Renderer::DrawShaderType(ShaderType type) {

    int typeIndex = static_cast<int>(type);
    if (!shaders[typeIndex])
        return;

    shaders[typeIndex]->DrawElements(elementIndexArrays[typeIndex], elementBuffers[typeIndex], numberOfShaderType[typeIndex], GL_POINTS);
}

RotationCounts Renderer::HandleAngle(RotationCounts rotationCounts) 
{
    if (rotationCounts.leftRotations > 0) {
        rotationCounts.leftRotations--;
        angleRemaining -= 3.141592/2.;
    }

    if (rotationCounts.rightRotations > 0) {
        rotationCounts.rightRotations--;
        angleRemaining += 3.141592/2.;
    }

    if (abs(angleRemaining) > 0.01) {
        angleRemaining *= 0.6;
    } else {
        angleRemaining = 0;
    }

    return rotationCounts;
}

void Renderer::HandleMovement(posf deltaPos, int tileIndex, bool& hasMovementStarted) 
{
    if (!hasMovementStarted && movementRemaining[tileIndex].x == 0 && movementRemaining[tileIndex].y == 0) {
        return;
    }

    if (hasMovementStarted)
        movementRemaining[tileIndex] = posf{movementRemaining[tileIndex].x - ((deltaPos.x)*x_tile_size),
                                            movementRemaining[tileIndex].y + ((deltaPos.y)*y_tile_size)};
        hasMovementStarted = false;

    if (abs(movementRemaining[tileIndex].x) + abs(movementRemaining[tileIndex].y) > 0.01) {
        movementRemaining[tileIndex].x *= 0.6f;
        movementRemaining[tileIndex].y *= 0.6f;
    } else {
        movementRemaining[tileIndex].x = 0.f;
        movementRemaining[tileIndex].y = 0.f;
    }

}

void Renderer::HandlePartialAngle(float& partialAngle, int& rotationSign, float& amountRemaining) 
{
    if (rotationSign == 0 && amountRemaining == 0)
        return;
    
    if (rotationSign != 0) {
        amountRemaining = -rotationSign;
        amountRemaining = abs(amountRemaining) > 1. ? (amountRemaining > 0) - (amountRemaining < 0) : amountRemaining;
        rotationSign = 0;
    }

    if (abs(amountRemaining) > 0.1) {
        amountRemaining *= 0.7f;
        angleRemaining = partialAngle *(-cos(amountRemaining * 6.28f)*0.5f + 0.5);
    } else {
        partialAngle = 0;
        amountRemaining = 0;
        angleRemaining = 0;
    }
}

std::vector<int> ShaderTypeFromEntityType(EntityType entityType) {
    std::vector<int> types;
    switch (entityType) 
    { 
        case EntityType::BENT_PIPE:
            types.push_back(static_cast<int>(ShaderType::PIPE));
            types.push_back(static_cast<int>(ShaderType::PIPE_SHADOW));
            return types;
        case EntityType::STRAIGHT_PIPE:
            types.push_back(static_cast<int>(ShaderType::PIPE));
            types.push_back(static_cast<int>(ShaderType::PIPE_SHADOW));
            return types;
        case EntityType::BACKGROUND:
            types.push_back(static_cast<int>(ShaderType::BACKGROUND));
            return types;
        default:
            types.push_back(static_cast<int>(ShaderType::NONE));
            return types;
    }
}

void Renderer::UpdateGraphicsData(std::unique_ptr<EntityManager>& em)
{
    em->rotationCounts = HandleAngle(em->rotationCounts);
    HandlePartialAngle(em->partialRotationAngle, em->partialRotationSign, partialRotationRemaining);

    int origoIndex = em->getTileIndexFromEntityIndex(0);
    
    origo.x = gridPositions[origoIndex*2];
    origo.y = gridPositions[origoIndex*2+1];

    std::fill(numberOfShaderType, numberOfShaderType+SHADER_TYPE_COUNT, 0);

    std::vector<int> movableIndices;
    std::vector<int> nonMovableIndices;

    for (int i = 0; i < em->numEntities; i++) {
        unsigned int tileIndex = em->getTileIndexFromEntityIndex(i);
        EntityType entityType = em->types[i];
        std::vector<int> shaderTypes = ShaderTypeFromEntityType(entityType);
        for (int shaderType : shaderTypes) {
            if (shaderType >= SHADER_TYPE_COUNT)
                continue;
            int index = numberOfShaderType[shaderType];
            if (index >= NUMBER_OF_TILES)
                continue;

            elementIndexArrays[shaderType].get()[index] = tileIndex;
            angles[tileIndex] = (em->isMovable[i] || em->isTemporarilyMovable[i]) && !(em->gotPushed[i]) ? angleRemaining : 0;
            orientations[tileIndex] = em->orientations[i];
            if (shaderType == static_cast<int>(ShaderType::PIPE)) 
            {
                switch (entityType) 
                {
                case EntityType::BENT_PIPE:
                    pipeTypes[tileIndex] = 1.;
                    break;
                case EntityType::STRAIGHT_PIPE:
                    pipeTypes[tileIndex] = 0.;
                    break;
                default:
                    break;
                }
            }
            
            HandleMovement(em->deltaPositions[i], tileIndex, em->hasMoved[i]);
            renderPositions[tileIndex*2] = gridPositions[tileIndex*2] + movementRemaining[tileIndex].x;
            renderPositions[tileIndex*2+1] = gridPositions[tileIndex*2+1] + movementRemaining[tileIndex].y;
            numberOfShaderType[shaderType]++;
        }
    }

    int pipeType = static_cast<int>(ShaderType::PIPE);
    unsigned int* pipeIndices = elementIndexArrays[pipeType].get();

    std::function<bool (int, int)> sortFunc;

    if (angleRemaining > -1.5 && angleRemaining <= .01 || angleRemaining > 1.5) {
        sortFunc = [](int a, int b) {return a < b;};
    } else if (angleRemaining < 1.5 && angleRemaining >= 0)  {
        sortFunc = [](int a, int b) {return a % TILES_COLUMNS != b % TILES_COLUMNS ? a > b : a < b;};
    } else {
        sortFunc = [](int a, int b) {return a < b;};
    }

    std::sort(elementIndexArrays[pipeType].get(), 
            elementIndexArrays[pipeType].get() + numberOfShaderType[pipeType],
            sortFunc);
}

void Renderer::Draw() 
{
    unsigned int start = SDL_GetTicks();

    unsigned int t = SDL_GetTicks();
    time = t*0.0005f;
    lightPosition = posf{-0.5f-0.5f*cos(time),-0.5f-0.5f*sin(time)};

    DrawGrid();

    for (int type = 0; type < SHADER_TYPE_COUNT; type++) {
        DrawShaderType(static_cast<ShaderType>(type));
    }
}