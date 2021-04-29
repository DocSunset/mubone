#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <atomic>
#include <array>

#include "sensorreceiver.hpp"
#include "gesturemodel.hpp"
#include "synthesisreceiver.hpp"
#include "audiosphere.h"
#include "cloudmanager.h"
#include "displaycomponents.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent   : public AudioAppComponent, public Timer, public Slider::Listener, public KeyListener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const AudioSourceChannelInfo& iobuffer) override;
    void releaseResources() override;

    //==============================================================================
    void paint (Graphics& g) override;
    void resized() override;

    //==============================================================================
    void timerCallback() override;
    void sliderValueChanged(Slider * s) override;
    bool keyPressed(const KeyPress& key, Component* originatingComponent) override;

private:
    void initializeSlider(const double& rangelo, const double& rangehi, 
                          const double& initial, Slider& slider, Colour colour = Colours::aqua);
    void initializeSliders();
    void updateSliders();
    void initializeSetupComp();
    void initializeAudioSettings();
    int mixInputs(const AudioSourceChannelInfo& iobuffer); 
    // returns input channel to which the inputs were mixed
    int getInputChannel();

    AudioDeviceSelectorComponent audioSetupComp;
    Label cpuUsageLabel;
    Label cpuUsageText;

    std::array<TextButton, mubone::synthesis::CloudManager::numclouds> cloud_lights;
    std::array<std::atomic<bool>, mubone::synthesis::CloudManager::numclouds> cloud_flags;

    Slider soundsindicator;
    std::atomic<int> sounds;

    Slider referencesindicator;
    std::atomic<int> references;

    Slider rmsthreshold;
    std::atomic<float> thresh;
    TextButton threshindicator;
    std::atomic<bool> thresholdpassed;
    std::atomic<bool> clear_flag;

    GrainDisplayComponent grain_display;
    StateDisplayComponent state_display;

    mubone::synthesis::GrainReceiver grain_receiver;
    mubone::synthesis::StateReceiver state_receiver;
    mubone::gesture::Model gesture_model;
    mubone::sensor::Receiver sensor_receiver;

    mubone::synthesis::GrainDescription gui_grain;
    mubone::synthesis::State gui_state;
    mubone::synthesis::State state;
    
    mubone::Audiosphere audiosphere;
    mubone::synthesis::CloudManager cloudmanager;

    std::size_t timeinsamples;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
