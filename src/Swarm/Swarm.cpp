#include "Swarm.hpp"

void Swarm::sense(unsigned int pID, uint32_t frameHash)
{
    // Allocate completely on the actual thread stack (NO HEAP/MALLOC)
    // immune to allocator contention spikes
    Neighbor* localBuf = static_cast<Neighbor*>(alloca(nNeighbors * sizeof(Neighbor)));
    
    for (unsigned int n = 0; n < nNeighbors; ++n)
    {
        localBuf[n] = { std::numeric_limits<float>::infinity(), 0 };
    }

    // Convert stack vector to max heap
    std::make_heap(localBuf, localBuf + nNeighbors);

    glm::vec2& position = positions[pID];
    unsigned int homeCell = grid.hash(position);

    int X = static_cast<int>(homeCell % grid.nX);
    int Y = static_cast<int>(homeCell / grid.nX);
    const int inX = static_cast<int>(grid.nX);
    const int inY = static_cast<int>(grid.nY);

    const int maxLvlX = inX / 2;
    const int maxLvlY = inY / 2;

    // definitions for early exit (local cell offsets)
    const float cellMinX = static_cast<float>(X) * grid.cellSize;
    const float cellMinY = static_cast<float>(Y) * grid.cellSize;
    const float dx = position.x - cellMinX;
    const float dy = position.y - cellMinY;

    const float closestWallDist = fminf(
        fminf(dx, grid.cellSize - dx),
        fminf(dy, grid.cellSize - dy)
    );
    const float wallRadiusSq = closestWallDist * closestWallDist;

    // search the entire domain through levels of increasing distance to home cell
    for (int level = 0; level <= std::max(maxLvlX, maxLvlY); ++level)
    {
        if (level == 0)
        {
            findNeighborsInCell(homeCell, pID, position, localBuf, false);

            // stop if farthest neighbor is < distance to closest home cell boundary
            if (localBuf[0].distanceSq < wallRadiusSq) break;
        }
        else
        {
            // Clamp loop bounds to the maximum physical physical limits of the grid axes.
            // This natively prevents duplicate checking from periodic wrapping
            int limitY = std::min(level, maxLvlY);
            int limitX = std::min(level, maxLvlX);
            
            for (int dY = -limitY; dY <= limitY; ++dY)
            {
                const bool yIsInner = (std::abs(dY) < level);

                for (int dX = -limitX; dX <= limitX; ++dX)
                {
                    // Only process the outer ring perimeter
                    if (yIsInner && std::abs(dX) < level) continue;
    
                    int unwrappedX = X + dX;
                    int unwrappedY = Y + dY;

                    int targetX = (unwrappedX + inX) % inX;
                    int targetY = (unwrappedY + inY) % inY;
                    unsigned int cellID = static_cast<unsigned int>(targetX + targetY * inX);
    
                    // check if need to wrap particle-particle distance
                    
                    bool wrap = (unwrappedX < 0 || unwrappedX >= inX || unwrappedY < 0 || unwrappedY >= inY);

                    findNeighborsInCell(cellID, pID, position, localBuf, wrap);
                }
            }
        }

        // check to see if enough neighbors have been found
        if (localBuf[0].distanceSq != std::numeric_limits<float>::infinity())
        {

            float floatLvl = static_cast<float>(level);

            // extract distance from particle to the nearest outer boundary for current level
            float dLeft   = dx + floatLvl * grid.cellSize;
            float dRight  = (floatLvl + 1.0f) * grid.cellSize - dx;
            float dBottom = dy + floatLvl * grid.cellSize;
            float dTop    = (floatLvl + 1.0f) * grid.cellSize - dy;

            float minDistToBoundary = fminf(fminf(dLeft, dRight), fminf(dBottom, dTop));

            // if 5th neighbor is closer than unsearched boundary, terminate immediately
            if (localBuf[0].distanceSq <= minDistToBoundary * minDistToBoundary) break;
        }
    }

    // sort neighbors by distance
    std::sort(localBuf, localBuf + nNeighbors);

    // for debugging
    if (debugBool && pID == debugSelectedID)
    {
        // Thread-safe because pID is unique to exactly one loop iteration
        debugNeighbors.assign(localBuf, localBuf + nNeighbors);
    }

    // now that we have a list of the n-nearest neighbor indices, we can perform the sensing step
    glm::vec2 headingSum = headings[pID];

    for (unsigned int n = 0; n < nNeighbors; ++n)
    {
        if (localBuf[n].distanceSq != std::numeric_limits<float>::infinity())
        {
            // no weight
            // float weight = 1.0f;
            
            // distance-based alignment weights
            // float weight = (localBuf[n].distanceSq < 0.1f) ? 1.0f / 0.1f : 1.0f / localBuf[n].distanceSq;

            // closeness-based alignment weights
            float weight = 1.0f - (static_cast<float>(n) / static_cast<float>(nNeighbors));;
            // float weight = powf(0.5f, static_cast<float>(n));

            headingSum += headings[localBuf[n].id] * weight;
        }
        else
        {
            break;
        }
    }
    
    float noise = deterministicRNG(particleIDs[pID], frameHash, 3);
    targetAngles[pID] = glm::atan(headingSum.y, headingSum.x) + noiseScale * PI * noise;
    
}

void Swarm::tradSense(unsigned int pID, uint32_t frameHash)
{
    static const float senseRadius = grid.cellSize;
    
    unsigned int tid = static_cast<unsigned int>(omp_get_thread_num());

    glm::vec2 headingSum = headings[pID];
    glm::vec2& position = positions[pID];

    unsigned int homeCell = grid.hash(position);

    for (unsigned int level = 0; level < 2; ++level)
    {
        std::vector<unsigned int>& cells = grid.getCellsAtLevel(homeCell, level, pID, tid);
        for (unsigned int cellID : cells)
        {
            unsigned int start = grid.cellOffsets[cellID    ];
            unsigned int end   = grid.cellOffsets[cellID + 1];

            for (unsigned int k = start; k < end; ++k)
            {
                unsigned int j = grid.gridParticles[k];
                if (j == pID) continue;

                glm::vec2 delta = position - positions[j];
                // account for periodic BCs
                shortestDistance(delta);

                float d2 = glm::dot(delta, delta);

                if (d2 < senseRadius * senseRadius)
                {
                    float weight = 1.0f;
                    // float weight = (d2 < 0.1f) ? 1.0f / 0.1f : 1.0f / d2;

                    headingSum += headings[j] * weight;
                }
            }
        }
    }
    
    float noise = deterministicRNG(particleIDs[pID], frameHash, 3);
    targetAngles[pID] = glm::atan(headingSum.y, headingSum.x) + noiseScale * PI * noise;
}
