#pragma once

// BoBRobotics includes
#include "../os/net.h"
#include "input.h"

// OpenCV
#include <opencv2/opencv.hpp>

// Standard C includes
#include <cmath>
#include <cstdlib>

// Standard C++ includes
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// Extra 'nix includes
#ifndef WIN32
#include <fcntl.h>
#include <unistd.h>
#endif

namespace BoBRobotics {
namespace Video {

class RPiCamera : public Input
{
public:
    RPiCamera(uint16_t port = 0)
      : m_Socket(INVALID_SOCKET)
      , m_Port(port)
    {
        struct sockaddr_in addr;

        m_Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_Socket == INVALID_SOCKET) {
            throw OS::Net::NetworkError("Could not create socket");
        }

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(m_Port);

        if (bind(m_Socket, (const sockaddr *) &addr, (socklen_t) sizeof(addr))) {
            throw OS::Net::NetworkError("Could not create socket");
        }

        // non-blocking socket
#ifdef WIN32
        ulong nonblocking_enabled = 1;
        ioctlsocket(m_Socket, FIONBIO, &nonblocking_enabled);
#else
        fcntl(m_Socket, F_SETFL, O_NONBLOCK);
#endif
    }

    virtual ~RPiCamera()
    {
        if (m_Socket != INVALID_SOCKET) {
            close(m_Socket);
        }
    }

    virtual std::string getCameraName() const override
    {
        return "rpi_camera";
    }

    virtual cv::Size getOutputSize() const override
    {
        // This is fixed
        return cv::Size(152, 72);
    }

    virtual bool readFrame(cv::Mat &outFrame) override
    {
        // Frames from the RPi camera are always greyscale
        return readGreyscaleFrame(outFrame);
    }

    virtual bool readGreyscaleFrame(cv::Mat &outFrame) override
    {
        unsigned char buffer[72 * 19];

        if (m_Socket == INVALID_SOCKET) {
            return false;
        }

        // Make sure frame is correct size and type
        outFrame.create(72, 152, CV_32FC1);

        // Get the most recent UDP frame (grayscale for now)
        while (recv(m_Socket, reinterpret_cast<readbuff_t>(buffer), 72 * 19, 0) > 0) {
            // fill in the outFrame
            for (int i = 0; i < 72 * 19 - 1; ++i) {
                outFrame.at<float>(i % 72, buffer[0] + floor(i / 72)) = float(buffer[i]) / 255.0f;
            }
        }

        // Return true to indicate success
        return true;
    }

    virtual bool needsUnwrapping() const override
    {
        // We do not need to unwrap this - it is done onboard the RPi
        return false;
    }

private:
    socket_t m_Socket;
    uint16_t m_Port;
}; // RPiCamera
} // Video
} // BoBRobotics
