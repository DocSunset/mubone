/*
  ==============================================================================

    audiosphere.h
    Created: 17 Dec 2018 1:40:30pm
    Author:  Travis West

  ==============================================================================
*/

#pragma once

#include <memory>

#include "../3rdparty/simplesound/simple/circularbuffer.h"
#include "linalgtypes.h"

#include "soundobject.h"
#include "soundobjectlist.h"
// #include "cloudmanager.h"
#include "../JuceLibraryCode/JuceHeader.h"


namespace mubone
{

// this class provides a friendly interface to a SoundObjectList, collecting
// audio in real time and pushing it to the list to collect into separate sounds 
// which can then be sorted and accessed based on similarity to a control sample

class Audiosphere : public SoundObjectList
{
public:
    Audiosphere()
    :    
        SoundObjectList(),
        samplerate{0},
        readytoplay{false},
        constructingsoundobject{false},
        request{}
    {}
    
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate);
    bool readyToPlay() {return readytoplay;}
    void releaseResources();

    void record(const float* input, 
                const int& numsamples, 
                const Vector& ctrl,
                const std::size_t& t);
    void stopRecording();

    void checkThreshold(const float* input, const int& numsamples);
    static double getRMS(const float* input, const int& numsamples);
    bool thresholdPassed() const {return thresholdpassed;}
    bool recording() const {return constructingsoundobject;}
    double noisethreshold;
    static const int minimumDurationInMilliseconds = 10;

private:
    void startSoundObject();

    void writeToBuffers(const float* input, 
                        const int& numsamples, 
                        const Vector& ctrl,
                        const std::size_t& t);

    void endSoundObject();

    void updateBuilderBufferReferences();

    int minimumDurationInSamples;
    double samplerate;
    bool readytoplay;

    double rmsenvelope;
    bool thresholdpassed;
    bool constructingsoundobject;
    SoundObjectBuilder request;

    std::shared_ptr<Simple::CircularBuffer<float>> audiobuffer;
    std::shared_ptr<Simple::CircularBuffer<std::pair<std::size_t, Vector>>> ctrlbuffer;

    static const int buffersizeinseconds = 60 * 3; 
};

} // namespace mubone
