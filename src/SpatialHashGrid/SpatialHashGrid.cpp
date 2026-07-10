#include "SpatialHashGrid.hpp"

SpatialHashGrid::SpatialHashGrid(float size, float width, float height)
    : cellSize { size }
    , nX { static_cast<unsigned int>(width / size) }
    , nY { static_cast<unsigned int>(height / size) }
    , numCells { nX * nY }
{
    cellOffsets.resize(numCells + 1);
    scannedCells.resize(numCells);
}

unsigned int SpatialHashGrid::hash(const Particle& particle)
{
    unsigned int X = static_cast<unsigned int>(
        std::clamp(
            static_cast<int>(particle.position.x / cellSize),
            0,
            static_cast<int>(nX - 1)
        )
    );
    unsigned int Y = static_cast<unsigned int>(
        std::clamp(
            static_cast<int>(particle.position.y / cellSize),
            0,
            static_cast<int>(nY - 1)
        )
    );
    
    // row-major
    return X + Y * nX;
}

void SpatialHashGrid::build(const std::vector<Particle>& particles)
{
    if (gridParticles.size() != particles.size())
    {
        gridParticles.resize(particles.size());
        cellIDs.resize(particles.size());
    }

    // clear the offsets from previous build
    std::fill(cellOffsets.begin(), cellOffsets.end(), 0);

    // get counts
    for (unsigned int i = 0; i < gridParticles.size(); ++i)
    {
        unsigned int cellID = hash(particles[i]);
        cellIDs[i] = cellID;
        cellOffsets[cellID] += 1;
    }

    // calculate offsets
    unsigned int sum = 0;
    for (unsigned int i = 0; i < numCells; ++i)
    {
        unsigned int count = cellOffsets[i];
        cellOffsets[i] = sum;
        sum += count;
    }
    // final end-marker total
    cellOffsets[numCells] = sum;

    // sort particle IDs based on cell
    // create temporary copy so that the offsets array remains intact
    std::vector<unsigned int> offsetCopy = cellOffsets;
    for (unsigned int i = 0; i < gridParticles.size(); ++i)
    {
        // `a++` means that offset is set to `a` and then `a` is incremented
        unsigned int offset = offsetCopy[cellIDs[i]]++;
        gridParticles[offset] = i;
    }
}

std::vector<unsigned int> SpatialHashGrid::neighbors(unsigned int cellID)
{
    // only works for levels 0+1
    // w, e, n, s, nw, ne, sw, se

    // un-flatten the indices
    unsigned int X = cellID % nX;
    unsigned int Y = cellID / nX;

    unsigned int lX = (X == 0) ? nX - 1 : X - 1;
    unsigned int rX = (X == nX - 1) ? 0 : X + 1;
    
    unsigned int dY = (Y == 0) ? nY - 1 : Y - 1;
    unsigned int uY = (Y == nY - 1) ? 0 : Y + 1;

    // convert back to flattened indices (row-major)
    unsigned int  w = lX +  Y * nX;
    unsigned int  e = rX +  Y * nX;
    unsigned int  n =  X + uY * nX;
    unsigned int  s =  X + dY * nX;
    
    unsigned int nw = lX + uY * nX;
    unsigned int ne = rX + uY * nX;
    unsigned int sw = lX + dY * nX;
    unsigned int se = rX + dY * nX;

    return { w, e, n, s, nw, ne, sw, se };
}

std::vector<unsigned int> SpatialHashGrid::getCellsAtLevel(unsigned int cellID, unsigned int level, unsigned int particleIdx)
{
    std::vector<unsigned int> cells;

    // un-flatten home cell coordinates (row-major)
    int X = static_cast<int>(cellID % nX);
    int Y = static_cast<int>(cellID / nX);

    // handle home cell special case
    if (level == 0)
    {
        cells.push_back(cellID);
        return cells;
    }

    int iLevel = static_cast<int>(level);
    int inX = static_cast<int>(nX);
    int inY = static_cast<int>(nY);

    int maxLevelX = inX / 2;
    int maxLevelY = inY / 2;

    for (int dY = -iLevel; dY <= iLevel; ++dY)
    {
        // prevent duplicate checking due to periodic BCs
        if (std::abs(dY) > maxLevelY) continue;

        for (int dX = -iLevel; dX <= iLevel; ++dX)
        {
            // prevent duplicate checking due to periodic BCs
            if (std::abs(dX) > maxLevelX) continue;
            
            // restrict to boundary cells
            if ((std::abs(dX) < iLevel) && (std::abs(dY) < iLevel))
            {
                continue;
            }

            // get left/right/up/down while handling BCs
            int targetX = (X + dX) % inX;
            if (targetX < 0) { targetX += inX; }
            
            int targetY = (Y + dY) % inY;
            if (targetY < 0) { targetY += inY; }

            // re-flatten to row-major
            unsigned int targetCell = static_cast<unsigned int>(targetX + targetY * inX);

            unsigned int stamp = particleIdx + 1;

            // check if cell is already in list
            // (this can happen for small grids like 3x2)
            if (scannedCells[targetCell] != stamp)
            {
                scannedCells[targetCell] = stamp; // flag to avoid double dipping
                cells.push_back(targetCell);
            }
        }
    }

    return cells;
}
