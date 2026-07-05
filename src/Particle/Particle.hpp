#pragma once

#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

static const float PI { std::acosf(-1.0f) };

struct Particle
{
    glm::vec2 position;
    float velocity, angle; // angle in radians

    Particle(glm::vec2 pos, float vel, float theta = 0.0f)
        : position { pos }, velocity { vel }, angle { theta } {};

    void update(float dt, float x_max, float y_max, float v)
    {
        // make sure |v| is updated to current global setting
        velocity = v;

        angle = std::fmod(angle, 2.0f * PI);
        // shift to [-pi, pi]
        while (angle < -PI) { angle += 2.0f * PI; }
        while (angle > PI) { angle -= 2.0f * PI; }

        position += velocity * glm::vec2(glm::cos(angle), glm::sin(angle)) * dt;
        applyPeriodicBC(x_max, y_max);
    }

    void applyPeriodicBC(float x_max, float y_max)
    {
        while (position.x > x_max) { position.x -= x_max; }
        while (position.x < 0.0f) { position.x += x_max; }
        
        while (position.y > y_max) { position.y -= y_max; }
        while (position.y < 0.0f) { position.y += y_max; }
    }
};

struct Swarm
{
    float x_max, y_max;
    float scale;
    uint32_t master_seed;
    std::mt19937 rng;
    std::uniform_real_distribution<float> dist{ 0.0f, 1.0f };
    float noiseScale;

    // perhaps having separate position and angle vectors rather than particle vector would be faster
    unsigned int nNeighbors;
    float velocity;
    std::vector<Particle> particles;
    std::vector<glm::mat4> modelMatrices;

    float order_param;

    void print()
    {
        for (Particle& particle : particles)
        {
            std::cout << "x" << '\t' << particle.position.x << '\n';
            std::cout << "y" << '\t' << particle.position.y << '\n';
            std::cout << "a" << '\t' << particle.angle << '\n';
            std::cout << '\n';
        }
    }

    Swarm(float width, float height, float scaleFactor, uint32_t seed, float scaleNoise, unsigned int neighborCount, float v)
        : x_max { width }
        , y_max { height }
        , scale { scaleFactor }
        , master_seed { seed }
        , rng(seed)
        , noiseScale { scaleNoise }
        , nNeighbors { neighborCount }
        , velocity { v } {};
    
    Swarm(float width, float height, float scaleFactor, uint32_t seed, float scaleNoise, unsigned int neighborCount, float v, std::vector<Particle> pts)
        : x_max { width }
        , y_max { height }
        , scale { scaleFactor }
        , master_seed { seed }
        , rng(seed)
        , noiseScale { scaleNoise }
        , nNeighbors { neighborCount }
        , velocity { v }
        , particles { pts }
        { generateModelMatrices(); }
    
    Swarm(float width, float height, float scaleFactor, uint32_t seed, float scaleNoise, unsigned int neighborCount, float v, unsigned int numParticles)
        : x_max { width }
        , y_max { height }
        , scale { scaleFactor }
        , master_seed { seed }
        , rng(seed)
        , noiseScale { scaleNoise }
        , nNeighbors { neighborCount }
        , velocity { v }
    {
        particles.reserve(numParticles);

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
        modelMatrices.emplace_back(generateModelMatrix(particle));
    }
    
    std::vector<float> sense();
    
    std::vector<float> tradSense();
    
    void updateParticles(float dt)
    {
        glm::vec2 v_hat_sum(0.0f);

        // I'm a little skeptical on doing this rather than just 
        // setting a max angle change outright
        const float max_turn_speed = 2.0f * PI;

        std::vector<float> angles { sense() };

        for (unsigned int i = 0; i < particles.size(); ++i)
        {
            Particle& particle { particles[i] };

            // prevent instant 180s
            float angleChange = angles[i] - particle.angle;
            while (angleChange < -PI) { angleChange += 2.0f * PI; }
            while (angleChange > PI) { angleChange -= 2.0f * PI; }
            float max_angle_change = max_turn_speed * dt;
            particle.angle += glm::clamp(angleChange, -max_angle_change, max_angle_change);

            // enable instant 180s
            // particle.angle = angles[i];
            
            particle.update(dt, x_max, y_max, velocity);
            modelMatrices[i] = generateModelMatrix(particle);

            v_hat_sum += glm::vec2(glm::cos(particle.angle), glm::sin(particle.angle));
        }

        order_param = glm::length(v_hat_sum) / static_cast<float>(particles.size());
    }

    glm::mat4 generateModelMatrix(Particle& particle)
    {
        glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(particle.position, 0.0f));
        // render-space 0 degrees is vertical (+yhat direction) as opposed to +xhat direction
        modelMatrix = glm::rotate(modelMatrix, particle.angle - glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

        return glm::scale(modelMatrix, glm::vec3(scale, scale, 1.0f));
    }

    void generateModelMatrices()
    {
        std::vector<glm::mat4> matrices;
        matrices.reserve(particles.size());

        for (Particle& particle : particles)
        {
            matrices.emplace_back(generateModelMatrix(particle));
        }
    }

};
