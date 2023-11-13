/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ParametricEQ2AudioProcessorEditor::ParametricEQ2AudioProcessorEditor(ParametricEQ2AudioProcessor& p)
	: AudioProcessorEditor(&p), audioProcessor(p),
	band1GainVerticalSliderAttachment(audioProcessor.apvts, "band1_gain", band1GainVerticalSlider),
	band2GainVerticalSliderAttachment(audioProcessor.apvts, "band2_gain", band2GainVerticalSlider),
	band3GainVerticalSliderAttachment(audioProcessor.apvts, "band3_gain", band3GainVerticalSlider),

	band1FreqRotarySliderAttachment(audioProcessor.apvts, "band1_freq", band1FreqRotarySlider),
	band2FreqRotarySliderAttachment(audioProcessor.apvts, "band2_freq", band2FreqRotarySlider),
	band3FreqRotarySliderAttachment(audioProcessor.apvts, "band3_freq", band3FreqRotarySlider),

	band1SlopeChoiceSliderAttachment(audioProcessor.apvts, "band1_slope", band1SlopeChoiceSlider),
	band2SlopeChoiceSliderAttachment(audioProcessor.apvts, "band2_slope", band2SlopeChoiceSlider),
	band3SlopeChoiceSliderAttachment(audioProcessor.apvts, "band3_slope", band3SlopeChoiceSlider),

	band1TypeChoiceSliderAttachment(audioProcessor.apvts, "band1_type", band1TypeChoiceSlider),
	band2TypeChoiceSliderAttachment(audioProcessor.apvts, "band2_type", band2TypeChoiceSlider),
	band3TypeChoiceSliderAttachment(audioProcessor.apvts, "band3_type", band3TypeChoiceSlider)
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
	using namespace juce;
	// (Our component is opaque, so we must completely fill the background with a solid colour)
	g.fillAll(juce::Colours::black);

	auto bounds = getLocalBounds();
	auto responseArea = bounds.removeFromLeft(bounds.getWidth() * 0.66);
	responseArea.reduce(10, 10);

	auto sampleRate = audioProcessor.getSampleRate();
	auto width = responseArea.getWidth();

	std::vector<double> magnitudes;
	magnitudes.resize(width);

	for (int i = 0; i < width; ++i) {
		double magnitude = 1.f;

		auto freq = mapToLog10(double(i) / double(width), 20.0, 20000.0);

		magnitude *= getBandMagnitudeForFrequency(monoChain.get<0>(), 1000.0, sampleRate);
		magnitude *= getBandMagnitudeForFrequency(monoChain.get<1>(), 1000.0, sampleRate);
		magnitude *= getBandMagnitudeForFrequency(monoChain.get<2>(), 1000.0, sampleRate);

		magnitudes[i] = Decibels::gainToDecibels(magnitude);
	}

	Path responseCurve;

	const double outputMin = responseArea.getBottom();
	const double outputMax = responseArea.getY();
	auto map = [outputMin, outputMax](double input)
		{
			return jmap(input, -24.0, 24.0, outputMin, outputMax);
		};

	responseCurve.startNewSubPath(responseArea.getX(), map(magnitudes.front()));

	for (size_t i = 1; i < magnitudes.size(); ++i)
	{
		responseCurve.lineTo(responseArea.getX() + i, map(magnitudes[i]));
	}

	g.setColour(Colours::grey);
	g.drawRect(responseArea.toFloat(), 1.f);

	g.setColour(Colours::white);
	g.strokePath(responseCurve, PathStrokeType(2.f));
}

void ParametricEQ2AudioProcessorEditor::resized()
{
	// This is generally where you'll want to lay out the positions of any
	auto bounds = getLocalBounds();

	auto paramsArea = bounds.removeFromRight(bounds.getWidth() * 0.33);

	auto bottomParamsArea = paramsArea.removeFromBottom(paramsArea.getHeight() * 0.2);
	auto topParamsArea = paramsArea.removeFromTop(paramsArea.getHeight() * 0.2);

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

	auto topParamsTypeArea = topParamsArea.removeFromTop(topParamsArea.getHeight() * 0.5);
	auto topParamsSlopeArea = topParamsArea;

	band1TypeChoiceSlider.setBounds(topParamsTypeArea.removeFromLeft(topParamsTypeArea.getWidth() * 0.33));
	band2TypeChoiceSlider.setBounds(topParamsTypeArea.removeFromLeft(topParamsTypeArea.getWidth() * 0.5));
	band3TypeChoiceSlider.setBounds(topParamsTypeArea.removeFromLeft(topParamsTypeArea.getWidth()));

	band1SlopeChoiceSlider.setBounds(topParamsSlopeArea.removeFromLeft(topParamsSlopeArea.getWidth() * 0.33));
	band2SlopeChoiceSlider.setBounds(topParamsSlopeArea.removeFromLeft(topParamsSlopeArea.getWidth() * 0.5));
	band3SlopeChoiceSlider.setBounds(topParamsSlopeArea.removeFromLeft(topParamsSlopeArea.getWidth()));
}

void ParametricEQ2AudioProcessorEditor::parameterValueChanged(int parameterIndex, float newValue)
{

}

void ParametricEQ2AudioProcessorEditor::timerCallback()
{

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
		&band3BandWidthRotarySlider,

		&band1SlopeChoiceSlider,
		&band2SlopeChoiceSlider,
		&band3SlopeChoiceSlider,

		&band1TypeChoiceSlider,
		&band2TypeChoiceSlider,
		&band3TypeChoiceSlider,
	};
}