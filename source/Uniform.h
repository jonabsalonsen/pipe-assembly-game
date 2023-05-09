#include "common.h"

class Uniform {
    public:
        char const* name;
        int size;
        const float* data;

        Uniform(char const* name, int size, float* data);
};