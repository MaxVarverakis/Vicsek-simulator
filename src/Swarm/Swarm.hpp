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
    float noiseScale;

    unsigned int nNeighbors;
    float velocity;

    std::vector<glm::vec2> reordered_positions;
    std::vector<glm::vec2> reordered_headings;
    std::vector<uint32_t> reordered_particleIDs;


    std::vector<float> targetAngles;
    std::vector<glm::vec2> positions, headings;
    std::vector<uint32_t> particleIDs;
    
    unsigned int numThreads;

    uint64_t currentFrame;
    
    bool debugBool;
    unsigned int debugSelectedID = 0;
    std::vector<Neighbor> debugNeighbors;
    
    float order_param;

    Swarm(float cellSize, float width, float height, float scaleFactor, uint32_t seed, float scaleNoise, unsigned int neighborCount, float v, int num_threads, uint64_t frame = 0, bool debug = 0)
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
        , currentFrame { frame }
        , debugBool { debug }
    {
        initThreadData(num_threads);
    };
    
    Swarm(float cellSize, float width, float height, float scaleFactor, uint32_t seed, float scaleNoise, unsigned int neighborCount, float v, std::vector<glm::vec2> pos, std::vector<glm::vec2> directions, int num_threads, uint64_t frame = 0, bool debug = 0)
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
        , currentFrame { frame }
        , debugBool { debug }
    {
        reordered_positions.resize(positions.size());
        reordered_headings.resize(positions.size());
        reordered_particleIDs.resize(positions.size());
        particleIDs.resize(positions.size());
        targetAngles.resize(positions.size());
        debugNeighbors.resize(neighborCount, { std::numeric_limits<float>::infinity(), 0 });
        initThreadData(num_threads);

        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            particleIDs[i] = i;
        }
    }
    
    Swarm(float cellSize, float width, float height, float scaleFactor, uint32_t seed, float scaleNoise, unsigned int neighborCount, float v, unsigned int numParticles, int num_threads, uint64_t frame = 0, bool debug = 0)
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
        , currentFrame { frame }
        , debugBool { debug }
    {
        reordered_positions.resize(numParticles);
        reordered_headings.resize(numParticles);
        reordered_particleIDs.resize(numParticles);
        positions.reserve(numParticles);
        headings.reserve(numParticles);
        particleIDs.resize(numParticles);
        targetAngles.resize(numParticles);
        debugNeighbors.resize(neighborCount, { std::numeric_limits<float>::infinity(), 0 });
        
        initThreadData(num_threads);

        // generate random particles
        for (unsigned int i = 0; i < numParticles; ++i)
        {
            particleIDs[i] = i;

            float angle = PI * deterministicRNG(i, 0, 0);
            positions.emplace_back(
                glm::vec2(x_max * 0.5f * (deterministicRNG(i, 0, 1) + 1.0f), y_max * 0.5f * (deterministicRNG(i, 0, 2) + 1.0f))
            );
            headings.emplace_back(
                glm::cos(angle), glm::sin(angle)
            );
        }
    }

    void initThreadData(int num_threads)
    {
        omp_set_num_threads(num_threads);
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
    
    void sense(unsigned int pID, uint32_t frameHash);
    
    void tradSense(unsigned int pID, uint32_t frameHash);

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

        unsigned int nextDebugSelectedID = debugSelectedID;
        const unsigned int targetID = debugSelectedID; // Cache locally
        // reorder positions and headings based on the grid layout
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            unsigned int originalIdx = grid.gridParticles[i];
            reordered_positions[i] = positions[originalIdx];
            reordered_headings[i] = headings[originalIdx];
            reordered_particleIDs[i] = particleIDs[originalIdx];

            if (originalIdx == targetID)
            {
                nextDebugSelectedID = i;
            }
        }
        debugSelectedID = nextDebugSelectedID;

        // Fast pointer swap to apply the changes instantly
        positions.swap(reordered_positions);
        headings.swap(reordered_headings);
        particleIDs.swap(reordered_particleIDs);
    
        uint32_t frameHash = Utilities::hash32(static_cast<uint32_t>(currentFrame) + 0x85ebca6bu); // from MurmurHash3 

        #pragma omp parallel for schedule(guided)
        for (std::size_t i = 0; i < static_cast<std::size_t>(positions.size()); ++i)
        {
            unsigned int idx = static_cast<unsigned int>(i);
            sense(idx, frameHash);
            // tradSense(idx, frameHash);
        }

        #pragma omp parallel for schedule(static, 256) reduction(+:v_hat_x,v_hat_y)
        for (std::size_t i = 0; i < static_cast<std::size_t>(positions.size()); ++i)
        {
            unsigned int idx = static_cast<unsigned int>(i);
            updateParticle(idx, dt);

            v_hat_x += headings[idx].x;
            v_hat_y += headings[idx].y;
        }

        order_param = glm::length(glm::vec2(v_hat_x, v_hat_y)) / static_cast<float>(positions.size());

        currentFrame++;
    }

    void findNeighborsInCell(unsigned int cellID, unsigned int pID, const glm::vec2& position, Neighbor* buffer, bool wrap)
    {
        unsigned int start = grid.cellOffsets[cellID];
        unsigned int end   = grid.cellOffsets[cellID + 1];
    
        for (unsigned int k = start; k < end; ++k)
        {
            if (pID == k) continue;

            glm::vec2 delta = position - positions[k];
            if (wrap) shortestDistance(delta);

            float maxDistSq = buffer[0].distanceSq;

            // early exit if too far away in x-direction
            float dx2 = delta.x * delta.x;
            if (dx2 >=  maxDistSq) continue;

            float d2 = dx2 + (delta.y * delta.y);
            if (d2 >=  maxDistSq) continue;
            
            std::pop_heap(buffer, buffer + nNeighbors);
            buffer[nNeighbors - 1] = { d2, k };
            std::push_heap(buffer, buffer + nNeighbors);
        }
    }

    inline float deterministicRNG(uint32_t pID, uint32_t frameHash, uint32_t sequence = 0) 
    {
        // mix the inputs to prevent sequential pID correlations
        // fibonacci hash the particle IDs
        uint32_t state = master_seed ^ frameHash ^ Utilities::hash32(pID + sequence * 0x9e3779b9u);
        uint32_t h = Utilities::hash32(state);
        
        return (static_cast<float>(h) / 4294967295.0f) * 2.0f - 1.0f;
    }
};
