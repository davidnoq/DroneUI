#pragma once
#include <cmath>    // for M_PI
#include <chrono>
#include <thread>

class Drone {
private:
    double x, y;
    double speed;
    double orientation;

public:
    Drone(double startX, double startY, double startSpeed, double startOrientation);

    // one?step move + randomize heading
    void StartSimulation();

    // run StartSimulation() repeatedly
    void SimulateRandomMovement(double durationSeconds, double timeStepSeconds = 0.1);

    double GetX() const { return x; }
    double GetY() const { return y; }
    double GetOrientation() const { return orientation; }
	double GetSpeed() const { return speed; }
};
