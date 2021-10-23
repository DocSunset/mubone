#pragma once

#include "parameters.hpp"
#include "parameter_mapping.h"
#include "../JuceLibraryCode/JuceHeader.h"

using namespace mubone;
using namespace mubone::synthesis;

class GrainDisplayComponent : public juce::Component
{
    juce::Slider amp;
    juce::Slider freq;
    juce::Slider dur;
    juce::Slider skew;
    juce::Slider space;
    juce::Slider space_spray;
    juce::Slider deg;
    
public:
    void set_text_function(juce::Slider& slider)
    {
        auto f = [&](double value)
        {
            if (slider.getSliderStyle() != juce::Slider::SliderStyle::TwoValueVertical 
                    && slider.getSliderStyle() != juce::Slider::SliderStyle::TwoValueHorizontal)
                return juce::String("");
            auto suff = slider.getTextValueSuffix();
            auto sep = juce::String(",\n");
            auto s1 = juce::String(slider.getMinValue(), slider.getNumDecimalPlacesToDisplay());
            auto s2 = juce::String(slider.getMaxValue(), slider.getNumDecimalPlacesToDisplay());
            return s1 + suff + sep + s2;
        };
        slider.textFromValueFunction = f;
    }

    GrainDisplayComponent()
    :   amp (juce::Slider::SliderStyle::TwoValueVertical, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    ,   freq(juce::Slider::SliderStyle::TwoValueVertical, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    ,   dur (juce::Slider::SliderStyle::TwoValueVertical, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    ,   skew (juce::Slider::SliderStyle::TwoValueVertical, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    ,   space (juce::Slider::SliderStyle::LinearBarVertical, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    ,   space_spray (juce::Slider::SliderStyle::LinearBarVertical, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    ,   deg (juce::Slider::SliderStyle::LinearBarVertical, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    {
        amp.setVelocityBasedMode(true);
        freq.setVelocityBasedMode(true);
        dur.setVelocityBasedMode(true);
        skew.setVelocityBasedMode(true);
        space.setVelocityBasedMode(true);
        space_spray.setVelocityBasedMode(true);
        deg.setVelocityBasedMode(true);
        amp.setVelocityModeParameters(1, 10000, 0);
        freq.setVelocityModeParameters(1, 10000, 0);
        dur.setVelocityModeParameters(1, 10000, 0);
        deg.setVelocityModeParameters(1, 10000, 0);
        amp.setRange(-127, 0);
        freq.setRange(frequency_mapping(0), frequency_mapping(1));
        dur.setRange(duration_mapping(0) * 1000.0f, duration_mapping(1) * 1000.0f);
        skew.setRange(0, 1);
        space.setRange(0, 2);
        space_spray.setRange(0, 2);
        deg.setRange(0, 1);
        amp.setTextValueSuffix("dBFS");
        freq.setTextValueSuffix("Hz");
        dur.setTextValueSuffix("ms");
        skew.setTextValueSuffix("skew");
        space.setTextValueSuffix("width");
        space_spray.setTextValueSuffix("spray");
        deg.setTextValueSuffix("search units");
        amp.setNumDecimalPlacesToDisplay(3);
        freq.setNumDecimalPlacesToDisplay(3);
        dur.setNumDecimalPlacesToDisplay(0);
        skew.setNumDecimalPlacesToDisplay(3);
        space.setNumDecimalPlacesToDisplay(3);
        space_spray.setNumDecimalPlacesToDisplay(3);
        deg.setNumDecimalPlacesToDisplay(3);
        set_text_function(amp);
        set_text_function(freq);
        set_text_function(dur);
        set_text_function(skew);
        set_text_function(space);
        set_text_function(space_spray);
        set_text_function(deg);

        freq.setSkewFactorFromMidPoint(frequency_mapping(0.5));
        dur.setSkewFactorFromMidPoint(duration_mapping(0.5) * 1000.0f);

        addAndMakeVisible(amp);
        addAndMakeVisible(freq);
        addAndMakeVisible(dur);
        addAndMakeVisible(skew);
        addAndMakeVisible(space);
        addAndMakeVisible(space_spray);
        addAndMakeVisible(deg);
    }

    void paint (juce::Graphics& g) override {}

    void resized() override
    {
        constexpr float num_sliders = 6;
        juce::Rectangle<int> area(getLocalBounds());
        const auto width = area.getWidth() / num_sliders;
        const auto margin = 5;
        amp.setBounds(area.removeFromLeft(width).reduced(margin));
        freq.setBounds(area.removeFromLeft(width).reduced(margin));
        dur.setBounds(area.removeFromLeft(width).reduced(margin));
        skew.setBounds(area.removeFromLeft(width).reduced(margin));
        space.setBounds(area.removeFromLeft(width/2).reduced(margin));
        space_spray.setBounds(area.removeFromLeft(width/2).reduced(margin));
        deg.setBounds(area.reduced(margin));

        amp.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true, width,   35);
        freq.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true, width,  35);
        dur.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true, width,   35);
        skew.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true, width,  35);
        space.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true, width/2, 35);
        space_spray.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true, width/2, 35);
        deg.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true, width,   35);
    }

    void update(const GrainDescription& gd)
    {
        amp.setMinAndMaxValues(get<amplitude_min>(gd), get<amplitude_max>(gd), juce::dontSendNotification);
        freq.setMinAndMaxValues(get<frequency_min>(gd), get<frequency_max>(gd), juce::dontSendNotification);
        dur.setMinAndMaxValues(get<duration_min>(gd), get<duration_max>(gd), juce::dontSendNotification);
        skew.setMinAndMaxValues(get<skew_min>(gd), get<skew_max>(gd), juce::dontSendNotification);
        space.setValue(get<spatial_width>(gd), juce::dontSendNotification);
        space_spray.setValue(get<spatial_spray>(gd), juce::dontSendNotification);
        deg.setValue(get<search_radius>(gd), juce::dontSendNotification);

        amp.updateText();
        freq.updateText();
        dur.updateText();
        skew.updateText();
        space.updateText();
        space_spray.updateText();
        deg.updateText();
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
