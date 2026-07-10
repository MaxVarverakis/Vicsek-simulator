#include "Swarm.hpp"

void Swarm::sense()
{
    for (unsigned int i = 0; i < particles.size(); ++i)
    {
        std::fill(scratch_neighborIds.begin(), scratch_neighborIds.end(), 0);
        std::fill(scratch_distances.begin(), scratch_distances.end(), std::numeric_limits<float>::infinity());

        unsigned int homeCell = grid.hash(particles[i]);
        unsigned int level = 0;
        bool foundEnough = false;

        while (true)
        {
            grid.getCellsAtLevel(homeCell, level, i);

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
                    if (i == j) continue;

                    glm::vec2 delta = particles[i].position - particles[j].position;
                    // account for periodic BCs
                    if (delta.x > x_max / 2.0f) { delta.x -= x_max; }
                    if (delta.x < -x_max / 2.0f) { delta.x += x_max; }
                    if (delta.y > y_max / 2.0f) { delta.y -= y_max; }
                    if (delta.y < -y_max / 2.0f) { delta.y += y_max; }

                    float distance = glm::length(delta);

                    for (unsigned int n = 0; n < nNeighbors; ++n)
                    {
                        if (distance < scratch_distances[n])
                        {
                            // bump and insert
                            std::move_backward(scratch_distances.begin() + n, scratch_distances.end() - 1, scratch_distances.end());
                            std::move_backward(scratch_neighborIds.begin() + n, scratch_neighborIds.end() - 1, scratch_neighborIds.end());

                            scratch_distances[n] = distance;
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
        float sinSum = glm::sin(particles[i].angle);
        float cosSum = glm::cos(particles[i].angle);

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

                sinSum += glm::sin(particles[neighborIdx].angle) * weight;
                cosSum += glm::cos(particles[neighborIdx].angle) * weight;
            }
            else
            {
                break;
            }
        }
        
        targetAngles[i] = glm::atan(sinSum, cosSum) + noiseScale * PI * (2.0f * dist(rng) - 1.0f);
    }
}

std::vector<float> Swarm::tradSense()
{
    const float senseRadius = grid.cellSize;

    std::vector<float> angles(particles.size());

    for (unsigned int i = 0; i < particles.size(); ++i)
    {
        float sinSum = glm::sin(particles[i].angle);
        float cosSum = glm::cos(particles[i].angle);

        unsigned int homeCell = grid.hash(particles[i]);

        // homeCell is include in neighbors vector
        grid.getCellsAtLevel(homeCell, 0, i);
        std::vector<unsigned int> neighborCells = grid.outCells;
        grid.getCellsAtLevel(homeCell, 1, i);
        neighborCells.insert(neighborCells.end(), grid.outCells.begin(), grid.outCells.end());

        for (unsigned int cellID : neighborCells)
        {
            unsigned int start = grid.cellOffsets[cellID    ];
            unsigned int end   = grid.cellOffsets[cellID + 1];

            for (unsigned int k = start; k < end; ++k)
            {
                unsigned int j = grid.gridParticles[k];
                if (j == i) continue;

                glm::vec2 delta = particles[i].position - particles[j].position;
                // account for periodic BCs
                if (delta.x > x_max / 2.0f) { delta.x -= x_max; }
                if (delta.x < -x_max / 2.0f) { delta.x += x_max; }
                if (delta.y > y_max / 2.0f) { delta.y -= y_max; }
                if (delta.y < -y_max / 2.0f) { delta.y += y_max; }

                float distance = glm::length(delta);

                if (distance < senseRadius)
                {
                    float weight = 1.0f;
                    // float weight = (distance < 0.1f) ? 1.0f / 0.1f : 1.0f / distance;

                    sinSum += glm::sin(particles[j].angle) * weight;
                    cosSum += glm::cos(particles[j].angle) * weight;
                }
            }
        }
        
        angles[i] = glm::atan(sinSum, cosSum) + noiseScale * PI * (2.0f * dist(rng) - 1.0f);
    }

    return angles;
}
