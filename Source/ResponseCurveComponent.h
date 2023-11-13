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

void drawCircleCenter(juce::Graphics& g, float x, float y, float radius);

//==============================================================================
/*
*/
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
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResponseCurveComponent)
};
