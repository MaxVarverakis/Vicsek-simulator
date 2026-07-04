#include "Utilities.hpp"

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

void Utilities::parallelSims(float width, float height, float scaleFactor, uint32_t seed, float scaleNoise, unsigned int neighborCount, float v, unsigned int numParticles, const float dt)
{
    unsigned int terminationCount = 300;

    Swarm swarm(width, height, scaleFactor, seed, scaleNoise, neighborCount, v, numParticles);
    
    while (true)
    {
        swarm.updateParticles(dt);
        scaleNoise = std::max<float>(scaleNoise - dt / 64.0f, 0.0f);
        swarm.changeNoise(scaleNoise);
        
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
