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

template<int Index, typename ChainType>
void updateBand(const ChainSettings& chainSettings, ChainType& leftChain, ChainType& rightChain, double sampleRate)
{
	auto& leftChainBand = leftChain.get<Index>();
	auto& rightChainBand = rightChain.get<Index>();

	auto bandSettings = chainSettings.bandSettings[Index];

	switch (bandSettings.band_type)
	{
	case BandType::LowPass:
	{
		auto lowpass_coefficients = makeLowPassFilter(bandSettings, sampleRate);

		updateLowHighPassFilter(leftChainBand, lowpass_coefficients, bandSettings.band_slope);
		updateLowHighPassFilter(rightChainBand, lowpass_coefficients, bandSettings.band_slope);
		break;
	}
	case BandType::Peak:
	{
		auto peak_coefficients = makePeakFilter(bandSettings, sampleRate);

		updatePeakFilter(leftChainBand, peak_coefficients);
		updatePeakFilter(rightChainBand, peak_coefficients);
		break;
	}
	case BandType::HighPass:
	{
		auto highpass_coefficients = makeHighPassFilter(bandSettings, sampleRate);

		updateLowHighPassFilter(leftChainBand, highpass_coefficients, bandSettings.band_slope);
		updateLowHighPassFilter(rightChainBand, highpass_coefficients, bandSettings.band_slope);
		break;
	}
	}
}

template<typename BandType, typename CoefficientsType>
void updatePeakFilter(BandType& band, CoefficientsType& coefficients)
{
	band.template setBypassed<1>(true);
	band.template setBypassed<2>(true);
	band.template setBypassed<3>(true);

	updateCoefficients(band.get<0>().coefficients, coefficients);
}

template<typename BandType, typename CoefficientsType>
void updateLowHighPassFilter(BandType& band, CoefficientsType& coefficients, Slope& slope) {
	band.setBypassed<0>(true);
	band.setBypassed<1>(true);
	band.setBypassed<2>(true);
	band.setBypassed<3>(true);

	switch (slope)
	{
	case Slope_48:
	{
		updateCoefficients(band.get<3>().coefficients, coefficients[3]);
		band.setBypassed<3>(false);
	}
	case Slope_36:
	{
		updateCoefficients(band.get<2>().coefficients, coefficients[2]);
		band.setBypassed<2>(false);
	}
	case Slope_24:
	{
		updateCoefficients(band.get<1>().coefficients, coefficients[1]);
		band.setBypassed<1>(false);
	}
	case Slope_12:
	{
		updateCoefficients(band.get<0>().coefficients, coefficients[0]);
		band.setBypassed<0>(false);
	}
	}
}

inline auto makePeakFilter(const BandSettings& bandSettings, double sampleRate) {
	return juce::dsp::IIR::Coefficients<float>::makePeakFilter(
		sampleRate,
		bandSettings.band_freq,
		1.f,
		juce::Decibels::decibelsToGain(bandSettings.band_gain)
	);
}

inline auto makeLowPassFilter(const BandSettings& bandSettings, double sampleRate) {
	return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(
		bandSettings.band_freq,
		sampleRate,
		2 * (bandSettings.band_slope + 1)
	);
}

inline auto makeHighPassFilter(const BandSettings& bandSettings, double sampleRate) {
	return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
		bandSettings.band_freq,
		sampleRate,
		2 * (bandSettings.band_slope + 1)
	);
}


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
