/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ParametricEQ2AudioProcessorEditor::ParametricEQ2AudioProcessorEditor(ParametricEQ2AudioProcessor& p)
	: AudioProcessorEditor(&p), audioProcessor(p)
{
	// Make sure that before the constructor has finished, you've set the
	// editor's size to whatever you need it to be.
	for (auto* component : getComponents())
	{
		addAndMakeVisible(component);
	}
	
	setSize(600, 300);
}

ParametricEQ2AudioProcessorEditor::~ParametricEQ2AudioProcessorEditor()
{
}

//==============================================================================
void ParametricEQ2AudioProcessorEditor::paint(juce::Graphics& g)
{
	// (Our component is opaque, so we must completely fill the background with a solid colour)
	g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

	g.setColour(juce::Colours::white);
	g.setFont(15.0f);
	g.fillRect(getLocalBounds());
}

void ParametricEQ2AudioProcessorEditor::resized()
{
	// This is generally where you'll want to lay out the positions of any
	auto bounds = getLocalBounds();

	auto paramsArea = bounds.removeFromRight(bounds.getWidth() * 0.33);

	auto bottomParamsArea = paramsArea.removeFromBottom(paramsArea.getHeight() * 0.2);

	band1GainVerticalSlider.setBounds(paramsArea.removeFromLeft(paramsArea.getWidth() * 0.33));
	band2GainVerticalSlider.setBounds(paramsArea.removeFromLeft(paramsArea.getWidth() * 0.5));
	band3GainVerticalSlider.setBounds(paramsArea.removeFromLeft(paramsArea.getWidth()));

	auto bottomParamsFreqArea = bottomParamsArea.removeFromTop(bottomParamsArea.getHeight() * 0.5);
	auto bottomParamsBandWidthArea = bottomParamsArea;

	band1FreqRotarySlider.setBounds(bottomParamsFreqArea.removeFromLeft(bottomParamsFreqArea.getWidth() * 0.33));
	band2FreqRotarySlider.setBounds(bottomParamsFreqArea.removeFromLeft(bottomParamsFreqArea.getWidth() * 0.5));
	band3FreqRotarySlider.setBounds(bottomParamsFreqArea.removeFromLeft(bottomParamsFreqArea.getWidth()));

	band1BandWidthRotarySlider.setBounds(bottomParamsBandWidthArea.removeFromLeft(bottomParamsBandWidthArea.getWidth() * 0.33));
	band2BandWidthRotarySlider.setBounds(bottomParamsBandWidthArea.removeFromLeft(bottomParamsBandWidthArea.getWidth() * 0.5));
	band3BandWidthRotarySlider.setBounds(bottomParamsBandWidthArea.removeFromLeft(bottomParamsBandWidthArea.getWidth()));
}

std::vector<juce::Component*> ParametricEQ2AudioProcessorEditor::getComponents()
{
	return
	{
		&band1GainVerticalSlider,
		&band2GainVerticalSlider,
		&band3GainVerticalSlider,

		&band1FreqRotarySlider,
		&band2FreqRotarySlider,
		&band3FreqRotarySlider,

		&band1BandWidthRotarySlider,
		&band2BandWidthRotarySlider,
		&band3BandWidthRotarySlider
	};
}