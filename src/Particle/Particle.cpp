#include "Particle.hpp"

std::vector<float> Swarm::sense()
{
    std::vector<float> angles(particles.size());

    for (unsigned int i = 0; i < particles.size(); ++i)
    {
        std::vector<unsigned int> neighborIds(nNeighbors);
        std::vector<float> distances(nNeighbors, 9999.0f);

        for (unsigned int j = 0; j < particles.size(); ++j)
        {
            if (j != i)
            {
                glm::vec2 delta = particles[i].position - particles[j].position;
                // account for periodic BCs
                if (delta.x > x_max / 2.0f) { delta.x -= x_max; }
                if (delta.x < -x_max / 2.0f) { delta.x += x_max; }
                if (delta.y > y_max / 2.0f) { delta.y -= y_max; }
                if (delta.y < -y_max / 2.0f) { delta.y += y_max; }

                float distance = glm::length(delta);

                unsigned int j_copy = j;

                for (unsigned int k = 0; k < nNeighbors; ++k)
                {
                    if (distance < distances[k])
                    {
                        // insert and bump
                        unsigned int oldIdx = neighborIds[k];
                        float oldDist = distances[k];

                        neighborIds[k] = j_copy;
                        distances[k] = distance;

                        distance = oldDist;
                        j_copy = oldIdx;
                    }
                }
            }
        }

        // now that we have a list of the k-nearest neighbor indices, we can perform the sensing step
        float sinSum = glm::sin(particles[i].angle);
        float cosSum = glm::cos(particles[i].angle);

        for (unsigned int m = 0; m < nNeighbors; ++m)
        {
            unsigned int neighborIdx = neighborIds[m];
            
            // no weight
            // float weight = 1.0f;
            
            // distance-based alignment weights
            // float weight = (distances[m] < 0.1f) ? 1.0f / 0.1f : 1.0f / distances[m];

            // closeness-based alignment weights
            float weight = 1.0f - static_cast<float>(m / (nNeighbors + 1));
            // float weight = powf(0.5f, static_cast<float>(m));

            sinSum += glm::sin(particles[neighborIdx].angle) * weight;
            cosSum += glm::cos(particles[neighborIdx].angle) * weight;
        }
        // should clamp by a max centripetal acceleration
        angles[i] = glm::atan(sinSum, cosSum) + noiseScale * PI * (2.0f * dist(rng) - 1.0f);
    }

    return angles;
}

std::vector<float> Swarm::tradSense()
{
    const float senseRadius { 50.0f };

    std::vector<float> angles(particles.size());

    for (unsigned int i = 0; i < particles.size(); ++i)
    {
        float sinSum = glm::sin(particles[i].angle);
        float cosSum = glm::cos(particles[i].angle);
        int neighborCount = 0;

        for (unsigned int j = 0; j < particles.size(); ++j)
        {
            if (j != i)
            {
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
                    ++neighborCount;
                }
            }
        }
        if (neighborCount > 0)
        { angles[i] = glm::atan(sinSum, cosSum) + noiseScale * PI * (2.0f * dist(rng) - 1.0f); }
        else
        { angles[i] = particles[i].angle + noiseScale * PI * (2.0f * dist(rng) - 1.0f); }
    }

    return angles;
}
