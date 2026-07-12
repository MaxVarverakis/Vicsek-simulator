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

#include "../Particle/Particle.hpp"
#include "../SpatialHashGrid/SpatialHashGrid.hpp"

struct Swarm
{
    SpatialHashGrid grid;
    float x_max, y_max;
    float scale;
    uint32_t master_seed;
    std::vector<std::mt19937> rngs;
    std::uniform_real_distribution<float> uniformDist{ 0.0f, 1.0f };
    float noiseScale;

    unsigned int nNeighbors;
    float velocity;


    std::vector<unsigned int> scratch_neighborIds;
    std::vector<float> scratch_distances;

    std::vector<float> targetAngles;
    std::vector<glm::vec2> positions, headings;
    unsigned int numThreads;
    
    float order_param;
    bool colors_bool { false };
    std::vector<glm::vec4> colors;

    Swarm(float cellSize, float width, float height, float scaleFactor, uint32_t seed, float scaleNoise, unsigned int neighborCount, float v, int num_threads)
        : grid(cellSize, width, height, static_cast<unsigned int>(num_threads))
        , x_max { cellSize * grid.nX }
        , y_max { cellSize * grid.nY }
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
        , scale { scaleFactor }
        , master_seed { seed }
        , noiseScale { scaleNoise }
        , nNeighbors { neighborCount }
        , velocity { v }
        , positions { pos }
        , headings { directions }
        , numThreads { static_cast<unsigned int>(num_threads) }
    {
        initThreadData(num_threads);
    }
    
    Swarm(float cellSize, float width, float height, float scaleFactor, uint32_t seed, float scaleNoise, unsigned int neighborCount, float v, unsigned int numParticles, int num_threads)
        : grid(cellSize, width, height, static_cast<unsigned int>(num_threads))
        , x_max { cellSize * grid.nX }
        , y_max { cellSize * grid.nY }
        , scale { scaleFactor }
        , master_seed { seed }
        , noiseScale { scaleNoise }
        , nNeighbors { neighborCount }
        , velocity { v }
        , numThreads { static_cast<unsigned int>(num_threads) }
    {
        positions.reserve(numParticles);
        headings.reserve(numParticles);
        targetAngles.resize(numParticles);
        
        initThreadData(num_threads);

        if (colors_bool && (colors.size() < positions.size()))
        {
            colors.reserve(positions.size());
        }

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
            colors.emplace_back(Utilities::cycle_angle_to_color(angle));
        }
    }

    void initThreadData(int num_threads)
    {
        omp_set_num_threads(num_threads);
        scratch_neighborIds.resize(nNeighbors * numThreads, 0);
        scratch_distances.resize(nNeighbors * numThreads, std::numeric_limits<float>::infinity());

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

    void nsSense(unsigned int pID);
    
    void sense(unsigned int pID);
    
    void tradSense(unsigned int pID);

    void updateParticle(unsigned int pID, float dt)
    {
        // I'm a little skeptical on doing this rather than just 
        // setting a max angle change outright
        static const float max_turn_speed = 2.0f * PI;

        // prevent instant 180s
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

        if (colors_bool) { colors[pID] = Utilities::cycle_angle_to_color(angle); }
    }

    void update(float dt)
    {
        if (colors_bool && (colors.size() < positions.size()))
        {
            colors.resize(positions.size(), glm::vec4(1.0f));
        }

        float v_hat_x = 0.0f;
        float v_hat_y = 0.0f;

        // build the sorted gridParticles of SHG before sensing!    
        grid.build(positions);
    
        #pragma omp parallel for schedule(static)
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            sense(i);
            // tradSense(i);
        }

        #pragma omp parallel for schedule(static) reduction(+:v_hat_x,v_hat_y)
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            updateParticle(i, dt);

            v_hat_x += headings[i].x;
            v_hat_y += headings[i].y;
        }

        order_param = glm::length(glm::vec2(v_hat_x, v_hat_y)) / static_cast<float>(positions.size());
    }
};
