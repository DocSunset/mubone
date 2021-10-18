#include <cmath>
#include <limits>
#include "parameters.hpp"
#include "list.hpp"
#include "MainComponent.h"

using namespace mubone;
using namespace mubone::synthesis;

MainComponent::MainComponent()
:   audioSetupComp (deviceManager,
                          0,     // minimum input channels
                          256,   // maximum input channels
                          0,     // minimum output channels
                          256,   // maximum output channels
                          false, // ability to select midi inputs
                          false, // ability to select midi output device
                          false, // treat channels as stereo pairs
                          false),// hide advanced options
    soundsindicator(juce::Slider::LinearHorizontal, juce::Slider::TextBoxLeft),
    referencesindicator(juce::Slider::LinearHorizontal, juce::Slider::TextBoxLeft),
    rmsthreshold(juce::Slider::Rotary, juce::Slider::TextBoxBelow),
    gui_grain{},
    gui_state{},
    state{},
    audiosphere(),
    cloudmanager(audiosphere),
    timeinsamples{0}
{
    initialize(gui_state);
    initialize(state);
    sensor_receiver.add_listener(&gesture_model);
    clear_flag.store(false);

    initializeSetupComp();
    initializeSliders();
    addAndMakeVisible(&grain_display);
    addAndMakeVisible(&state_display);
    setSize (1280, 920);
    initializeAudioSettings();
    
    startTimerHz(20);
    setWantsKeyboardFocus(true);
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
    sensor_receiver.disconnect();
    grain_receiver.disconnect();
    state_receiver.disconnect();
    stopTimer();
}

//== Audio stuff ===============================================================

void MainComponent::initializeAudioSettings()
{
    setAudioChannels (2, 2);
    auto audiosettings = deviceManager.getAudioDeviceSetup();
    audiosettings.bufferSize = 32;
    deviceManager.setAudioDeviceSetup(audiosettings, false);
}

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    audiosphere.prepareToPlay(samplesPerBlockExpected, sampleRate);
    cloudmanager.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()

    audiosphere.releaseResources();
    cloudmanager.releaseResources();
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& iobuffer)
{
    grain_receiver.audio_try_dequeue(cloudmanager.graindescription);
    state_receiver.audio_try_dequeue(state);

    audiosphere.noisethreshold = thresh.load();

    cloudmanager.set_granulate_state(get<granulating>(state));
    // if (get<gesture_recording>(state)) cloudmanager.record();
    // else cloudmanager.stopRecording();

    if (get<clouds_planted>(state) != cloudmanager.numPlanted())
        cloudmanager.setPlantedClouds(get<clouds_planted>(state));

    auto inputchannel = mixInputs(iobuffer);
    if (0 <= inputchannel && inputchannel < iobuffer.buffer->getNumChannels())
    {
        auto readpointer = iobuffer.buffer->getReadPointer(inputchannel, iobuffer.startSample);
        audiosphere.checkThreshold(readpointer, iobuffer.numSamples);
        if (readpointer != nullptr)
        {
            if (get<sound_recording>(state))
            {
                audiosphere.record(
                        readpointer, 
                        iobuffer.numSamples, 
                        cloudmanager.normal(), 
                        timeinsamples); // TODO: add audio_features
            }
            else           
            {
                audiosphere.stopRecording();
            }
        }
    }
    
    iobuffer.clearActiveBufferRegion();
    if (clear_flag.load() || get<reset_trigger>(state)) 
    {
        audiosphere.clear();
        clear_flag.store(false);
    }
    else audiosphere.update();
    cloudmanager.getNextAudioBlock(timeinsamples, iobuffer);
    updateSliders();
    timeinsamples += iobuffer.numSamples;
}

int MainComponent::mixInputs(const juce::AudioSourceChannelInfo& iobuffer)
{
    auto* device = deviceManager.getCurrentAudioDevice();
    if (device == nullptr) return -1; 
    auto activeInputChannels = device->getActiveInputChannels();
    auto maxInputChannels = activeInputChannels.getHighestBit() + 1;

    // get the lowest numbered 
    int minimumActiveChannel = std::numeric_limits<int>::max();
    const auto startsamp = iobuffer.startSample;
    const auto numsamps = iobuffer.numSamples;
    for (int i = 0; i < maxInputChannels; ++i)
    {
        if (activeInputChannels[i])
        {
            // these will be true for the first (lowest index) active input channel
            if (i < minimumActiveChannel) minimumActiveChannel = i;
            if (i == minimumActiveChannel) continue; 

            // all other channels get mixed onto that channel
            iobuffer.buffer->addFrom(minimumActiveChannel, startsamp,
                                     *(iobuffer.buffer), i, startsamp, numsamps);
        }
    }
    if (minimumActiveChannel == std::numeric_limits<int>::max()) return -1; // no active input channels
    else return minimumActiveChannel;
}

// it appears this function is no longer used and should be pruned?
int MainComponent::getInputChannel()
{
    auto* device = deviceManager.getCurrentAudioDevice();
    if (device == nullptr) return -1; 
    auto activeInputChannels = device->getActiveInputChannels();
    auto maxInputChannels = activeInputChannels .getHighestBit() + 1;
    for (int i = 0; i < maxInputChannels; ++i)
        if (activeInputChannels[i]) return i;
    return -1;
}

//== GUI Stuff =================================================================

void MainComponent::initializeSetupComp()
{
    addAndMakeVisible (audioSetupComp);
    cpuUsageLabel.setText ("CPU Usage", juce::dontSendNotification);
    cpuUsageText.setJustificationType (juce::Justification::right);
    addAndMakeVisible (&cpuUsageLabel);
    addAndMakeVisible (&cpuUsageText);
}

void MainComponent::initializeSlider(const double& rangelo, 
                                     const double& rangehi, 
                                     const double& initial, 
                                     juce::Slider& slider,
                                     juce::Colour colour)
{
    slider.setRange(rangelo, rangehi);
    slider.setValue(initial);
    slider.setColour(juce::Slider::ColourIds::thumbColourId, colour);
    addAndMakeVisible(slider);
}

void MainComponent::initializeSliders()
{
    threshindicator.setColour(juce::TextButton::buttonOnColourId, juce::Colours::goldenrod);
    addAndMakeVisible(threshindicator);
    for (int i = 0; i < mubone::synthesis::CloudManager::numclouds; ++i)
    {
        auto& button = cloud_lights[i];
        if (i&1) button.setColour(juce::TextButton::buttonOnColourId, juce::Colours::aqua);
        else button.setColour(juce::TextButton::buttonOnColourId, juce::Colours::aquamarine);
        addAndMakeVisible(button);
    }
    initializeSlider(0, 200, 0, soundsindicator);
    initializeSlider(0, 100000, 0, referencesindicator);
    initializeSlider(-70, 0, -35, rmsthreshold, juce::Colours::goldenrod);
    rmsthreshold.addListener(this);
}

void MainComponent::updateSliders()
{
    thresholdpassed.store(audiosphere.thresholdPassed());
    for (int i = 0; i < mubone::synthesis::CloudManager::numclouds; ++i)
    {
        cloud_flags[i].store(cloudmanager.cloudPlanted(i));
    }
    sounds.store(static_cast<int>(audiosphere.numSounds()));
    references.store(static_cast<int>(audiosphere.numReferences()));
}

void MainComponent::resized()
{
    const auto margin = 5;
    const auto grid = 40;

    juce::Rectangle<int> area(getLocalBounds());
    
    const auto min_setupwidth = 300;
    const auto setupwidth = area.getWidth() / 4 > min_setupwidth ? area.getWidth() / 4 : 0;
    auto setup = area.removeFromLeft(setupwidth);
    auto cpu = setup.removeFromTop(grid);
    cpuUsageLabel.setBounds(cpu.removeFromLeft((cpu.reduced(margin).getWidth() / 2)));
    cpuUsageText.setBounds(cpu);
    audioSetupComp.setBounds(setup);

    auto lights = area.removeFromTop(4 * grid);
    auto light_width = lights.getWidth() / mubone::synthesis::CloudManager::numclouds;
    for (auto& button : cloud_lights)
        button.setBounds(lights.removeFromLeft(light_width).reduced(3*margin));
    
    soundsindicator.setBounds(area.removeFromBottom(grid).reduced(margin));
    referencesindicator.setBounds(area.removeFromBottom(grid).reduced(margin));
    auto state = area.removeFromBottom(4 * grid);
    auto threshrect = state.removeFromLeft(8 * grid);
    rmsthreshold.setBounds(threshrect.removeFromLeft(threshrect.getWidth()/2).reduced(margin));
    threshindicator.setBounds(threshrect.reduced(margin));
    state_display.setBounds(state.reduced(margin));
    
    grain_display.setBounds(area.reduced(margin));
}

float dbtoa(double db)
{
    return std::pow(10, db / 20);
}

void MainComponent::sliderValueChanged(juce::Slider * s)
{
    if (s == &rmsthreshold) thresh.store(dbtoa(rmsthreshold.getValue()));
}

void MainComponent::timerCallback()
{
    auto cpu = deviceManager.getCpuUsage() * 100;
    cpuUsageText.setText (juce::String (cpu, 6) + " %", juce::dontSendNotification);

    for (int i = 0; i < mubone::synthesis::CloudManager::numclouds; ++i)
    {
        cloud_lights[i].setToggleState(cloud_flags[i].load(), juce::dontSendNotification);
    }
    soundsindicator.setValue(sounds.load(), juce::dontSendNotification);
    referencesindicator.setValue(references.load(), juce::dontSendNotification);
    threshindicator.setToggleState(thresholdpassed.load(), juce::dontSendNotification);
    while(grain_receiver.gui_try_dequeue(gui_grain)) {}
    grain_display.update(gui_grain);
    while(state_receiver.gui_try_dequeue(gui_state)) {}
    state_display.update(gui_state);
}

bool MainComponent::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    auto character = key.getTextDescription();
    if (character == juce::String("A"))
    {
        sensor_receiver.align.store(true);
        return true;
    }
    else if (character == juce::String("I"))
    {
        sensor_receiver.init.store(true);
        return true;
    }
    else if (character == juce::String("Z"))
    {
        gesture_model.set_zero.store(true);
        return true;
    }
    else if (character == juce::String("R"))
    {
        clear_flag.store(true);
        return true;
    }
    else return false;
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}
