#pragma once

#include <glm/glm.hpp>

#include "../Utilities/Utilities.hpp"

struct Particle
{
    glm::vec2 position, heading;
    float velocity; //, angle; // angle in radians
    // float sinAngle, cosAngle;

    Particle(glm::vec2 pos, float vel, float theta = 0.0f)
        : position { pos }
        , heading { glm::cos(theta), glm::sin(theta) }
        , velocity { vel }
        {};

    void update(float dt, float x_max, float y_max, float v)
    {
        // make sure |v| is updated to current global setting
        velocity = v;

        // position += velocity * glm::vec2(cosAngle, sinAngle) * dt;
        position += velocity * heading * dt;
        applyPeriodicBC(x_max, y_max);
    }

    void applyPeriodicBC(float x_max, float y_max)
    {
        while (position.x > x_max) { position.x -= x_max; }
        while (position.x < 0.0f) { position.x += x_max; }
        
        while (position.y > y_max) { position.y -= y_max; }
        while (position.y < 0.0f) { position.y += y_max; }
    }

    float angle()
    {
        return glm::atan(heading.y, heading.x);
    }

    void printInfo()
    {
        std::cout << "x" << '\t' << position.x << '\n';
        std::cout << "y" << '\t' << position.y << '\n';
        std::cout << "nx" << '\t' << heading.x << '\n';
        std::cout << "ny" << '\t' << heading.y << '\n';
        std::cout << '\n';
    }
};
