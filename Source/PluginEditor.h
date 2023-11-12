/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct CustomVerticalSlider : juce::Slider
{
	CustomVerticalSlider():
		juce::Slider(
			juce::Slider::SliderStyle::LinearVertical,
			juce::Slider::TextEntryBoxPosition::NoTextBox
		)
	{

	}
};

struct CustomRotarySlider : juce::Slider
{
	CustomRotarySlider() :
		juce::Slider(
			juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
			juce::Slider::TextEntryBoxPosition::NoTextBox
		)
	{

	}
};

//==============================================================================
/**
*/
class ParametricEQ2AudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
	ParametricEQ2AudioProcessorEditor(ParametricEQ2AudioProcessor&);
	~ParametricEQ2AudioProcessorEditor() override;

	//==============================================================================
	void paint(juce::Graphics&) override;
	void resized() override;

private:
	// This reference is provided as a quick way for your editor to
	// access the processor object that created it.
	ParametricEQ2AudioProcessor& audioProcessor;

	CustomVerticalSlider band1GainVerticalSlider, band2GainVerticalSlider, band3GainVerticalSlider;

	CustomRotarySlider band1FreqRotarySlider, band2FreqRotarySlider, band3FreqRotarySlider;
	CustomRotarySlider band1BandWidthRotarySlider, band2BandWidthRotarySlider, band3BandWidthRotarySlider;

	std::vector<juce::Component*> getComponents();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParametricEQ2AudioProcessorEditor)
};
