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

struct CustomChoiceSlider : juce::Slider
{
	CustomChoiceSlider() :
		juce::Slider(
			juce::Slider::SliderStyle::RotaryVerticalDrag,
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

	CustomChoiceSlider band1SlopeChoiceSlider, band2SlopeChoiceSlider, band3SlopeChoiceSlider;
	CustomChoiceSlider band1TypeChoiceSlider, band2TypeChoiceSlider, band3TypeChoiceSlider;

	using APVTS = juce::AudioProcessorValueTreeState;
	using Attachment = APVTS::SliderAttachment;

	Attachment band1GainVerticalSliderAttachment, band2GainVerticalSliderAttachment, band3GainVerticalSliderAttachment;
	Attachment band1FreqRotarySliderAttachment, band2FreqRotarySliderAttachment, band3FreqRotarySliderAttachment;

	Attachment band1SlopeChoiceSliderAttachment, band2SlopeChoiceSliderAttachment, band3SlopeChoiceSliderAttachment;
	Attachment band1TypeChoiceSliderAttachment, band2TypeChoiceSliderAttachment, band3TypeChoiceSliderAttachment;

	std::vector<juce::Component*> getComponents();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParametricEQ2AudioProcessorEditor)
};
