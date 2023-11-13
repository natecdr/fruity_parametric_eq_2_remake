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
    /* This demo code just fills the component's background and
       draws some placeholder text to get you started.

       You should replace everything in this method with your own
       drawing code..
    */
    using namespace juce;

    auto responseArea = getLocalBounds();
    //auto responseArea = bounds.removeFromLeft(bounds.getWidth() * 0.66);
    ////responseArea.reduce(10, 10);

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

void ResponseCurveComponent::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..

}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void ResponseCurveComponent::timerCallback()
{
    if (parametersChanged.compareAndSetBool(false, true))
    {
        auto chainSettings = getChainSettings(audioProcessor.apvts);
        updateBand<0>(chainSettings, monoChain, audioProcessor.getSampleRate());
        updateBand<1>(chainSettings, monoChain, audioProcessor.getSampleRate());
        updateBand<2>(chainSettings, monoChain, audioProcessor.getSampleRate());

        repaint();
    }
}
