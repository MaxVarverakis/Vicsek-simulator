#pragma once

#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <omp.h>

#include "../Utilities/Utilities.hpp"
#include "../SpatialHashGrid/SpatialHashGrid.hpp"

struct Neighbor
{
    float distanceSq;
    unsigned int id;

    bool operator<(const Neighbor& other) const
    {
        return distanceSq < other.distanceSq;
    }
};

struct Swarm
{
    SpatialHashGrid grid;
    float x_max, y_max;
    float half_x, half_y;
    float scale;
    uint32_t master_seed;
    std::vector<std::mt19937> rngs;
    std::uniform_real_distribution<float> uniformDist{ 0.0f, 1.0f };
    float noiseScale;

    unsigned int nNeighbors;
    float velocity;

    std::vector<glm::vec2> reordered_positions;
    std::vector<glm::vec2> reordered_headings;

    std::vector<float> targetAngles;
    std::vector<glm::vec2> positions, headings;
    unsigned int numThreads;
    
    float order_param;

    Swarm(float cellSize, float width, float height, float scaleFactor, uint32_t seed, float scaleNoise, unsigned int neighborCount, float v, int num_threads)
        : grid(cellSize, width, height, static_cast<unsigned int>(num_threads))
        , x_max { cellSize * grid.nX }
        , y_max { cellSize * grid.nY }
        , half_x { x_max * 0.5f}
        , half_y { y_max * 0.5f}
        , scale { scaleFactor }
        , master_seed { seed }
        , noiseScale { scaleNoise }
        , nNeighbors { neighborCount }
        , velocity { v }
        , numThreads { static_cast<unsigned int>(num_threads) }
    {
        initThreadData(num_threads);
    };
    
    Swarm(float cellSize, float width, float height, float scaleFactor, uint32_t seed, float scaleNoise, unsigned int neighborCount, float v, std::vector<glm::vec2> pos, std::vector<glm::vec2> directions, int num_threads)
        : grid(cellSize, width, height, static_cast<unsigned int>(num_threads))
        , x_max { cellSize * grid.nX }
        , y_max { cellSize * grid.nY }
        , half_x { x_max * 0.5f}
        , half_y { y_max * 0.5f}
        , scale { scaleFactor }
        , master_seed { seed }
        , noiseScale { scaleNoise }
        , nNeighbors { neighborCount }
        , velocity { v }
        , positions { pos }
        , headings { directions }
        , numThreads { static_cast<unsigned int>(num_threads) }
    {
        reordered_positions.resize(positions.size());
        reordered_headings.resize(positions.size());
        initThreadData(num_threads);
    }
    
    Swarm(float cellSize, float width, float height, float scaleFactor, uint32_t seed, float scaleNoise, unsigned int neighborCount, float v, unsigned int numParticles, int num_threads)
        : grid(cellSize, width, height, static_cast<unsigned int>(num_threads))
        , x_max { cellSize * grid.nX }
        , y_max { cellSize * grid.nY }
        , half_x { x_max * 0.5f}
        , half_y { y_max * 0.5f}
        , scale { scaleFactor }
        , master_seed { seed }
        , noiseScale { scaleNoise }
        , nNeighbors { neighborCount }
        , velocity { v }
        , numThreads { static_cast<unsigned int>(num_threads) }
    {
        reordered_positions.resize(numParticles);
        reordered_headings.resize(numParticles);
        positions.reserve(numParticles);
        headings.reserve(numParticles);
        targetAngles.resize(numParticles);
        
        initThreadData(num_threads);

        // generate random particles
        for (unsigned int i = 0; i < numParticles; ++i)
        {
            float angle = PI * (2.0f * uniformDist(rngs[0]) - 1.0f);
            positions.emplace_back(
                glm::vec2(x_max * uniformDist(rngs[0]), y_max * uniformDist(rngs[0]))
            );
            headings.emplace_back(
                glm::cos(angle), glm::sin(angle)
            );
        }
    }

    void initThreadData(int num_threads)
    {
        omp_set_num_threads(num_threads);

        rngs.reserve(numThreads);
        for (unsigned int i = 0; i < numThreads; i++)
        {
            rngs.emplace_back(master_seed + i);
        }
    }

    void applyPeriodicBC(glm::vec2& position)
    {
        if (position.x >= x_max) { position.x -= x_max; }
        else if (position.x < 0.0f) { position.x += x_max; }

        if (position.y >= y_max) { position.y -= y_max; }
        else if (position.y < 0.0f) { position.y += y_max; }
    }

    void shortestDistance(glm::vec2& delta)
    {
        if (delta.x > half_x)       { delta.x -= x_max; }
        else if (delta.x < -half_x) { delta.x += x_max; }

        if (delta.y > half_y)       { delta.y -= y_max; }
        else if (delta.y < -half_y) { delta.y += y_max; }
    }
    
    void sense(unsigned int pID);
    
    void tradSense(unsigned int pID);

    void updateParticle(unsigned int pID, float dt)
    {
        // prevent instant 180s
        // I'm a little skeptical on doing this rather than just 
        // setting a max angle change outright
        static const float max_turn_speed = 2.0f * PI;
        float angle = glm::atan(headings[pID].y, headings[pID].x);
        float angleChange = targetAngles[pID] - angle;
        while (angleChange < -PI) { angleChange += 2.0f * PI; }
        while (angleChange > PI) { angleChange -= 2.0f * PI; }
        float max_angle_change = max_turn_speed * dt;
        angle += glm::clamp(angleChange, -max_angle_change, max_angle_change);
        headings[pID] = glm::vec2(glm::cos(angle), glm::sin(angle));

        // enable instant 180s
        // headings[pID] = glm::vec2(glm::cos(targetAngles[pID]), glm::sin(targetAngles[pID]));
        
        positions[pID] += velocity * headings[pID] * dt;
        applyPeriodicBC(positions[pID]);
    }

    void update(float dt)
    {
        float v_hat_x = 0.0f;
        float v_hat_y = 0.0f;

        // build the sorted gridParticles of SHG before sensing!    
        grid.build(positions);

        // reorder positions and headings based on the grid layout
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            unsigned int originalIdx = grid.gridParticles[i];
            reordered_positions[i] = positions[originalIdx];
            reordered_headings[i] = headings[originalIdx];
        }
        // Fast pointer swap to apply the changes instantly
        positions.swap(reordered_positions);
        headings.swap(reordered_headings);
    
        #pragma omp parallel for schedule(dynamic, 32)
        for (std::size_t i = 0; i < static_cast<std::size_t>(positions.size()); ++i)
        {
            unsigned int idx = static_cast<unsigned int>(i);
            sense(idx);
            // tradSense(idx);
        }

        #pragma omp parallel for schedule(static, 64) reduction(+:v_hat_x,v_hat_y)
        for (std::size_t i = 0; i < static_cast<std::size_t>(positions.size()); ++i)
        {
            unsigned int idx = static_cast<unsigned int>(i);
            updateParticle(idx, dt);

            v_hat_x += headings[idx].x;
            v_hat_y += headings[idx].y;
        }

        order_param = glm::length(glm::vec2(v_hat_x, v_hat_y)) / static_cast<float>(positions.size());
    }

    void findNeighborsInCell(unsigned int cellID, unsigned int pID, const glm::vec2& position, Neighbor* buffer)
    {
        unsigned int start = grid.cellOffsets[cellID];
        unsigned int end   = grid.cellOffsets[cellID + 1];
    
        for (unsigned int k = start; k < end; ++k)
        {
            if (pID == k) continue;

            glm::vec2 delta = position - positions[k];
            shortestDistance(delta);
            float d2 = glm::dot(delta, delta);

            if (d2 >= buffer[0].distanceSq) continue;
            
            std::pop_heap(buffer, buffer + nNeighbors);
            buffer[nNeighbors - 1] = { d2, k };
            std::push_heap(buffer, buffer + nNeighbors);
        }
    }
};
