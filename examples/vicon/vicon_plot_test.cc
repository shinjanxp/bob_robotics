// C++ includes
#include <chrono>
#include <iostream>
#include <thread>

// BoB robotics includes
#include "common/plot_agent.h"
#include "vicon/capture_control.h"
#include "vicon/udp.h"

// Third-party includes
#include "third_party/matplotlibcpp.h"

using namespace BoBRobotics;
using namespace std::literals;
using namespace units::angle;
using namespace units::literals;
namespace plt = matplotlibcpp;

auto now()
{
    return std::chrono::high_resolution_clock::now();
}

int
main()
{
    Vicon::UDPClient<> vicon(51001);
    Vicon::CaptureControl viconCaptureControl("192.168.1.100", 3003, "c:\\users\\ad374\\Desktop");
    while (vicon.getNumObjects() == 0) {
        std::this_thread::sleep_for(1s);
        std::cout << "Waiting for object" << std::endl;
    }

    if (!viconCaptureControl.startRecording("test1")) {
        return EXIT_FAILURE;
    }

    bool warningGiven = false;
    do {
        plt::figure(1);
        auto data = vicon.getObjectData(0);
        plotAgent(data.getPose<>(), -2500_mm, 2500_mm, -2500_mm, 2500_mm);
        if (data.timeSinceReceived() > 500ms) {
            if (!warningGiven) {
                std::cerr << "Warning: Object is out of range" << std::endl;
                warningGiven = true;
            }
        } else {
            warningGiven = false;
        }
        plt::pause(0.025);
    } while (plt::fignum_exists(1));

    if (!viconCaptureControl.stopRecording("test1")) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
