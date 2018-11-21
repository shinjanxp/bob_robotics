#pragma once

// BoB robotics includes
#include "../common/pose.h"
#include "../common/stopwatch.h"
#include "../hid/joystick.h"
#include "../robots/robot.h"
#include "../video/opengl.h"
#include "common.h"
#include "renderer.h"

// OpenGL includes
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// Standard C includes
#include <cmath>

// Standard C++ includes
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>

namespace BoBRobotics {
namespace AntWorld {
using namespace units::literals;

//----------------------------------------------------------------------------
// BoBRobotics::AntWorld::AntAgent
//----------------------------------------------------------------------------
//! A simple agent with a position and a panoramic view of the current AntWorld
class AntAgent
  : public Robots::Robot
  , public Video::OpenGL
{
    using meter_t = units::length::meter_t;
    using meters_per_second_t = units::velocity::meters_per_second_t;
    using degree_t = units::angle::degree_t;
    using radians_per_second_t = units::angular_velocity::radians_per_second_t;

public:
    static constexpr meters_per_second_t DefaultVelocity = 300_mm / 1_s;
    static constexpr radians_per_second_t DefaultTurnSpeed = 200_deg_per_s;

    AntAgent(GLFWwindow *window, Renderer &renderer,
             const cv::Size &renderSize,
             meters_per_second_t velocity = DefaultVelocity, radians_per_second_t turnSpeed = DefaultTurnSpeed)
      : Video::OpenGL(renderSize)
      , m_Velocity(velocity)
      , m_TurnSpeed(turnSpeed)
      , m_Renderer(renderer)
      , m_Window(window)
    {}

    virtual void addJoystick(HID::Joystick &joystick) override
    {
        addJoystick(joystick, 0.25f);
    }

    void addJoystick(HID::Joystick &joystick, float deadZone)
    {
        joystick.addHandler(
                [this, deadZone](HID::JAxis axis, float value) {
                    switch (axis) {
                    case HID::JAxis::LeftStickVertical:
                        m_JoystickY = value;
                        break;
                    case HID::JAxis::LeftStickHorizontal:
                        m_JoystickX = value;
                        break;
                    default:
                        return false;
                    }

                    if (fabs(m_JoystickY) > deadZone) {
                        moveForward(-m_JoystickY);
                    } else if (fabs(m_JoystickX) > deadZone) {
                        turnOnTheSpot(m_JoystickX);
                    } else {
                        stopMoving();
                    }
                    return true;
                });
    }

    template<typename LengthUnit = meter_t>
    Vector3<LengthUnit> getPosition()
    {
        std::lock_guard<std::mutex> guard(m_PoseMutex);
        updatePose();
        return convertUnitArray<LengthUnit>(m_Position);
    }

    template<typename AngleUnit = degree_t>
    Vector3<AngleUnit> getAttitude()
    {
        std::lock_guard<std::mutex> guard(m_PoseMutex);
        updatePose();
        return convertUnitArray<AngleUnit>(m_Attitude);
    }

    template<typename LengthUnit = meter_t, typename AngleUnit = degree_t>
    auto getPose()
    {
        std::lock_guard<std::mutex> guard(m_PoseMutex);
        updatePose();
        return std::make_pair(convertUnitArray<LengthUnit>(m_Position),
                              convertUnitArray<AngleUnit>(m_Attitude));
    }

    GLFWwindow *getWindow() const
    {
        return m_Window;
    }


    virtual radians_per_second_t getMaximumTurnSpeed() override
    {
        return m_TurnSpeed;
    }

    void setPosition(meter_t x, meter_t y, meter_t z)
    {
        BOB_ASSERT(m_MoveMode == MoveMode::NotMoving);

        m_Position[0] = x;
        m_Position[1] = y;
        m_Position[2] = z;
    }

    void setAttitude(degree_t yaw, degree_t pitch, degree_t roll)
    {
        BOB_ASSERT(m_MoveMode == MoveMode::NotMoving);

        m_Attitude[0] = yaw;
        m_Attitude[1] = pitch;
        m_Attitude[2] = roll;
    }

    virtual bool readFrame(cv::Mat &frame) override
    {
        // If the agent is "moving", we need to calculate its current position
        std::lock_guard<std::mutex> guard(m_PoseMutex);
        updatePose();

        // Clear colour and depth buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render first person
        const auto size = getOutputSize();
        m_Renderer.renderPanoramicView(m_Position[0], m_Position[1], m_Position[2],
                                       m_Attitude[0], m_Attitude[1], m_Attitude[2],
                                       0, 0, size.width, size.height);

        // Swap front and back buffers
        glfwSwapBuffers(m_Window);

        // Read frame
        return Video::OpenGL::readFrame(frame);
    }

    virtual void stopMoving() override
    {
        std::lock_guard<std::mutex> guard(m_PoseMutex);
        updatePose();
        m_MoveMode = MoveMode::NotMoving;
    }

    virtual void moveForward(float speed) override
    {
        BOB_ASSERT(speed >= -1.f && speed <= 1.f);

        std::lock_guard<std::mutex> guard(m_PoseMutex);
        BOB_ASSERT(m_Attitude[1] == 0_deg && m_Attitude[2] == 0_deg);

        updatePose();
        m_MoveMode = MoveMode::MovingForward;
        m_MoveSpeed = speed;
    }

    virtual void turnOnTheSpot(float clockwiseSpeed) override
    {
        BOB_ASSERT(clockwiseSpeed >= -1.f && clockwiseSpeed <= 1.f);

        std::lock_guard<std::mutex> guard(m_PoseMutex);
        BOB_ASSERT(m_Attitude[1] == 0_deg && m_Attitude[2] == 0_deg);

        updatePose();
        m_MoveMode = MoveMode::Turning;
        m_MoveSpeed = clockwiseSpeed;
    }

    static auto initialiseWindow(const cv::Size &size)
    {
        // Set GLFW error callback
        glfwSetErrorCallback(handleGLFWError);

        // Initialize the library
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        // Prevent window being resized
        glfwWindowHint(GLFW_RESIZABLE, false);

        GLFWwindow *ptr = glfwCreateWindow(size.width, size.height, "Ant world", nullptr, nullptr);
        if (!ptr) {
            glfwTerminate();
            throw std::runtime_error("Failed to create window");
        }

        // Wrap in a unique_ptr so we can free it properly when we're done
        std::unique_ptr<GLFWwindow, std::function<void(GLFWwindow *)>> window(ptr, &glfwDestroyWindow);

        // Make the window's context current
        glfwMakeContextCurrent(window.get());

        // Initialize GLEW
        if (glewInit() != GLEW_OK) {
            throw std::runtime_error("Failed to initialize GLEW");
        }

        // Enable VSync
        glfwSwapInterval(1);

        glDebugMessageCallback(handleGLError, nullptr);

        // Set clear colour to match matlab and enable depth test
        glClearColor(0.0f, 1.0f, 1.0f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glLineWidth(4.0);
        glPointSize(4.0);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glEnable(GL_TEXTURE_2D);

        return window;
    }

private:
    using TimeType = std::chrono::time_point<std::chrono::high_resolution_clock>;

    Vector3<degree_t> m_Attitude{ { 0_deg, 0_deg, 0_deg } };
    Vector3<meter_t> m_Position{ { 0_m, 0_m, 0_m } };
    meters_per_second_t m_Velocity;
    radians_per_second_t m_TurnSpeed;
    Renderer &m_Renderer;
    GLFWwindow *m_Window;
    std::mutex m_PoseMutex;
    float m_JoystickX = 0.f, m_JoystickY = 0.f;

    Stopwatch m_MoveStopwatch;
    enum class MoveMode
    {
        NotMoving,
        MovingForward,
        Turning
    } m_MoveMode = MoveMode::NotMoving;
    float m_MoveSpeed;

    void updatePose()
    {
        const units::time::second_t elapsed = m_MoveStopwatch.lap();

        using namespace units::math;
        switch (m_MoveMode) {
        case MoveMode::MovingForward: {
            const meter_t dist = m_MoveSpeed * m_Velocity * elapsed;
            m_Position[0] += dist * cos(m_Attitude[0]);
            m_Position[1] += dist * sin(m_Attitude[0]);
        } break;
        case MoveMode::Turning:
            m_Attitude[0] -= m_MoveSpeed * m_TurnSpeed * elapsed;
            while (m_Attitude[0] > 360_deg) {
                m_Attitude[0] -= 360_deg;
            }
            while (m_Attitude[0] < 0_deg) {
                m_Attitude[0] += 360_deg;
            }
            break;
        default:
            break;
        }
    }

    static void handleGLFWError(int errorNumber, const char *message)
    {
        throw std::runtime_error("GLFW error number: " + std::to_string(errorNumber) + ", message:" + message);
    }

    static void handleGLError(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar *message, const void *)
    {
        throw std::runtime_error(message);
    }

    static TimeType now()
    {
        return std::chrono::high_resolution_clock::now();
    }
}; // AntAgent

#ifndef NO_HEADER_DEFINITIONS
constexpr units::velocity::meters_per_second_t AntAgent::DefaultVelocity;
constexpr units::angular_velocity::radians_per_second_t AntAgent::DefaultTurnSpeed;
#endif

} // AntWorld
} // BoBRobotics
