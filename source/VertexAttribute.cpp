#include "VertexAttribute.h"
VertexAttribute::VertexAttribute(char const* _name, int _size, const int _count, std::shared_ptr<float[]> _data)
{
    name = _name;
    size = _size;
    count = _count;
    data = _data;
    buffer = 0;
    GlCall(glGenBuffers(1, &buffer));
    GlCall(glBindBuffer(GL_ARRAY_BUFFER, buffer));
    GlCall(glBufferData(GL_ARRAY_BUFFER, size * NUMBER_OF_TILES * sizeof(float), data.get(), GL_STATIC_DRAW));
}
