#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "parameters.hpp"
#include "oscreceiver.hpp"

namespace mubone::sensor
{

class Receiver : public OSCReceiver<sensor::Sample>
{
public:
    Receiver()
    :
        OSCReceiver<sensor::Sample>()
    {
        counterweight.addListener(this);
        slide_brace.addListener(this);
        counterweight.connect(counterweight_port);
        slide_brace.connect(slide_brace_port);
        counterweight_output.connect(broadcast_ip, output_port);
        slide_brace_output.connect(broadcast_ip, output_port);
        align.store(false);
        init.store(false);
    }

    void disconnect()
    {
        counterweight.disconnect();
        slide_brace.disconnect();
        counterweight_output.disconnect();
        slide_brace_output.disconnect();
    }
    ~Receiver() {disconnect();}

    void extra_message_handler(const juce::OSCMessage& message) override
    {
        if (message.getAddressPattern().matches("/device_ip"))
        {
            // determine which receiver got this message and set the corresponding output's IP address
        }
        if (align.load())
        {
            counterweight_output.send("/state/align");
            //slide_brace_output.send("/state/align");
            align.store(false);
        }
        if (init.load())
        {
            counterweight_output.send("/state/initialize");
            init.store(false);
        }
    }

    std::atomic<bool> align;
    std::atomic<bool> init;

private:
    juce::OSCReceiver counterweight;
    juce::OSCReceiver slide_brace;
    juce::OSCSender counterweight_output;
    juce::OSCSender slide_brace_output;
    static constexpr int output_port = 6006;
    static constexpr char broadcast_ip[] = "192.168.0.255";

    static constexpr int counterweight_port = 5004;
    static constexpr int slide_brace_port = 5005;
};

} // namespace mubone
