#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <algorithm>

struct SpatialHashGrid
{
    float cellSize;
    unsigned int nX, nY;
    unsigned int numCells;

    std::vector<unsigned int> cellOffsets, scannedCells, gridParticles, cellIDs;
    std::vector<unsigned int> outCells;
    std::vector<unsigned int> offsetCopy;

    SpatialHashGrid(float size, float width, float height);

    unsigned int hash(const glm::vec2& position);
    void build(const std::vector<glm::vec2>& positions);
    std::vector<unsigned int> neighbors(unsigned int cellID);
    void getCellsAtLevel(unsigned int cellID, unsigned int level, unsigned int particleIdx);
};
