#pragma once

// Standard C++ includes
#include <vector>

// Standard C includes
#include <cmath>
#include <cstdint>

// Common includes
#include "../common/i2c_interface.h"
#include "tank.h"

namespace BoBRobotics {
namespace Robots {
//----------------------------------------------------------------------------
// BoBRobotics::Robots::Norbot
//----------------------------------------------------------------------------
//! An interface for wheeled, Arduino-based robots developed at the University of Sussex
class Norbot : public Tank
{
public:
    Norbot(const char *path = "/dev/i2c-1", int slaveAddress = 0x29)
      : m_I2C(path, slaveAddress)
      , m_Left(0.0f)
      , m_Right(0.0f)
    {}

    //----------------------------------------------------------------------------
    // Tank virtuals
    //----------------------------------------------------------------------------
    virtual ~Norbot() override
    {
        stopMoving();
    }

    virtual void tank(float left, float right) override
    {
        BOB_ASSERT(left >= -1.f && left <= 1.f);
        BOB_ASSERT(right >= -1.f && right <= 1.f);

        // Cache left and right
        m_Left = left;
        m_Right = right;

        // Convert standard (-1,1) values to bytes in order to send to I2C slave
        uint8_t buffer[2] = { floatToI2C(left), floatToI2C(right) };

        // Send buffer
        write(buffer);
    }

    //----------------------------------------------------------------------------
    // Public API
    //----------------------------------------------------------------------------
    template<typename T, size_t N>
    void read(T (&data)[N])
    {
        m_I2C.read(data);
    }

    template<typename T, size_t N>
    void write(const T (&data)[N])
    {
        m_I2C.write(data);
    }

    float getLeft() const
    {
        return m_Left;
    }

    float getRight() const
    {
        return m_Right;
    }

private:
    //----------------------------------------------------------------------------
    // Private methods
    //----------------------------------------------------------------------------
    uint8_t floatToI2C(float speed)
    {
        return (uint8_t) std::min(
                255.0f,
                std::max(0.0f, std::round(((speed + 1.0f) / 2.0f) * 255.0f)));
    }

    //----------------------------------------------------------------------------
    // Private members
    //----------------------------------------------------------------------------
    BoBRobotics::I2CInterface m_I2C;
    float m_Left;
    float m_Right;
}; // Norbot
} // Robots
} // BoBRobotics
