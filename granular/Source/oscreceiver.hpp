#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "linalgtypes.h"
#include "parameters.hpp"

#include <vector>
#include <type_traits>

namespace mubone
{
using namespace sensor;
using namespace gesture;
using namespace synthesis;

template<class SignalList>
class OSCReceiver
:   public juce::OSCReceiver::Listener<juce::OSCReceiver::RealtimeCallback>
{
public:
    class Listener
    {
    public:
        virtual void oscBundleReceived(const SignalList& siglist) {}
        virtual ~Listener() {}
    };

    OSCReceiver() {initialize(siglist);}

    void add_listener(Listener* l) {listeners.push_back(l);}

    void virtual extra_message_handler(const juce::OSCMessage& message) {}
    void oscMessageReceived(const juce::OSCMessage& message) override
    {
        if (message.isEmpty()) return;
        for_each(siglist, [&](auto& sig)
        {
            if (!message.getAddressPattern().matches(sig.address()))
                return;
            
            using sig_t = typename std::remove_reference_t<decltype(sig)>::value_type;

            if constexpr (std::is_same_v<sig_t, Vector>)
            {
                if (message.size() > 2 &&
                    message[0].isFloat32() &&
                    message[1].isFloat32() &&
                    message[2].isFloat32())
                    sig = Vector(message[0].getFloat32(),
                                       message[1].getFloat32(),
                                       message[2].getFloat32());
            }
            else if constexpr (std::is_same_v<sig_t, Quaternion>)
            {
                if (message.size() > 3 &&
                    message[0].isFloat32() &&
                    message[1].isFloat32() &&
                    message[2].isFloat32() &&
                    message[3].isFloat32())
                    sig = Quaternion(message[3].getFloat32(),
                                     message[0].getFloat32(),
                                     message[1].getFloat32(),
                                     message[2].getFloat32());
            }
            else if constexpr (std::is_integral_v<sig_t>)
            {
                if (message[0].isInt32()) sig = message[0].getInt32();
            }
            else if constexpr (std::is_floating_point_v<sig_t>)
            {
                if (message[0].isFloat32()) sig = message[0].getFloat32();
            }
            messages_handled += 1;
            return;
        });
        extra_message_handler(message);
    }

    void virtual extra_bundle_handler(const juce::OSCBundle& bundle) {}
    void oscBundleReceived(const juce::OSCBundle& bundle) override
    {
        messages_handled = 0;
        for (const auto& element : bundle)
        {
            if (element.isMessage()) oscMessageReceived(element.getMessage());
            else oscBundleReceived(element.getBundle());
        }
        if (messages_handled > 0)
        {
            for (auto l : listeners) l->oscBundleReceived(siglist);
        }
        extra_bundle_handler(bundle);
    }

    SignalList siglist;
    int messages_handled;
private:
    std::vector<Listener*> listeners;
};

} // namespace mubone
