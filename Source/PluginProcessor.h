/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <iostream>
#include <array>

//==============================================================================
/**
*/
template<typename T>
struct Fifo
{
	void prepare(int numChannels, int numSamples)
	{
		static_assert(std::is_same_v<T, juce::AudioBuffer<float>>,
			"prepare(numChannels, numSamples) should only be used when the Fifo is holding juce::AudioBuffer<float>");
		for (auto& buffer : buffers)
		{
			buffer.setSize(numChannels,
				numSamples,
				false, //clear everything?
				true, //including the extra space?
				true); //avoid reallocating if possible?
			buffer.clear();
		}
	}

	void prepare(size_t numElements)
	{
		static_assert(std::is_same_v<T, std::vector<float>>,
			"prepare(numElements) should only be used when the Fifo is holding std::vector<float>");
		for (auto& buffer : buffers)
		{
			buffer.clear();
			buffer.resize(numElements, 0);
		}
	}

	bool push(const T& t)
	{
		auto write = fifo.write(1);
		if (write.blockSize1 > 0)
		{
			buffers[write.startIndex1] = t;
			return true;
		}
		return false;
	}

	bool pull(T& t)
	{
		auto read = fifo.read(1);
		if (read.blockSize1 > 0)
		{
			t = buffers[read.startIndex1];
			return true;
		}
		return false;
	}

	int getNumAvailableForReading() const
	{
		return fifo.getNumReady();
	}
private:
	static constexpr int Capacity = 30;
	std::array<T, Capacity> buffers;
	juce::AbstractFifo fifo{ Capacity };
};

enum Channel
{
	Right,
	Left
};

template<typename BlockType>
struct SingleChannelSampleFifo
{
	SingleChannelSampleFifo(Channel ch) : channelToUse(ch)
	{
		prepared.set(false);
	}

	void update(const BlockType& buffer)
	{
		jassert(isPrepared());
		jassert(buffer.getNumChannels() > channelToUse);
		auto* channelPtr = buffer.getReadPointer(channelToUse);

		for (int i = 0; i < buffer.getNumSamples(); ++i)
		{
			pushNextSampleIntoFifo(channelPtr[i]);
		}
	}

	void prepare(int bufferSize)
	{
		prepared.set(false);
		size.set(bufferSize);

		bufferToFill.setSize(1, //channel
			bufferSize, //num samples
			false, //keepExistingContent
			true, //clear extra space
			true); //avoid reallocating
		audioBufferFifo.prepare(1, bufferSize);
		fifoIndex = 0;
		prepared.set(true);
	}
	//=============
	int getNumCompleteBuffersAvailable() const { return audioBufferFifo.getNumAvailableForReading(); }
	bool isPrepared() const { return prepared.get(); }
	int getSize() const { return size.get(); }
	bool getAudioBuffer(BlockType& buffer) { return audioBufferFifo.pull(buffer); }
	//=============
private:
	Channel channelToUse;
	int fifoIndex = 0;
	Fifo<BlockType> audioBufferFifo;
	BlockType bufferToFill;
	juce::Atomic<bool> prepared = false;
	juce::Atomic<int> size = 0;

	void pushNextSampleIntoFifo(float sample)
	{
		if (fifoIndex == bufferToFill.getNumSamples())
		{
			auto ok = audioBufferFifo.push(bufferToFill);
			juce::ignoreUnused(ok);
			fifoIndex = 0;
		}

		bufferToFill.setSample(0, fifoIndex, sample);;
		++fifoIndex;
	}
};

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
void updateBand(const ChainSettings& chainSettings, ChainType& chain, double sampleRate)
{
	auto& chainBand = chain.get<Index>();

	auto bandSettings = chainSettings.bandSettings[Index];

	switch (bandSettings.band_type)
	{
	case BandType::LowPass:
	{
		auto lowpass_coefficients = makeLowPassFilter(bandSettings, sampleRate);

		updateLowHighPassFilter(chainBand, lowpass_coefficients, bandSettings.band_slope);
		break;
	}
	case BandType::Peak:
	{
		auto peak_coefficients = makePeakFilter(bandSettings, sampleRate);

		updatePeakFilter(chainBand, peak_coefficients);
		break;
	}
	case BandType::HighPass:
	{
		auto highpass_coefficients = makeHighPassFilter(bandSettings, sampleRate);

		updateLowHighPassFilter(chainBand, highpass_coefficients, bandSettings.band_slope);
		break;
	}
	}
}

template<typename BandType, typename CoefficientsType>
void updatePeakFilter(BandType& band, CoefficientsType& coefficients)
{
	band.setBypassed<1>(true);
	band.setBypassed<2>(true);
	band.setBypassed<3>(true);

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

juce::String getParameterId(int bandNumber, juce::String bandParameter);

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

	using BlockType = juce::AudioBuffer<float>;
	SingleChannelSampleFifo<BlockType> leftChannelFifo{ Channel::Left };
	SingleChannelSampleFifo<BlockType> rightChannelFifo{ Channel::Right };

private:
	MonoChain leftChain, rightChain;

	void updateFilters();
	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParametricEQ2AudioProcessor)
};
