/*
  ==============================================================================

    BandThumbComponent.h
    Created: 13 Nov 2023 2:58:38pm
    Author:  natha

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class BandThumbComponent  : public juce::Component
{
public:
    BandThumbComponent();
    ~BandThumbComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    juce::ComponentDragger dragger;
    juce::ComponentBoundsConstrainer constrainer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BandThumbComponent)
};
