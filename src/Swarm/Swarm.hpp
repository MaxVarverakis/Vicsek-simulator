#pragma once

#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Particle/Particle.hpp"
#include "../SpatialHashGrid/SpatialHashGrid.hpp"

struct Swarm
{
    SpatialHashGrid grid;
    float x_max, y_max;
    float scale;
    uint32_t master_seed;
    std::mt19937 rng;
    std::uniform_real_distribution<float> dist{ 0.0f, 1.0f };
    float noiseScale;

    // perhaps having separate position and angle vectors rather than particle vector would be faster
    unsigned int nNeighbors;
    float velocity;

    std::vector<unsigned int> scratch_neighborIds;
    std::vector<float> scratch_distances;
    std::vector<float> targetAngles;
    std::vector<Particle> particles;
    std::vector<glm::mat4> modelMatrices;
    std::vector<glm::vec4> renderData; // (x, y, heading.x, heading.y)
    
    float order_param;
    bool colors_bool;
    std::vector<glm::vec4> colors;

    void printParticleInfo()
    {
        for (unsigned int i = 0; i < particles.size(); ++i)
        {
            Particle& particle = particles[i];
            std::cout << "idx" << '\t' << i << '\n';
            particle.printInfo();
            std::cout << '\n';
        }
    }

    Swarm(float cellSize, float width, float height, float scaleFactor, uint32_t seed, float scaleNoise, unsigned int neighborCount, float v)
        : grid(cellSize, width, height)
        , x_max { cellSize * grid.nX }
        , y_max { cellSize * grid.nY }
        , scale { scaleFactor }
        , master_seed { seed }
        , rng(seed)
        , noiseScale { scaleNoise }
        , nNeighbors { neighborCount }
        , velocity { v }
    {
        scratch_neighborIds.resize(nNeighbors, 0);
        scratch_distances.resize(nNeighbors, std::numeric_limits<float>::infinity());
    };
    
    Swarm(float cellSize, float width, float height, float scaleFactor, uint32_t seed, float scaleNoise, unsigned int neighborCount, float v, std::vector<Particle> pts)
        : grid(cellSize, width, height)
        , x_max { cellSize * grid.nX }
        , y_max { cellSize * grid.nY }
        , scale { scaleFactor }
        , master_seed { seed }
        , rng(seed)
        , noiseScale { scaleNoise }
        , nNeighbors { neighborCount }
        , velocity { v }
        , particles { pts }
    {
        generateRenderData();
        // generateModelMatrices();
        scratch_neighborIds.resize(nNeighbors, 0);
        scratch_distances.resize(nNeighbors, std::numeric_limits<float>::infinity());
    }
    
    Swarm(float cellSize, float width, float height, float scaleFactor, uint32_t seed, float scaleNoise, unsigned int neighborCount, float v, unsigned int numParticles)
        : grid(cellSize, width, height)
        , x_max { cellSize * grid.nX }
        , y_max { cellSize * grid.nY }
        , scale { scaleFactor }
        , master_seed { seed }
        , rng(seed)
        , noiseScale { scaleNoise }
        , nNeighbors { neighborCount }
        , velocity { v }
    {
        particles.reserve(numParticles);
        targetAngles.resize(numParticles);
        // modelMatrices.reserve(numParticles);
        renderData.reserve(numParticles);
        scratch_neighborIds.resize(nNeighbors, 0);
        scratch_distances.resize(nNeighbors, std::numeric_limits<float>::infinity());

        if (colors_bool && (colors.size() < particles.size()))
        {
            colors.reserve(particles.size());
        }

        // generate random particles
        for (unsigned int i = 0; i < numParticles; ++i)
        {
            Particle particle(
                glm::vec2(x_max * dist(rng), y_max * dist(rng)),
                velocity,
                PI * (2.0f * dist(rng) - 1.0f)
            );
            
            addParticle(particle);
        }
    }

    void addParticle(Particle& particle)
    {
        particles.emplace_back(particle);
        // modelMatrices.emplace_back(generateModelMatrix(particle));
        renderData.emplace_back(generateParticleRenderData(particle));
        colors.emplace_back(Utilities::cycle_angle_to_color(particle.angle()));
    }
    
    void nSquaredSense();
    
    void sense();
    
    void tradSense();
    
    void updateParticles(float dt)
    {
        if (colors_bool && (colors.size() < particles.size()))
        {
            colors.resize(particles.size(), glm::vec4(1.0f));
        }

        glm::vec2 v_hat_sum(0.0f);

        // I'm a little skeptical on doing this rather than just 
        // setting a max angle change outright
        const float max_turn_speed = 2.0f * PI;

        // build the sorted gridParticles of SHG before sensing!
        grid.build(particles);
        sense();
        // tradSense();

        for (unsigned int i = 0; i < particles.size(); ++i)
        {
            Particle& particle { particles[i] };

            // prevent instant 180s


            float angle = particle.angle();
            float angleChange = targetAngles[i] - angle;
            while (angleChange < -PI) { angleChange += 2.0f * PI; }
            while (angleChange > PI) { angleChange -= 2.0f * PI; }
            float max_angle_change = max_turn_speed * dt;
            angle += glm::clamp(angleChange, -max_angle_change, max_angle_change);
            particle.heading = glm::vec2(glm::cos(angle), glm::sin(angle));

            // enable instant 180s
            // particle.heading = glm::vec2(glm::cos(targetAngles[i]), glm::sin(targetAngles[i]));
            
            particle.update(dt, x_max, y_max, velocity);
            // modelMatrices[i] = generateModelMatrix(particle);
            renderData[i] = generateParticleRenderData(particle);

            if (colors_bool) { colors[i] = Utilities::cycle_angle_to_color(particle.angle()); }

            v_hat_sum += particle.heading;
        }

        order_param = glm::length(v_hat_sum) / static_cast<float>(particles.size());
    }

    glm::mat4 generateModelMatrix(Particle& particle)
    {
        glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(particle.position, 0.0f));
        // render-space 0 degrees is vertical (+yhat direction) as opposed to +xhat direction
        modelMatrix = glm::rotate(modelMatrix, particle.angle() - glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

        return glm::scale(modelMatrix, glm::vec3(scale, scale, 1.0f));
    }
    
    glm::vec4 generateParticleRenderData(Particle& particle)
    {
        return glm::vec4(particle.position, particle.heading);
    }

    void generateModelMatrices()
    {
        modelMatrices.reserve(particles.size());

        for (Particle& particle : particles)
        {
            modelMatrices.emplace_back(generateModelMatrix(particle));
        }
    }
    
    void generateRenderData()
    {
        renderData.reserve(particles.size());

        for (Particle& particle : particles)
        {
            renderData.emplace_back(generateParticleRenderData(particle));
        }
    }

    
};
