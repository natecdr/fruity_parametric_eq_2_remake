/*
  ==============================================================================

    ResponseCurveComponent.h
    Created: 13 Nov 2023 12:35:43pm
    Author:  natha

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "BandThumbComponent.h"

void drawCircleCenter(juce::Graphics& g, float x, float y, float radius);

//==============================================================================
/*
*/
enum FFTOrder
{
	order2048 = 11,
	order4096 = 12,
	order8192 = 13
};

template<typename BlockType>
struct FFTDataGenerator
{
	void produceFFTDataForRendering(const juce::AudioBuffer<float>& audioData, const float negativeInfinity)
	{
		const auto fftSize = getFFTSize();

		fftData.assign(fftData.size(), 0);
		auto* readIndex = audioData.getReadPointer(0);
		std::copy(readIndex, readIndex + fftSize, fftData.begin());

		//apply windowing function
		window->multiplyWithWindowingTable(fftData.data(), fftSize);

		//render FFT data
		forwardFFT->performFrequencyOnlyForwardTransform(fftData.data());

		int numBins = (int)fftSize / 2;

		//normalize fft values
		for (int i = 0; i < numBins; ++i)
		{
			fftData[i] /= (float)numBins;
		}

		//convert  to dB
		for (int i = 0; i < numBins; ++i)
		{
			fftData[i] = juce::Decibels::gainToDecibels(fftData[i], negativeInfinity);
		}

		fftDataFifo.push(fftData);
	}

	void changeOrder(FFTOrder newOrder)
	{
		//when changing order, recreate the window, forwardFFT, fifo, fftData, fifoIndex

		order = newOrder;
		auto fftSize = getFFTSize();

		forwardFFT = std::make_unique<juce::dsp::FFT>(order);
		window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);

		fftData.clear();
		fftData.resize(fftSize * 2, 0);
		fftDataFifo.prepare(fftData.size());
	}
	int getFFTSize() const { return 1 << order; }
	int getNumAvailableFFTDataBlocks() const { return fftDataFifo.getNumAvailableForReading(); }
	bool getFFTData(BlockType& fftData) { return fftDataFifo.pull(fftData); }
private:
	FFTOrder order;
	BlockType fftData;
	std::unique_ptr<juce::dsp::FFT> forwardFFT;
	std::unique_ptr<juce::dsp::WindowingFunction<float>> window;

	Fifo<BlockType> fftDataFifo;
};

//template<typename PathType>
//struct AnalyzerPathGenerator
//{
//	void generatePath(const std::vector<float>& renderData,
//		juce::Rectangle<float> fftBounds,
//		int fftSize,
//		float binWidth,
//		float negativeInfinity)
//	{
//		auto top = fftBounds.getY();
//		auto bottom = fftBounds.getBottom();
//		auto width = fftBounds.getWidth();
//
//		int numBins = (int)fftSize / 2;
//
//		PathType p;
//		p.preallocateSpace(3 * (int)fftBounds.getWidth());
//
//		auto map = [bottom, top, negativeInfinity](float v)
//			{
//				return juce::jmap(v,
//					negativeInfinity, 0.f,
//					float(bottom), top);
//			};
//
//		auto y = map(renderData[0]);
//
//		jassert(!std::isnan(y) && !std::isinf(y));
//
//		p.startNewSubPath(0, y);
//
//		const int pathResolution = 2;
//
//		for (int binNum = 1; binNum < numBins; binNum += pathResolution)
//		{
//			y = map(renderData[binNum]);
//			jassert(!std::isnan(y) && !std::isinf(y));
//
//			if (!std::isnan(y) && !std::isinf(y))
//			{
//				auto binFreq = binNum * binWidth;
//				auto normalizedBinX = juce::mapFromLog10(binFreq, 20.f, 20000.f);
//				int binX = std::floor(normalizedBinX * width);
//				p.lineTo(binX, y);
//			}
//		}
//
//		pathFifo.push(p);
//	}
//
//	int getNumPathsAvailable() const
//	{
//		return pathFifo.getNumAvailableForReading();
//	}
//
//	bool getPath(PathType& path)
//	{
//		return pathFifo.pull(path);
//	}
//private:
//	Fifo<PathType> pathFifo;
//};

class ResponseCurveComponent  : public juce::Component,
    juce::AudioProcessorParameter::Listener,
    juce::Timer
{
public:
    ResponseCurveComponent(ParametricEQ2AudioProcessor& p);
    ~ResponseCurveComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void parameterValueChanged(int parameterIndex, float newValue);
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}
    void timerCallback() override;

private:
    ParametricEQ2AudioProcessor& audioProcessor;

    juce::Atomic<bool> parametersChanged{ false };
    MonoChain monoChain;

    BandThumbComponent thumbs[3];
    static constexpr float thumbSize = 30.f;

    void updateResponseCurve();
    void updateThumbsFromParameters();
    void drawResultingResponseCurve(juce::Graphics& g);

	SingleChannelSampleFifo<ParametricEQ2AudioProcessor::BlockType>* leftChannelFifo;

	juce::AudioBuffer<float> monoBuffer;

	FFTDataGenerator<std::vector<float>> leftChannelFFTDataGenerator;

	//AnalyzerPathGenerator<juce::Path> pathProducer;

	juce::Path leftChannelFFTPath;

	void drawFFTLines(juce::Graphics& g,
		const std::vector<float>& renderData,
		int fftSize,
		float binWidth,
		float negativeInfinity);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResponseCurveComponent)
};
