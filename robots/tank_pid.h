#pragma once

// BoB robotics includes
#include "../common/circstat.h"
#include "../common/fsm.h"
#include "../common/logging.h"
#include "../common/pose.h"
#include "../common/stopwatch.h"
#include "tank.h"

// Third-party includes
#include "../third_party/units.h"

// Standard C includes
#include <cmath>

// Standard C++ includes
#include <algorithm>
#include <iostream>

namespace BoBRobotics {
namespace Robots {

using namespace units::literals;

enum class TankPIDState
{
    Invalid,
    OrientingToGoal,
    DrivingToGoal,
    AtGoal
};

class TankPID
  : public FSM<TankPIDState>::StateHandler
{
    using meter_t = units::length::meter_t;
    using radian_t = units::angle::radian_t;
    using second_t = units::time::second_t;
    using Event = FSM<TankPIDState>::StateHandler::Event;

public:
    TankPID(Tank &robot, float Kp, float Ki, float Kd,
            meter_t distanceTolerance = 5_cm,
            radian_t angleTolerance = 3_deg,
            radian_t startTurningThreshold = 45_deg, float averageSpeed = 0.5f)
      : m_StateMachine(this, TankPIDState::Invalid)
      , m_Robot(robot)
      , m_DistanceTolerance(distanceTolerance)
      , m_AngleTolerance(angleTolerance)
      , m_StartTurningThreshold(startTurningThreshold)
      , m_Kp(Kp)
      , m_Ki(Ki)
      , m_Kd(Kd)
      , m_AverageSpeed(averageSpeed)
    {}

    void start(const Vector2<meter_t> &goal)
    {
        m_Goal = goal;

        /*
         * We start off assuming that the heading is way off and that we need to
         * turn on the spot.
         */
        m_StateMachine.transition(TankPIDState::OrientingToGoal);
    }

    bool driveRobot(const Pose2<meter_t, radian_t> &robotPose)
    {
        m_RobotPose = robotPose;
        const radian_t headingToGoal = units::math::atan2(m_Goal.y() - m_RobotPose.y(),
                                                          m_Goal.x() - m_RobotPose.x());
        m_HeadingOffset = circularDistance(headingToGoal, m_RobotPose.yaw());

        // Get state machine to carry out appropriate action
        m_StateMachine.update();

        // Return true if we're at goal (within m_DistanceTolerance)
        return m_StateMachine.getCurrentState() == TankPIDState::AtGoal;
    }

    virtual bool handleEvent(TankPIDState state, Event event) override
    {
        using namespace units::math;

        if (event == Event::Update && m_Goal.distance2D(m_RobotPose) < m_DistanceTolerance) {
            m_StateMachine.transition(TankPIDState::AtGoal);
            return true;
        }

        switch (state) {
        case TankPIDState::OrientingToGoal:
            if (event == Event::Enter) {
                LOG_DEBUG << "Starting turning";
            } else if (event == Event::Update) {
                /*
                 * If m_HeadingOffset is suitably small, we've finished turning
                 * towards the goal.
                 */
                if (abs(m_HeadingOffset) <= m_AngleTolerance) {
                    // Start driving to the goal in a straight(ish) line
                    m_StateMachine.transition(TankPIDState::DrivingToGoal);
                } else if (m_HeadingOffset < 0_deg) {
                    m_Robot.tank(1.f, -1.f);
                } else {
                    m_Robot.tank(-1.f, 1.f);
                }
            } else { // Exit
                m_Robot.stopMoving();
                LOG_DEBUG << "Stopping turning";
            }
            break;
        case TankPIDState::DrivingToGoal: {
            // Error value used in PID control
            const float error = m_HeadingOffset.value();

            if (event == Event::Enter) {
                // Start timing
                m_Stopwatch.start();
            } else if (event == Event::Update) {
                // If the heading offset is big, then start turning on the spot
                if (abs(m_HeadingOffset) > m_StartTurningThreshold) {
                    m_StateMachine.transition(TankPIDState::OrientingToGoal);
                    return true;
                }

                const float dt = static_cast<second_t>(m_Stopwatch.lap()).value();

                // PID controller for straight(ish) portion of course
                const float p = m_Kp * error;
                const float i = m_Ki * (error + m_LastError * dt);
                const float d = m_Kd * (m_LastError - error) / dt;

                const float differential = p + i + d;

                const float v1 = std::min(1.f, std::max(0.f, m_AverageSpeed + differential));
                const float v2 = std::min(1.f, std::max(0.f, m_AverageSpeed - differential));
                if (differential >= 0.f) {
                    m_Robot.tank(v1, v2);
                } else {
                    m_Robot.tank(v2, v1);
                }
            } else { // Exit
                m_Robot.stopMoving();
                return true;
            }

            // Keep track of previous error for I and D terms of PID
            m_LastError = error;
        } break;
        case TankPIDState::AtGoal:
            if (event == Event::Enter) {
                LOG_INFO << "Reached goal";
            }
            break;
        case TankPIDState::Invalid:
            break;
        }

        return true;
    }

private:
    FSM<TankPIDState> m_StateMachine;
    Tank &m_Robot;
    Pose2<meter_t, radian_t> m_RobotPose;
    Vector2<meter_t> m_Goal;
    const meter_t m_DistanceTolerance;
    const radian_t m_AngleTolerance, m_StartTurningThreshold;
    radian_t m_HeadingOffset;
    double m_LastError = std::numeric_limits<double>::quiet_NaN();
    Stopwatch m_Stopwatch;
    const float m_Kp, m_Ki, m_Kd, m_AverageSpeed;
}; // TankPID
} // Robots
} // BoBRobotics