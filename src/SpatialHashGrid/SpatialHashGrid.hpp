#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <algorithm>

struct SpatialHashGrid
{
    float cellSize;
    unsigned int nX, nY;
    unsigned int numCells;

    std::vector<unsigned int> cellOffsets, scannedCells, gridParticles, cellIDs, offsetCopy;
    std::vector<std::vector<unsigned int>> outCells;

    SpatialHashGrid(float size, float width, float height, unsigned int num_threads);

    unsigned int hash(const glm::vec2& position);
    void build(const std::vector<glm::vec2>& positions);
    std::vector<unsigned int> neighbors(unsigned int cellID);
    std::vector<unsigned int>& getCellsAtLevel(unsigned int cellID, unsigned int level, unsigned int particleIdx, unsigned int threadID);
};
