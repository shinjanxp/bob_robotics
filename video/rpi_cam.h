#pragma once

// C++ includes
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// Extra 'nix includes
#ifndef WIN32
#include <unistd.h>
#include <fcntl.h>
#endif

#include <math.h>

// OpenCV
#include <opencv2/opencv.hpp>

// BoBRobotics includes
#include "input.h"
// standardise sockets across platforms
#include "../os/net.h"

namespace BoBRobotics {
namespace Video {

class RPiCamera : public Input
{
public:

    RPiCamera()
      : RPiCamera(0)
    {}

    RPiCamera(int port) 
    : m_Socket(0)
    {

        m_Port = port;

        // set up the networking code needed to receive images:
        if (!setupSockets()) {
            // error
            return;
        }

        // setup temp frame
        cv::Mat f(72,152,CV_32FC1);
        m_Frame = f;

    }
	
    virtual ~RPiCamera()
    {}

    virtual std::string getCameraName() const override
    {
        return "Raspberry Pi Camera";
    }

    virtual cv::Size getOutputSize() const override
    {
            // this is fixed
            return cv::Size(152,72);
    }

    virtual bool readFrame(cv::Mat &outFrame) override
    {
        unsigned char buffer[72*19];

        if (!m_Socket) {
            return false;
        }

        // get the most recent UDP frame (grayscale for now)
        while (recv(m_Socket, buffer, 72*19, 0) > 0) {
            // fill in the outFrame
            //std::cout << (int) buffer[0] << std::endl;        
            int j = 0;
            for (int i = 0; i < 72*19-1; ++i) {
                m_Frame.at<float>(i%72,buffer[0]+floor(i/72)) = float(buffer[j])/255.0f;
                ++j;
            }
        }

        outFrame = m_Frame;
        
        // return true for now, but not right as we are not fetching a colour frame 
        return true;
    }

    virtual bool readGreyscaleFrame(cv::Mat &outFrame)
    {
        // If reading (RGB frame) was succesful
        /*if(readFrame(m_IntermediateFrame)) {
            // If output frame isn't correct size, create it
            if(outFrame.size() != m_IntermediateFrame.size()) {
                outFrame.create(m_IntermediateFrame.size(), CV_8UC1);
            }

            // Convert intermediate frame to greyscale - needed?
            //cv::cvtColor(m_IntermediateFrame, outFrame, CV_BGR2GRAY);
            return true;
        }
        else {
            return false;
        }*/

        readFrame(m_Frame);
        outFrame = m_Frame;	
        
        return true;

    }

    virtual bool needsUnwrapping() const
    {
        // we do not need to unwrap this - it is done onboard the RPi
        return false;
    }

    virtual void setOutputSize(const cv::Size &)
    {
        throw std::runtime_error("This camera's resolution cannot be changed");
    }

private:
    bool setupSockets()
    {
        struct sockaddr_in addr;

        m_Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_Socket == INVALID_SOCKET) {
            goto error;
        }

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(m_Port);

        if (bind(m_Socket, (const sockaddr *) &addr, (int) sizeof(addr))) {
            goto error;
        }
            
            // non-blocking socket
#ifdef WIN32		
        ulong nonblocking_enabled = 1;
        ioctlsocket(m_Socket, FIONBIO, &nonblocking_enabled);
#else		
        fcntl(m_Socket, F_SETFL, O_NONBLOCK); 
#endif
        return true;
		
error:
        std::cerr << "Error (" << errno << "): RPi Cam UDP " << m_Port << std::endl;
        exit(1);
    }

    int m_Socket;
    int m_Port;
    cv::Mat m_Frame;
    cv::Mat m_IntermediateFrame;
}; // RPiCamera
} // Video
} // BoBRobotics
