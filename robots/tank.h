#pragma once

// BoB robotics includes
#include "../common/assert.h"
#include "../hid/joystick.h"
#include "../net/connection.h"
#include "robot.h"

// Standard C includes
#include <cmath>

// Standard C++ includes
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

// third party includes
#include "../third_party/units.h"

namespace BoBRobotics {
namespace Robots {
//----------------------------------------------------------------------------
// BoBRobotics::Robots::Tank
//----------------------------------------------------------------------------
//! Interface for driving wheeled robots with tank steering
class Tank
  : public Robot
{
    using _meter_t = units::length::meter_t;
    using _millimeter_t = units::length::millimeter_t;
    using _meters_per_second_t = units::velocity::meters_per_second_t;
    using _radians_per_second_t = units::angular_velocity::radians_per_second_t;

public:
    virtual void moveForward(float speed) override
    {
        tank(speed, speed);
    }

    virtual void turnOnTheSpot(float clockwiseSpeed) override
    {
        tank(clockwiseSpeed, -clockwiseSpeed);
    }

    virtual void stopMoving() override
    {
        tank(0.f, 0.f);
    }

    void addJoystick(HID::Joystick &joystick, float deadZone = 0.25f)
    {
        joystick.addHandler(
                [this, deadZone](HID::JAxis axis, float value) {
                    return onJoystickEvent(axis, value, deadZone);
                });
    }

    void drive(const HID::Joystick &joystick, float deadZone = 0.25f)
    {
        drive(joystick.getState(HID::JAxis::LeftStickHorizontal),
              joystick.getState(HID::JAxis::LeftStickVertical),
              deadZone);
    }

    virtual void drive(_meters_per_second_t v, _radians_per_second_t omega)
    {
        const _meter_t axisLength = getRobotAxisLength();
        const _meters_per_second_t diff{
            (omega * axisLength / 2).value()
        };
        const _meters_per_second_t vL = v - diff;
        const _meters_per_second_t vR = v + diff;

        const _meters_per_second_t maxSpeed = getMaximumSpeed();
        using namespace units::math;
        const _meters_per_second_t larger = std::max(abs(vL), abs(vR));
        if (larger > maxSpeed) {
            tank(static_cast<float>(vL / larger), static_cast<float>(vR / larger));
        } else {
            tank(vL, vR);
        }
    }

    //! Set the left and right motors to the specified speed
    virtual void tank(float left, float right)
    {
        BOB_ASSERT(left >= -1.f && left <= 1.f);
        BOB_ASSERT(right >= -1.f && right <= 1.f);
        std::cout << "Dummy motor: left: " << left << "; right: " << right
                  << std::endl;
    }

    virtual void tank(_meters_per_second_t left, _meters_per_second_t right)
    {
        const _meters_per_second_t maxSpeed = getMaximumSpeed();
        tank(static_cast<float>(left / maxSpeed), static_cast<float>(right / maxSpeed));
    }

    virtual _millimeter_t getRobotWheelRadius()
    {
        throw std::runtime_error("getRobotWheelRadius() is not implemented for this class");
    }

    virtual _millimeter_t getRobotAxisLength()
    {
        throw std::runtime_error("getRobotAxisLength() is not implemented for this class");
    }

    virtual _meters_per_second_t getMaximumSpeed()
    {
        throw std::runtime_error("getMaximumSpeed() is not implemented for this class");
    }

    virtual _radians_per_second_t getMaximumTurnSpeed()
    {
        // max turn speed = v_max / r
        return _radians_per_second_t{ (getMaximumSpeed() * 2 / static_cast<_meter_t>(getRobotAxisLength())).value() };
    }

    //! Controls the robot with a network stream
    void readFromNetwork(Net::Connection &connection)
    {
        // handle incoming TNK commands
        connection.setCommandHandler("TNK",
            [this](Net::Connection &connection, const Net::Command &command) {
                onCommandReceived(connection, command);
            });

        m_Connection = &connection;
    }

    void stopReadingFromNetwork()
    {
        if (m_Connection) {
            // Ignore TNK commands
            m_Connection->setCommandHandler("TNK", nullptr);
        }
    }

private:
    Net::Connection *m_Connection = nullptr;
    float m_X = 0;
    float m_Y = 0;

    void drive(float x, float y, float deadZone)
    {
        const float pi = 3.141592653589793238462643383279502884f;
        const float halfPi = pi / 2.0f;

        const bool deadX = (fabs(x) < deadZone);
        const bool deadY = (fabs(y) < deadZone);
        if (deadX && deadY) {
            tank(0.0f, 0.0f);
        } else if (deadX) {
            tank(-y, -y);
        } else if (deadY) {
            tank(x, -x);
        } else {
            // If length of joystick vector places it in deadZone, stop motors
            float r = hypot(x, y);
            r = std::min(1.f, r);
            const float theta = atan2(x, -y);
            const float twoTheta = 2.0f * theta;

            // Drive motor
            if (theta >= 0.0f && theta < halfPi) {
                tank(r, r * cos(twoTheta));
            } else if (theta >= halfPi && theta < pi) {
                tank(-r * cos(twoTheta), -r);
            } else if (theta < 0.0f && theta >= -halfPi) {
                tank(r * cos(twoTheta), r);
            } else if (theta < -halfPi && theta >= -pi) {
                tank(-r, -r * cos(twoTheta));
            }
        }
    }

    void onCommandReceived(Net::Connection &, const Net::Command &command)
    {
        // second space separates left and right parameters
        if (command.size() != 3) {
            throw Net::BadCommandError();
        }

        // parse strings to floats
        const float left = stof(command[1]);
        const float right = stof(command[2]);

        // send motor command
        tank(left, right);
    }

    bool onJoystickEvent(HID::JAxis axis, float value, float deadZone)
    {
        // only interested in left joystick
        switch (axis) {
        case HID::JAxis::LeftStickVertical:
            m_Y = value;
            break;
        case HID::JAxis::LeftStickHorizontal:
            m_X = value;
            break;
        default:
            return false;
        }

        // drive robot with joystick
        drive(m_X, m_Y, deadZone);
        return true;
    }
}; // Tank
} // Robots
} // BoBRobotics
