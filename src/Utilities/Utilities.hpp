#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "../Particle/Particle.hpp"

namespace Utilities
{
    void singleColumnData(const std::string& filename, const std::vector<double>& data);
    void addLine(const std::string& filename, const std::vector<double>& data);

    template <typename FileStream>
    void checkFileOpen(const FileStream& file)
    {
        if (!file.is_open()) {
            throw std::ios_base::failure("Failed to open file!");
        }
    };

    
    template<typename... Args>
    void addLine(const std::string& filename, const Args&... args)
    {
        std::ofstream outFile(filename + ".txt", std::ios::app);
        checkFileOpen(outFile);

        bool first = true;
        ((outFile << (first ? "" : ",") << args, first = false), ...);
        outFile << '\n';

        outFile.close();
    }

    void parallelSims(float width, float height, float scaleFactor, uint32_t seed, float scaleNoise, unsigned int neighborCount, float v, unsigned int numParticles, const float dt);
    
}
