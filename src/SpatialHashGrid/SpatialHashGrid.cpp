#include "SpatialHashGrid.hpp"

SpatialHashGrid::SpatialHashGrid(float size, float width, float height, unsigned int numParticles)
    : cellSize { size }
    , nX ( width / size )
    , nY ( height / size )
    , numCells { nX * nY }
{
    cellOffsets.resize(numCells);
    gridParticles.resize(numParticles);
}
