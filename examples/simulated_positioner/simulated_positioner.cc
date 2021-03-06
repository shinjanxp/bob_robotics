// BoB robotics includes
#include "common/pose.h"
#include "common/sfml_world.h"
#include "robots/robot_positioner.h"
#include "robots/simulated_tank.h"

// Third-party includes
#include "third_party/units.h"

// Standard C++ includes
#include <chrono>
#include <thread>

using namespace BoBRobotics;
using namespace units::length;
using namespace units::velocity;
using namespace units::angular_velocity;
using namespace units::time;
using namespace units::literals;
using namespace units::angle;
using namespace std::literals;

int
main()
{
    Robots::SimulatedTank<> robot(0.3_mps, 104_mm);
    SFMLWorld<> display;
    auto car = display.createCarAgent();

    // A circle to show where the goal is
    sf::CircleShape goalCircle(10);
    goalCircle.setFillColor(sf::Color::Blue);
    goalCircle.setOrigin(10, 10);
    goalCircle.setPosition(SFMLWorld<>::WindowWidth / 2, SFMLWorld<>::WindowHeight / 2);

    constexpr meter_t stoppingDistance = 5_cm;      // if the robot's distance from goal < stopping dist, robot stops
    constexpr radian_t allowedHeadingError = 2_deg; // the amount of error allowed in the final heading
    constexpr double k1 = 1.51;                     // curveness of the path to the goal
    constexpr double k2 = 4.4;                      // speed of turning on the curves
    constexpr double alpha = 1.03;                  // causes more sharply peaked curves
    constexpr double beta = 0.02;                   // causes to drop velocity if 'k'(curveness) increases

    // construct the positioner
    Robots::RobotPositioner robp(
            stoppingDistance,
            allowedHeadingError,
            k1,
            k2,
            alpha,
            beta);

    bool runPositioner = false;
    bool reachedGoalAnnounced = false;
    while (display.isOpen()) {
        // Get the robot's current pose
        const auto &pose = robot.getPose();

        // Run GUI events
        car.setPose(pose);
        sf::Event event = display.updateAndDrive(robot, goalCircle, car);

        // Spacebar toggles whether positioner is running
        if (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::Space) {
            runPositioner = !runPositioner;
            robot.stopMoving();
        }

        if (runPositioner) {
            if (display.mouseClicked()) {
                // Set a new goal position if user clicks in the window
                const auto mousePosition = display.mouseClickPosition();

                // Set the goal to this position
                robp.setGoalPose({ mousePosition.x(), mousePosition.y(), 15_deg });

                goalCircle.setPosition(display.vectorToPixel(mousePosition));
            }

            // Update course and drive robot
            robp.updateMotors(robot, pose);

            // Check if the robot is within threshold distance and bearing of goal
            if (robp.reachedGoal()) {
                if (!reachedGoalAnnounced) {
                    std::cout << "Reached goal" << std::endl;
                    robot.stopMoving();
                    reachedGoalAnnounced = true;
                }
            } else {
                reachedGoalAnnounced = false;
            }
        }

        // A small delay, so we don't eat all the CPU
        std::this_thread::sleep_for(2ms);
    }
}
