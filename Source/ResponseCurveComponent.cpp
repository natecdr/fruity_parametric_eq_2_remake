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
ResponseCurveComponent::ResponseCurveComponent(ParametricEQ2AudioProcessor& p) : audioProcessor(p)
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

    startTimer(60);
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
    using namespace juce;

    auto responseArea = getLocalBounds();

    g.setColour(Colours::grey);
    g.drawRect(responseArea.toFloat(), 1.f);

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

    auto chainSettings = getChainSettings(audioProcessor.apvts);

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

    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));

    for (int i = 0; i < 3; ++i) {
        float freq = chainSettings.bandSettings[i].band_freq;

        auto x = mapFromLog10((double)freq, 20.0, 20000.0) * width;

        auto y = map(chainSettings.bandSettings[i].band_gain);
        drawCircleCenter(g, x, y, 5.f);
    }
}

void ResponseCurveComponent::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..
    auto bounds = getLocalBounds();

    for (size_t i = 0; i < sizeof(thumbs) / sizeof(thumbs[0]); ++i) {
        thumbs[i].setBounds((i + 1) * bounds.getWidth() / 4, bounds.getHeight() / 2 - thumbSize/2, thumbSize, thumbSize);
    }
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void ResponseCurveComponent::timerCallback()
{
    if (parametersChanged.compareAndSetBool(false, true))
    {
        updateResponseCurve();

        repaint();
    }
}

void ResponseCurveComponent::updateResponseCurve()
{
    auto chainSettings = getChainSettings(audioProcessor.apvts);
    updateBand<0>(chainSettings, monoChain, audioProcessor.getSampleRate());
    updateBand<1>(chainSettings, monoChain, audioProcessor.getSampleRate());
    updateBand<2>(chainSettings, monoChain, audioProcessor.getSampleRate());
}