#pragma once

// BoB robotics includes
#include "../common/assert.h"
#include "../common/circstat.h"
#include "tank.h"

// EV3 library
#include "ev3dev.h"

// Third-party includes
#include "../third_party/units.h"

// Standard C++ includes
#include <utility>

namespace BoBRobotics {
namespace Robots {
//! Control for a Lego Mindstorms tank robot
class EV3
  : public Tank
{
public:
    EV3(const float proportionMaxSpeed = 0.7f,
        const ev3dev::address_type leftMotorPort = ev3dev::OUTPUT_D,
        const ev3dev::address_type rightMotorPort = ev3dev::OUTPUT_A)
      : m_MotorLeft(leftMotorPort)
      , m_MotorRight(rightMotorPort)
      , m_MaxSpeedTachos(proportionMaxSpeed * m_MotorLeft.max_speed())
      , m_TachoCountPerRotation(m_MotorLeft.count_per_rot())
    {}

    virtual ~EV3() override
    {
        stopMoving();
    }

    virtual void stopMoving() override
    {
        m_MotorLeft.stop();
        m_MotorRight.stop();
    }

    virtual void tank(float left, float right) override
    {
        BOB_ASSERT(left >= -1.f && left <= 1.f);
        BOB_ASSERT(right >= -1.f && right <= 1.f);

        m_MotorLeft.set_speed_sp(m_MaxSpeedTachos * left);
        m_MotorRight.set_speed_sp(m_MaxSpeedTachos * right);
        m_MotorLeft.run_forever();
        m_MotorRight.run_forever();
    }

    virtual millimeter_t getRobotWidth() override
    {
        return 12_cm;
    }

    virtual meters_per_second_t getMaximumSpeed() override
    {
        return tachoToSpeed(m_MaxSpeedTachos);
    }

    auto getWheelVelocities() const
    {
        return std::make_pair(tachoToSpeed(m_MotorLeft.speed()),
                              tachoToSpeed(m_MotorRight.speed()));
    }

private:
    ev3dev::large_motor m_MotorLeft, m_MotorRight;
    const int m_MaxSpeedTachos, m_TachoCountPerRotation;

    meters_per_second_t tachoToSpeed(int tachos) const
    {
        constexpr units::length::meter_t wheelRadius = 5.5_cm / 2;
        const double angularVelocity{ 2 * pi() * static_cast<double>(tachos) / static_cast<double>(m_TachoCountPerRotation) };
        return meters_per_second_t{ angularVelocity * wheelRadius.value() };
    }
};
}
}