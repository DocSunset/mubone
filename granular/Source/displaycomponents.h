#pragma once

#include "parameters.hpp"
#include "../JuceLibraryCode/JuceHeader.h"

using namespace mubone;
using namespace mubone::synthesis;

class GrainDisplayComponent : public juce::Component
{
    juce::Slider amp;
    juce::Slider freq;
    juce::Slider dur;
    juce::Slider deg;
    
public:
    GrainDisplayComponent()
    :   amp (juce::Slider::SliderStyle::LinearBarVertical, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    ,   freq(juce::Slider::SliderStyle::LinearBarVertical, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    ,   dur (juce::Slider::SliderStyle::LinearBarVertical, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    ,   deg (juce::Slider::SliderStyle::LinearBarVertical, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    {
        amp.setVelocityBasedMode(true);
        freq.setVelocityBasedMode(true);
        dur.setVelocityBasedMode(true);
        deg.setVelocityBasedMode(true);
        amp.setVelocityModeParameters(1, 10000, 0);
        freq.setVelocityModeParameters(1, 10000, 0);
        dur.setVelocityModeParameters(1, 10000, 0);
        deg.setVelocityModeParameters(1, 10000, 0);
        amp.setRange(0, 1);
        freq.setRange(0, 20000);
        dur.setRange(0, 60);
        deg.setRange(0, 1);
        amp.setTextValueSuffix("amp");
        freq.setTextValueSuffix("Hz");
        dur.setTextValueSuffix("sec");
        deg.setTextValueSuffix("units");
        addAndMakeVisible(amp);
        addAndMakeVisible(freq);
        addAndMakeVisible(dur);
        addAndMakeVisible(deg);
    }

    void paint (juce::Graphics& g) override {}

    void resized() override
    {
        juce::Rectangle<int> area(getLocalBounds());
        const auto width = area.getWidth() / 4;
        const auto margin = 5;
        amp.setBounds(area.removeFromLeft(width).reduced(margin));
        freq.setBounds(area.removeFromLeft(width).reduced(margin));
        dur.setBounds(area.removeFromLeft(width).reduced(margin));
        deg.setBounds(area.reduced(margin));
    }

    void update(const GrainDescription& gd)
    {
        amp.setValue(get<amplitude_min>(gd), juce::dontSendNotification);
        freq.setValue(get<frequency_min>(gd), juce::dontSendNotification);
        dur.setValue(get<duration_min>(gd), juce::dontSendNotification);
        deg.setValue(get<search_radius>(gd), juce::dontSendNotification);
    }
};

using mubone::synthesis::State;
class StateDisplayComponent : public juce::Component
{
    juce::TextButton sound_rec_indicator;
    juce::TextButton gestr_rec_indicator;
    juce::TextButton granulate_indicator;
    
public:
    StateDisplayComponent()
    {
        // add subcomponents for displaying state parameters
        sound_rec_indicator.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);
        gestr_rec_indicator.setColour(juce::TextButton::buttonOnColourId, juce::Colours::mediumpurple);
        granulate_indicator.setColour(juce::TextButton::buttonOnColourId, juce::Colours::green);
        addAndMakeVisible(sound_rec_indicator);
        addAndMakeVisible(gestr_rec_indicator);
        addAndMakeVisible(granulate_indicator);
    }

    void paint (juce::Graphics& g) override {}

    void resized() override
    {
        juce::Rectangle<int> area(getLocalBounds());
        const auto width = area.getWidth() / 3;
        const auto margin = 5;

        sound_rec_indicator.setBounds(area.removeFromLeft(width).reduced(margin));
        gestr_rec_indicator.setBounds(area.removeFromLeft(width).reduced(margin));
        granulate_indicator.setBounds(area.reduced(margin));
    }

    void update(const State& s)
    {
        // set slider values
        sound_rec_indicator.setToggleState(get<sound_recording>(s), juce::dontSendNotification);
        gestr_rec_indicator.setToggleState(get<gesture_recording>(s), juce::dontSendNotification);
        granulate_indicator.setToggleState(get<granulating>(s), juce::dontSendNotification);
    }
};

// TODO: 
//  - Trombone Orientation Display
//  - Audiosphere / sounds display
//  - cloud manager display
//  - Bring cloud manager and audiosphere display into separate classes
