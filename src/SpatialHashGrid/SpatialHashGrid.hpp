#pragma once

#include <vector>
#include <glm/glm.hpp>

struct SpatialHashGrid
{
    float cellSize;
    unsigned int nX, nY;
    unsigned int numCells;

    std::vector<unsigned int> cellOffsets, gridParticles;

    SpatialHashGrid(float size, float width, float height, unsigned int numParticles);
};
