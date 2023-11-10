/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
enum BandType
{
	LowPass,
	Peak,
	HighPass
};

enum Slope
{
	Slope_12,
	Slope_24,
	Slope_36,
	Slope_48
};

struct BandSettings
{
	float band_freq{ 0 };
	float band_gain{ 0 };
	Slope band_slope{ Slope::Slope_12 };
	BandType band_type{ BandType::Peak };
};

struct ChainSettings
{
	BandSettings bandSettings[3] = {};
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

using Filter = juce::dsp::IIR::Filter<float>;

using BandFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

using MonoChain = juce::dsp::ProcessorChain<BandFilter, BandFilter, BandFilter>;

using Coefficients = Filter::CoefficientsPtr;
void updateCoefficients(Coefficients& old, const Coefficients& replacement);

void updateSingleFilter(const ChainSettings& chainSettings);

template<int Index, typename ChainType>
void updateBand(const ChainSettings& chainSettings, ChainType& leftChain, ChainType& rightChain, double sampleRate)
{
	auto& leftChainBand = leftChain.get<Index>();
	auto& rightChainBand = rightChain.get<Index>();

	switch (chainSettings.bandSettings[Index].band_type)
	{
	case BandType::LowPass:
		DBG("lowpass");
		break;
	case BandType::Peak:
		updatePeakFilter(chainSettings.bandSettings[Index], leftChainBand, sampleRate);
		updatePeakFilter(chainSettings.bandSettings[Index], rightChainBand, sampleRate);
		break;
	case BandType::HighPass:
		DBG("highpass");
		break;
	}
}

template<typename BandType>
void updatePeakFilter(const BandSettings& bandSettings, BandType& band, double sampleRate)
{
	auto peak_coefficients = makePeakFilter(bandSettings, sampleRate);
	updateCoefficients(band.get<0>().coefficients, peak_coefficients);
}

Coefficients makePeakFilter(const BandSettings& bandSettings, double sampleRate);

class ParametricEQ2AudioProcessor : public juce::AudioProcessor
#if JucePlugin_Enable_ARA
	, public juce::AudioProcessorARAExtension
#endif
{
public:
	//==============================================================================
	ParametricEQ2AudioProcessor();
	~ParametricEQ2AudioProcessor() override;

	//==============================================================================
	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

	void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

	//==============================================================================
	juce::AudioProcessorEditor* createEditor() override;
	bool hasEditor() const override;

	//==============================================================================
	const juce::String getName() const override;

	bool acceptsMidi() const override;
	bool producesMidi() const override;
	bool isMidiEffect() const override;
	double getTailLengthSeconds() const override;

	//==============================================================================
	int getNumPrograms() override;
	int getCurrentProgram() override;
	void setCurrentProgram(int index) override;
	const juce::String getProgramName(int index) override;
	void changeProgramName(int index, const juce::String& newName) override;

	//==============================================================================
	void getStateInformation(juce::MemoryBlock& destData) override;
	void setStateInformation(const void* data, int sizeInBytes) override;

	static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
	juce::AudioProcessorValueTreeState apvts{ *this, nullptr, "Parameters", createParameterLayout() };

private:
	MonoChain leftChain, rightChain;

	void updateFilters(const ChainSettings& chainSettings);
	void updateBandFilter(int index, const ChainSettings& chainSettings);

	juce::StringRef getParameterId(int bandNumber, juce::StringRef bandParameter);

	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParametricEQ2AudioProcessor)
};
