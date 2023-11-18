/*
  ==============================================================================

    ResponseCurveComponent.cpp
    Created: 13 Nov 2023 12:35:43pm
    Author:  natha

  ==============================================================================
*/

#include <JuceHeader.h>
#include "ResponseCurveComponent.h"
#include "PluginEditor.h"


//==============================================================================
ResponseCurveComponent::ResponseCurveComponent(ParametricEQ2AudioProcessor& p) : audioProcessor(p),
thumbs{ BandThumbComponent(p, 0), BandThumbComponent(p, 1), BandThumbComponent(p, 2) },
leftChannelFifo(&audioProcessor.leftChannelFifo)
{
    // In your constructor, you should add any child components, and
    // initialise any special settings that your component needs.
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->addListener(this);
    }

    for (size_t i = 0; i < sizeof(thumbs) / sizeof(thumbs[0]); ++i) {
        addAndMakeVisible(thumbs[i]);
    }

    updateResponseCurve();
    updateThumbsFromParameters();

    leftChannelFFTDataGenerator.changeOrder(FFTOrder::order8k);
    monoBuffer.setSize(1, leftChannelFFTDataGenerator.getFFTSize());

    startTimer(30);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->removeListener(this);
    }
}

void ResponseCurveComponent::paint (juce::Graphics& g)
{
    auto responseArea = getLocalBounds();

    //FFT lines
    const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();

    const auto binWidth = audioProcessor.getSampleRate() / (double)fftSize;

    bool drawn = false;

    while (leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0) //Do we have more than 0 fft blocks available
    {
        std::vector<float> fftData; //Will contain fft data if we are able to pull a fft block

        if (leftChannelFFTDataGenerator.getFFTData(fftData)) //Try to pull a fft block
        {
            if (!drawn)
                drawFFTLines(g, fftData, fftSize, binWidth, -48.f);
            drawn = true;
        }
    }

    drawResultingResponseCurve(g);

    g.setColour(juce::Colours::grey);
    g.drawRect(responseArea.toFloat(), 1.f);
}

void ResponseCurveComponent::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..
    auto bounds = getLocalBounds();

    auto chainSettings = getChainSettings(audioProcessor.apvts);
    for (size_t i = 0; i < sizeof(thumbs) / sizeof(thumbs[0]); ++i) {
        auto freq = chainSettings.bandSettings[i].band_freq;
        auto x = juce::mapFromLog10((double)freq, 20.0, 20000.0) * bounds.getWidth();

        thumbs[i].setBounds(x - thumbSize/2, bounds.getHeight() / 2 - thumbSize/2, thumbSize, thumbSize);
        thumbs[i].setColour(getColourScheme()[i]);
    }
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void ResponseCurveComponent::timerCallback()
{
    juce::AudioBuffer<float> tempIncomingBuffer;

    while (leftChannelFifo->getNumCompleteBuffersAvailable() > 0)
    {
        if (leftChannelFifo->getAudioBuffer(tempIncomingBuffer));
        {
            auto size = tempIncomingBuffer.getNumSamples();
            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0),
                monoBuffer.getReadPointer(0, size),
                monoBuffer.getNumSamples() - size);

            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
                tempIncomingBuffer.getReadPointer(0, 0),
                size);

            leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);
        }
    }

    if (parametersChanged.compareAndSetBool(false, true))
    {
        updateResponseCurve();
        updateThumbsFromParameters();
    }

    repaint();

}

void ResponseCurveComponent::drawResultingResponseCurve(juce::Graphics& g)
{
    using namespace juce;

    auto responseArea = getLocalBounds();

    auto sampleRate = audioProcessor.getSampleRate();
    auto width = responseArea.getWidth();

    std::vector<double> magnitudes;
    magnitudes.resize(width);

    for (int i = 0; i < width; ++i) {
        double magnitude = 1.f;

        auto freq = mapToLog10(double(i) / double(width), 20.0, 20000.0);

        magnitude *= getBandMagnitudeForFrequency(monoChain.get<0>(), freq, sampleRate);
        magnitude *= getBandMagnitudeForFrequency(monoChain.get<1>(), freq, sampleRate);
        magnitude *= getBandMagnitudeForFrequency(monoChain.get<2>(), freq, sampleRate);

        magnitudes[i] = Decibels::gainToDecibels(magnitude);
    }

    Path responseCurve;

    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
        {
            return jmap(input, -24.0, 24.0, outputMin, outputMax);
        };

    responseCurve.startNewSubPath(0, responseArea.getBottom());
    responseCurve.lineTo(responseArea.getX(), map(magnitudes.front()));

    for (size_t i = 1; i < magnitudes.size(); ++i)
    {
        responseCurve.lineTo(responseArea.getX() + i, map(magnitudes[i]));
    }

    responseCurve.lineTo(responseArea.getRight(), responseArea.getBottom());
    responseCurve.closeSubPath();

    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));

    g.setColour(Colour::fromFloatRGBA(1, 1, 1, 0.1));
    g.fillPath(responseCurve);
}

void ResponseCurveComponent::updateResponseCurve()
{
    auto chainSettings = getChainSettings(audioProcessor.apvts);
    updateBand<0>(chainSettings, monoChain, audioProcessor.getSampleRate());
    updateBand<1>(chainSettings, monoChain, audioProcessor.getSampleRate());
    updateBand<2>(chainSettings, monoChain, audioProcessor.getSampleRate());
}

void ResponseCurveComponent::updateThumbsFromParameters()
{
    using namespace juce;

    auto chainSettings = getChainSettings(audioProcessor.apvts);
    auto responseArea = getLocalBounds();
    auto width = responseArea.getWidth();

    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
        {
            return jmap(input, -24.0, 24.0, outputMin, outputMax);
        };

    for (int i = 0; i < 3; ++i) {
        float freq = chainSettings.bandSettings[i].band_freq;

        auto x = mapFromLog10((double)freq, 20.0, 20000.0) * width;
        auto y = map(chainSettings.bandSettings[i].band_gain);

        thumbs[i].setPosition(x, y);
    }
}

void ResponseCurveComponent::drawFFTLines(juce::Graphics& g,
    const std::vector<float>& renderData,
    int fftSize,
    float binWidth,
    float negativeInfinity)
{
    auto fftBounds = getLocalBounds().toFloat();
    auto top = fftBounds.getY();
    auto bottom = fftBounds.getBottom();
    auto width = fftBounds.getWidth();

    int numBins = (int)fftSize / 2;

    auto map = [bottom, top, negativeInfinity](float v)
        {
            return juce::jmap(v,
                negativeInfinity, 0.f,
                float(bottom), top);
        };

    auto y = map(renderData[0]);

    jassert(!std::isnan(y) && !std::isinf(y));

    const int pathResolution = 1;

    int lastPixelDrawnX = 0;

    for (int binNum = 1; binNum < numBins; binNum += pathResolution)
    {
        y = map(renderData[binNum]);
        jassert(!std::isnan(y) && !std::isinf(y));

        if (!std::isnan(y) && !std::isinf(y))
        {
            auto binFreq = binNum * binWidth;
            auto normalizedBinX = juce::mapFromLog10(binFreq, 20.f, 20000.f);
            int binX = std::floor(normalizedBinX * width);


            if (std::round(binX) > lastPixelDrawnX)
            {
                float alpha = (y) / getHeight();

                g.setColour(juce::Colour::fromFloatRGBA(1.f, 0, 0, 1 - alpha));
                g.drawVerticalLine(binX, top, bottom);

                lastPixelDrawnX = std::round(binX);
            }
        }
    }
}