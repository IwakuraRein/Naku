#include "utils/engine.hpp"
#include "io/scene.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <chrono>

static constexpr char NAKU_VERSION[] = { "0.01" };

int main(int argc, char* argv[]) {
    std::string scenePath{ "res/scene/Box" };
    if (argc >= 2) {
        scenePath = argv[1];
    }
    std::string wName{"Naku"};
    wName = wName;
    naku::Engine engine(1600, 900, wName, 1.25f);
    //std::string scenePath{ "res/scene/The Modern Living Room" };
    naku::Scene scene{ engine, scenePath };
#ifndef NDEBUG
    scene.echo = true;
#endif // !NDEBUG

    try {
        std::cout << "Loading scene: " << scenePath << std::endl;
        auto t_start = std::chrono::high_resolution_clock::now();
        scene.load();
        auto t_end = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(t_end - t_start).count();
        std::cout << "\tVertices: " << scene.vertexCount() << std::endl;
        std::cout << "\tTriangles: " << scene.faceCount() << std::endl;
        std::cout.setf(std::ios::fixed);
        std::cout << "\tScene loaded in " << std::setprecision(3) << deltaTime << " seconds." << std::endl;
        engine.pWindow->setWindowName(wName + " - " + scenePath);

        while (true) {
            int i = engine.run();
            if (i == EXIT_FAILURE) return EXIT_FAILURE;
            else if (i == EXIT_SUCCESS) return EXIT_SUCCESS;
        }
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

