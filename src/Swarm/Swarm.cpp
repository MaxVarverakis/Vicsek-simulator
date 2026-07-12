#include "Swarm.hpp"

void Swarm::nsSense(unsigned int pID)
{
    for (unsigned int j = 0; j < positions.size(); ++j)
    {
        
        if (pID == j) continue;

        glm::vec2 delta = positions[pID] - positions[j];
        // account for periodic BCs
        if (delta.x > x_max / 2.0f) { delta.x -= x_max; }
        if (delta.x < -x_max / 2.0f) { delta.x += x_max; }
        if (delta.y > y_max / 2.0f) { delta.y -= y_max; }
        if (delta.y < -y_max / 2.0f) { delta.y += y_max; }

        // compare squared distance to avoid unnecessary sqrt
        float d2 = glm::dot(delta, delta);

        for (unsigned int n = 0; n < nNeighbors; ++n)
        {
            if (d2 < scratch_distances[n])
            {
                // bump and insert
                // std::move_backward(scratch_distances.begin() + n, scratch_distances.end() - 1, scratch_distances.end());
                // std::move_backward(scratch_neighborIds.begin() + n, scratch_neighborIds.end() - 1, scratch_neighborIds.end());

                for (unsigned int m = nNeighbors - 1; m > n; --m)
                {
                    scratch_distances[m] = scratch_distances[m-1];
                    scratch_neighborIds[m] = scratch_neighborIds[m-1];
                }

                scratch_distances[n] = d2;
                scratch_neighborIds[n] = j;
                break;
            }
        }
    }

    // now that we have a list of the n-nearest neighbor indices, we can perform the sensing step
    glm::vec2 headingSum = headings[pID];

    for (unsigned int n = 0; n < nNeighbors; ++n)
    {
        if (scratch_distances[n] != std::numeric_limits<float>::infinity())
        {
            unsigned int neighborIdx = scratch_neighborIds[n];
            
            // no weight
            // float weight = 1.0f;
            
            // distance-based alignment weights
            // float weight = (scratch_distances[n] < 0.1f) ? 1.0f / 0.1f : 1.0f / scratch_distances[n];

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
    
    targetAngles[pID] = glm::atan(headingSum.y, headingSum.x) + noiseScale * PI * (2.0f * dist(rng) - 1.0f);
}

void Swarm::sense(unsigned int pID)
{
    std::fill(scratch_neighborIds.begin(), scratch_neighborIds.end(), 0);
    std::fill(scratch_distances.begin(), scratch_distances.end(), std::numeric_limits<float>::infinity());

    unsigned int homeCell = grid.hash(positions[pID]);
    unsigned int level = 0;
    bool foundEnough = false;

    while (true)
    {
        grid.getCellsAtLevel(homeCell, level, pID);

        if (grid.outCells.empty() || level > std::max(grid.nX, grid.nY) / 2) 
        {
            break;
        }

        for (unsigned int cellID : grid.outCells)
        {
            unsigned int start = grid.cellOffsets[cellID    ];
            unsigned int end   = grid.cellOffsets[cellID + 1];

            // loop through particles in cell `cellID`
            for (unsigned int k = start; k < end; ++k)
            {
                unsigned int j = grid.gridParticles[k];
                if (pID == j) continue;

                glm::vec2 delta = positions[pID] - positions[j];
                // account for periodic BCs
                if (delta.x > x_max / 2.0f) { delta.x -= x_max; }
                if (delta.x < -x_max / 2.0f) { delta.x += x_max; }
                if (delta.y > y_max / 2.0f) { delta.y -= y_max; }
                if (delta.y < -y_max / 2.0f) { delta.y += y_max; }

                // compare squared distance to avoid unnecessary sqrt
                float d2 = glm::dot(delta, delta);

                for (unsigned int n = 0; n < nNeighbors; ++n)
                {
                    if (d2 < scratch_distances[n])
                    {
                        // bump and insert
                        // std::move_backward(scratch_distances.begin() + n, scratch_distances.end() - 1, scratch_distances.end());
                        // std::move_backward(scratch_neighborIds.begin() + n, scratch_neighborIds.end() - 1, scratch_neighborIds.end());

                        for (unsigned int m = nNeighbors - 1; m > n; --m)
                        {
                            scratch_distances[m] = scratch_distances[m-1];
                            scratch_neighborIds[m] = scratch_neighborIds[m-1];
                        }

                        scratch_distances[n] = d2;
                        scratch_neighborIds[n] = j;
                        break;
                    }
                }
            }
        }

        // check to see if enough neighbors have been found
        if (scratch_distances.back() != std::numeric_limits<float>::infinity())
        {
            if (!foundEnough)
            {
                // if found enough neighbors, search out to one more level before terminating
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

    // now that we have a list of the n-nearest neighbor indices, we can perform the sensing step
    glm::vec2 headingSum = headings[pID];

    for (unsigned int n = 0; n < nNeighbors; ++n)
    {
        if (scratch_distances[n] != std::numeric_limits<float>::infinity())
        {
            unsigned int neighborIdx = scratch_neighborIds[n];
            
            // no weight
            // float weight = 1.0f;
            
            // distance-based alignment weights
            // float weight = (scratch_distances[n] < 0.1f) ? 1.0f / 0.1f : 1.0f / scratch_distances[n];

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
    
    targetAngles[pID] = glm::atan(headingSum.y, headingSum.x) + noiseScale * PI * (2.0f * dist(rng) - 1.0f);
    
}

void Swarm::tradSense(unsigned int pID)
{
    static const float senseRadius = grid.cellSize;

    glm::vec2 headingSum = headings[pID];

    unsigned int homeCell = grid.hash(positions[pID]);

    for (unsigned int level = 0; level < 2; ++level)
    {
        grid.getCellsAtLevel(homeCell, level, pID);
        for (unsigned int cellID : grid.outCells)
        {
            unsigned int start = grid.cellOffsets[cellID    ];
            unsigned int end   = grid.cellOffsets[cellID + 1];

            for (unsigned int k = start; k < end; ++k)
            {
                unsigned int j = grid.gridParticles[k];
                if (j == pID) continue;

                glm::vec2 delta = positions[pID] - positions[j];
                // account for periodic BCs
                if (delta.x > x_max / 2.0f) { delta.x -= x_max; }
                if (delta.x < -x_max / 2.0f) { delta.x += x_max; }
                if (delta.y > y_max / 2.0f) { delta.y -= y_max; }
                if (delta.y < -y_max / 2.0f) { delta.y += y_max; }

                float d2 = glm::dot(delta, delta);

                if (d2 < senseRadius * senseRadius)
                {
                    float weight = 1.0f;
                    // float weight = (distance < 0.1f) ? 1.0f / 0.1f : 1.0f / distance;

                    headingSum += headings[j] * weight;
                }
            }
        }
    }
    
    targetAngles[pID] = glm::atan(headingSum.y, headingSum.x) + noiseScale * PI * (2.0f * dist(rng) - 1.0f);
}
