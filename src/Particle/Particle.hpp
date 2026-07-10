#pragma once

#include <glm/glm.hpp>

#include "../Utilities/Utilities.hpp"

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

        // shift to [-pi, pi]
        angle = std::remainderf(angle, 2.0f * PI);

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

    void printInfo()
    {
        std::cout << "x" << '\t' << position.x << '\n';
        std::cout << "y" << '\t' << position.y << '\n';
        std::cout << "a" << '\t' << angle << '\n';
        std::cout << '\n';
    }
};
