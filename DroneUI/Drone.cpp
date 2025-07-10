#include "Drone.h"
#include <cstdlib>   // for rand()

Drone::Drone(double startX, double startY, double startSpeed, double startOrientation)
    : x(startX), y(startY), speed(startSpeed), orientation(startOrientation)
{
}

void Drone::StartSimulation() {
    // ensure M_PI is available
#ifndef M_PI
    static constexpr double M_PI = 3.14159265358979323846;
#endif

    double radians = orientation * (M_PI / 180.0);
    x += speed * std::cos(radians);
    y += speed * std::sin(radians);
    orientation += (std::rand() % 7 - 3);
    if (orientation >= 360.0) orientation -= 360.0;
    if (orientation < 0.0)   orientation += 360.0;
}

void Drone::SimulateRandomMovement(double durationSeconds, double timeStepSeconds) {
    using clock = std::chrono::steady_clock;
    auto start = clock::now();
    auto step = std::chrono::duration<double>(timeStepSeconds);

    while (std::chrono::duration<double>(clock::now() - start).count() < durationSeconds) {
        StartSimulation();
        std::this_thread::sleep_for(step);
    }
}
