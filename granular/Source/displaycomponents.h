#pragma once

#include "parameters.hpp"
#include "../JuceLibraryCode/JuceHeader.h"

using namespace mubone;
using namespace mubone::synthesis;

class GrainDisplayComponent : public Component
{
    Slider amp;
    Slider freq;
    Slider dur;
    Slider deg;
    
public:
    GrainDisplayComponent()
    :   amp (Slider::SliderStyle::LinearBarVertical, Slider::TextEntryBoxPosition::TextBoxBelow)
    ,   freq(Slider::SliderStyle::LinearBarVertical, Slider::TextEntryBoxPosition::TextBoxBelow)
    ,   dur (Slider::SliderStyle::LinearBarVertical, Slider::TextEntryBoxPosition::TextBoxBelow)
    ,   deg (Slider::SliderStyle::LinearBarVertical, Slider::TextEntryBoxPosition::TextBoxBelow)
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

    void paint (Graphics& g) override {}

    void resized() override
    {
        Rectangle<int> area(getLocalBounds());
        const auto width = area.getWidth() / 4;
        const auto margin = 5;
        amp.setBounds(area.removeFromLeft(width).reduced(margin));
        freq.setBounds(area.removeFromLeft(width).reduced(margin));
        dur.setBounds(area.removeFromLeft(width).reduced(margin));
        deg.setBounds(area.reduced(margin));
    }

    void update(const GrainDescription& gd)
    {
        amp.setValue(get<amplitude>(gd), dontSendNotification);
        freq.setValue(get<frequency>(gd), dontSendNotification);
        dur.setValue(get<duration>(gd), dontSendNotification);
        deg.setValue(get<search_radius>(gd), dontSendNotification);
    }
};

using mubone::synthesis::State;
class StateDisplayComponent : public Component
{
    TextButton sound_rec_indicator;
    TextButton gestr_rec_indicator;
    TextButton granulate_indicator;
    
public:
    StateDisplayComponent()
    {
        // add subcomponents for displaying state parameters
        sound_rec_indicator.setColour(TextButton::buttonOnColourId, Colours::red);
        gestr_rec_indicator.setColour(TextButton::buttonOnColourId, Colours::mediumpurple);
        granulate_indicator.setColour(TextButton::buttonOnColourId, Colours::green);
        addAndMakeVisible(sound_rec_indicator);
        addAndMakeVisible(gestr_rec_indicator);
        addAndMakeVisible(granulate_indicator);
    }

    void paint (Graphics& g) override {}

    void resized() override
    {
        Rectangle<int> area(getLocalBounds());
        const auto width = area.getWidth() / 3;
        const auto margin = 5;

        sound_rec_indicator.setBounds(area.removeFromLeft(width).reduced(margin));
        gestr_rec_indicator.setBounds(area.removeFromLeft(width).reduced(margin));
        granulate_indicator.setBounds(area.reduced(margin));
    }

    void update(const State& s)
    {
        // set slider values
        sound_rec_indicator.setToggleState(get<sound_recording>(s), dontSendNotification);
        gestr_rec_indicator.setToggleState(get<gesture_recording>(s), dontSendNotification);
        granulate_indicator.setToggleState(get<granulating>(s), dontSendNotification);
    }
};

// TODO: 
//  - Trombone Orientation Display
//  - Audiosphere / sounds display
//  - cloud manager display
//  - Bring cloud manager and audiosphere display into separate classes
