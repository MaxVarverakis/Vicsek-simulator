#include "Utilities.hpp"

#include "../Swarm/Swarm.hpp"


void Utilities::singleColumnData(const std::string& filename, const std::vector<double>& data)
{
    std::ofstream outFile(filename + ".txt");
    checkFileOpen(outFile);

    for (unsigned int i = 0; i < data.size(); ++i)
    {
        outFile << data[i] << '\n';
    }

    outFile.close();
}

void Utilities::addLine(const std::string& filename, const std::vector<double>& data)
{
    std::ofstream outFile(filename + ".txt", std::ios::app);
    checkFileOpen(outFile);

    for (unsigned int i = 0; i < data.size(); ++i)
    {
        outFile << data[i];
        
        if (i < data.size() - 1) { outFile << ','; }
    }
    outFile << '\n';

    outFile.close();
}

void Utilities::parallelSims(float cellSize, float width, float height, float scaleFactor, uint32_t seed, float scaleNoise, unsigned int neighborCount, float v, unsigned int numParticles, const float dt)
{
    unsigned int terminationCount = 300;

    Swarm swarm(cellSize, width, height, scaleFactor, seed, scaleNoise, neighborCount, v, numParticles, 1);
    
    while (true)
    {
        swarm.update(dt);
        scaleNoise = std::max<float>(scaleNoise - dt / 64.0f, 0.0f);
        swarm.noiseScale = scaleNoise;
        
        if (scaleNoise<= 0.0f)
        {
            --terminationCount;
        }
        
        // func wrapper for "write vals to file"
        // ************************************
        // MAKE SURE TO CREATE DIRECTORY FIRST!
        // ************************************
        Utilities::addLine("/Users/max/UCLA/Research/Codes/Vicsek/data/NNSweep/order_param_NN_" + std::to_string(neighborCount), scaleNoise, swarm.order_param);
        
        if (terminationCount <= 0) { break; }
    }
}

glm::vec4 Utilities::bwr_angle_to_color(float angle)
{
    // red  = + pi / 2
    // blue = - pi / 2
    
    float r = std::clamp<float>(2.0f * fabsf(angle / PI + 0.5f), 0.0f, 1.0f);
    float g = 2.0f * fabsf(fabsf(angle) / PI - 0.5f);
    float b = std::clamp<float>(2.0f * fabsf(angle / PI - 0.5f), 0.0f, 1.0f);

    return glm::vec4(r, g, b, 1.0f);
}

glm::vec4 Utilities::cycle_angle_to_color(float angle)
{
    float r = glm::sin(angle) * 0.5f + 0.5f;
    float g = glm::sin(angle + 2.0f * PI / 3.0f) * 0.5f + 0.5f;
    float b = glm::sin(angle + 4.0f * PI / 3.0f) * 0.5f + 0.5f;

    return glm::vec4(r, g, b, 1.0f);
}
