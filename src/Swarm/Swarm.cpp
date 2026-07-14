#include "Swarm.hpp"

void Swarm::sense(unsigned int pID)
{
    unsigned int tid = static_cast<unsigned int>(omp_get_thread_num());

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

    int level = 0;
    bool foundEnough = false;
    const int maxLvlX = inX / 2;
    const int maxLvlY = inY / 2;

    while (true)
    {
        // stop once searched entire domain
        if (level > maxLvlX && level > maxLvlY) break;

        if (level == 0)
        {
            findNeighborsInCell(homeCell, pID, position, localBuf);
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
    
                    int targetX = (X + dX + inX) % inX;
                    int targetY = (Y + dY + inY) % inY;
                    unsigned int cellID = static_cast<unsigned int>(targetX + targetY * inX);
    
                    findNeighborsInCell(cellID, pID, position, localBuf);
                }
            }
        }

        // check to see if enough neighbors have been found
        if (localBuf[0].distanceSq != std::numeric_limits<float>::infinity())
        {
            // if found enough neighbors, search out to one more level before terminating, but
            // if our worst neighbor is closer than a single cell width, 
            // it is physically impossible for Level 2 to contain anything closer.
            const float safeRadiusSq = grid.cellSize * grid.cellSize;
            if (localBuf[0].distanceSq < safeRadiusSq)
            {
                break;
            }

            if (!foundEnough)
            {
                foundEnough = true;
                level++;
                continue;
            }
            break;
        }
        else
        {
            level++;
        }
    }

    // sort neighbors by distance
    std::sort(localBuf, localBuf + nNeighbors);

    // now that we have a list of the n-nearest neighbor indices, we can perform the sensing step
    glm::vec2 headingSum = headings[pID];

    for (unsigned int n = 0; n < nNeighbors; ++n)
    {
        if (localBuf[n].distanceSq != std::numeric_limits<float>::infinity())
        {
            // unsigned int neighborIdx = ids[n];
            unsigned int neighborIdx = localBuf[n].id;
            
            // no weight
            // float weight = 1.0f;
            
            // distance-based alignment weights
            // float weight = (dist[n] < 0.1f) ? 1.0f / 0.1f : 1.0f / dist[n];

            // closeness-based alignment weights
            float weight = 1.0f - (static_cast<float>(n) / static_cast<float>(nNeighbors + 1));;
            // float weight = powf(0.5f, static_cast<float>(n));

            headingSum += headings[neighborIdx] * weight;
        }
        else
        {
            break;
        }
    }
    
    targetAngles[pID] = glm::atan(headingSum.y, headingSum.x) + noiseScale * PI * (2.0f * uniformDist(rngs[tid]) - 1.0f);
    
}

void Swarm::tradSense(unsigned int pID)
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
    
    targetAngles[pID] = glm::atan(headingSum.y, headingSum.x) + noiseScale * PI * (2.0f * uniformDist(rngs[tid]) - 1.0f);
}
