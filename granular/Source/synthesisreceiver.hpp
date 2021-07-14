#pragma once



#include <cmath>

#include <../3rdparty/simplesound/simple/boundaries.h>
#include <../3rdparty/simplesound/simple/constants/pi.h>
#include "../3rdparty/readerwriterqueue/readerwriterqueue.h"
#include "../3rdparty/readerwriterqueue/atomicops.h"

#include "oscreceiver.hpp"
#include "../JuceLibraryCode/JuceHeader.h"

namespace mubone::synthesis
{

template<typename number>
number chord_length_from_central_angle(const number radians)
{
    return 2.0 * std::sin(radians / 2.0);
}

template<typename number = search_radius::value_type>
number search_radius_mapping(const number v)
{
    number degrees = Simple::clip(v, 180.0f, 1.0f);
    number radians = degrees * Simple::deg_to_rad;
    number distance = chord_length_from_central_angle(radians);
    return distance;
}

void synthesis_parameter_mapping(GrainDescription& g, GrainDescription& prev)
{
    if (get<search_radius>(g).value != get<search_radius>(prev).value)
    {
        get<search_radius>(g) = search_radius_mapping(get<search_radius>(g).value);
        get<search_radius>(prev) = get<search_radius>(g);
    }
}

void synthesis_parameter_mapping(State& s, State& prev)
{
}

template<class SignalList>
class QueuedReceiver : public OSCReceiver<SignalList>
{
public:
    QueuedReceiver()
    :
        OSCReceiver<SignalList>(),
        gui_queue{4},
        audio_queue{1}
    {
        osc.addListener(this);
        if constexpr (std::is_same_v<SignalList, GrainDescription>) osc.connect(8008);
        else if constexpr (std::is_same_v<SignalList, State>)       osc.connect(7007);
    }
    
    void disconnect() {osc.disconnect();}
    ~QueuedReceiver() {disconnect();}

    void enqueue_siglist()
    {
        SignalList& newlist = OSCReceiver<SignalList>::siglist;
        synthesis_parameter_mapping(newlist, previous_list);
        gui_queue.try_enqueue(newlist);
        audio_queue.try_enqueue(newlist);
    }

    void extra_message_handler(const juce::OSCMessage& message) override
    {
        if (not OSCReceiver<SignalList>::handling_bundle) enqueue_siglist();
    }

    void extra_bundle_handler(const juce::OSCBundle& bundle) override
    {
        if (OSCReceiver<SignalList>::messages_handled > 0)
        {
            enqueue_siglist();
            OSCReceiver<SignalList>::messages_handled = 0;
        }
    }

    bool gui_try_dequeue(SignalList& siglist) { return gui_queue.try_dequeue(siglist); }
    bool audio_try_dequeue(SignalList& siglist) { return audio_queue.try_dequeue(siglist); }

private:
    juce::OSCReceiver osc;
    moodycamel::ReaderWriterQueue<SignalList> gui_queue;
    moodycamel::ReaderWriterQueue<SignalList> audio_queue;
    SignalList previous_list;
};

#include "parameters.hpp"
using GrainReceiver = QueuedReceiver<GrainDescription>;
using StateReceiver = QueuedReceiver<State>;

} // namespace mubone
