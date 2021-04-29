/*
  ==============================================================================

    audiosphere.cpp
    Created: 17 Dec 2018 1:40:30pm
    Author:  Travis West

  ==============================================================================
*/

#include "audiosphere.h"
#include <cmath>

namespace mubone
{

void Audiosphere::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    // TODO: sound objects should also be resampled if the sample rate has changed
    // TODO: should not allocate large buffers in prepareToPlay...

    samplerate = sampleRate;
    minimumDurationInSamples = minimumDurationInMilliseconds * sampleRate / 1000;

    int audiobuffersize = buffersizeinseconds * samplerate;
    int ctrlbuffersize   = buffersizeinseconds * samplerate / samplesPerBlockExpected;

    audiobuffer = std::make_shared<Simple::CircularBuffer<float>>(audiobuffersize);
    ctrlbuffer  = std::make_shared<Simple::CircularBuffer<std::pair<std::size_t, Vector>>>(ctrlbuffersize);

    audiobuffer->clear();
    ctrlbuffer->clear();

    updateBuilderBufferReferences();

    readytoplay = true;
}

void Audiosphere::releaseResources()
{
    audiobuffer = nullptr;
    ctrlbuffer = nullptr;
    updateBuilderBufferReferences();
    readytoplay = false;
}

void Audiosphere::record(const float* input, 
                         const int& numsamples, 
                         const Vector& ctrl,
                         const std::size_t& t)
{
    if (!readytoplay) return;

    if (thresholdpassed)
    {
        if (!constructingsoundobject) startSoundObject();
        writeToBuffers(input, numsamples, ctrl, t);
    }
    else if (constructingsoundobject)
    {
        writeToBuffers(input, numsamples, ctrl, t);
        endSoundObject();
    }
}
    
void Audiosphere::stopRecording()
{
    if (constructingsoundobject)
    {
        endSoundObject();
    }
}

void Audiosphere::checkThreshold(const float* input,
                                 const int& numsamples)
{
    double newrms = Audiosphere::getRMS(input, numsamples);
    if (newrms < rmsenvelope) rmsenvelope = 0.9*rmsenvelope + 0.1*newrms;
    else rmsenvelope = 0.2*rmsenvelope + 0.8*newrms;
    thresholdpassed = rmsenvelope > noisethreshold;
}

double Audiosphere::getRMS(const float* input,
                           const int& numsamples)
{
    double sumofsquares = 0;
    for (int i = 0; i < numsamples; ++i)
        sumofsquares += input[i] * input[i];

    double meanofsquares = sumofsquares / numsamples;
    double rms = std::sqrt(meanofsquares);

    return rms;
}

void Audiosphere::startSoundObject()
{
    request.startsamp = audiobuffer->writehead();
    request.startblock = ctrlbuffer->writehead();
    constructingsoundobject = true;
}

void Audiosphere::writeToBuffers(const float* input,
                                 const int& numsamples,
                                 const Vector& ctrl,
                                 const std::size_t& t)
{
    audiobuffer->copyFrom(input, numsamples);
    ctrlbuffer->tick(std::make_pair(t, ctrl));
}

void Audiosphere::endSoundObject()
{
    constructingsoundobject = false;
    request.numsamples = audiobuffer->sampsBetween(request.startsamp, 
                                                   audiobuffer->writehead());

    if (request.numsamples < minimumDurationInSamples) return;

    request.numblocks = ctrlbuffer->sampsBetween(request.startblock, 
                                                 ctrlbuffer->writehead());
    addSound(request);
}

void Audiosphere::updateBuilderBufferReferences()
{
    request.audiobuffer = audiobuffer;
    request.ctrlbuffer = ctrlbuffer;
}

} // namespace mubone
