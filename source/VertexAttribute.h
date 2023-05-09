#include "common.h"

class VertexAttribute 
{
    public:
        char const* name;
        unsigned int buffer;
        int size;
        int count;
        std::shared_ptr<float[]> data;

        VertexAttribute(char const* name, int size, int count, std::shared_ptr<float[]>);
};