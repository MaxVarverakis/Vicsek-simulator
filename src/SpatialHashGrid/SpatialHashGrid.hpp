#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <algorithm>

#include "../Particle/Particle.hpp"

struct SpatialHashGrid
{
    float cellSize;
    unsigned int nX, nY;
    unsigned int numCells;

    std::vector<unsigned int> cellOffsets, scannedCells, gridParticles, cellIDs;

    SpatialHashGrid(float size, float width, float height);

    unsigned int hash(const Particle& particle);
    void build(const std::vector<Particle>& particles);
    std::vector<unsigned int> neighbors(unsigned int cellID);
    std::vector<unsigned int> getCellsAtLevel(unsigned int cellID, unsigned int level, unsigned int particleIdx);
};
