/*
  ==============================================================================

    BandThumbComponent.cpp
    Created: 13 Nov 2023 2:58:38pm
    Author:  natha

  ==============================================================================
*/

#include <JuceHeader.h>
#include "BandThumbComponent.h"

void drawCircleCenter(juce::Graphics& g, float x, float y, float radius)
{
    g.fillEllipse(x - radius / 2, y - radius / 2, radius, radius);
}

//==============================================================================
BandThumbComponent::BandThumbComponent(ParametricEQ2AudioProcessor& p, int index) : 
    audioProcessor(p), 
    bandIndex(index)
{
    // In your constructor, you should add any child components, and
    // initialise any special settings that your component needs.
}

BandThumbComponent::~BandThumbComponent()
{
}

void BandThumbComponent::paint (juce::Graphics& g)
{
    /* This demo code just fills the component's background and
       draws some placeholder text to get you started.

       You should replace everything in this method with your own
       drawing code..
    */
    auto bounds = getLocalBounds();

    g.setColour(juce::Colours::yellow );
    g.fillEllipse(bounds.toFloat());
}

void BandThumbComponent::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..
    auto bounds = getLocalBounds();

    jassert(bounds.getHeight() == bounds.getWidth());

    thumbSize = bounds.getHeight();

    constrainer.setMinimumOnscreenAmounts(thumbSize, thumbSize, thumbSize, thumbSize);
}

void BandThumbComponent::mouseDown(const juce::MouseEvent& event)
{
    dragger.startDraggingComponent(this, event);
    setMouseCursor(juce::MouseCursor::NoCursor);
    
}

void BandThumbComponent::mouseDrag(const juce::MouseEvent& event)
{
    dragger.dragComponent(this, event, &constrainer);
}

void BandThumbComponent::mouseUp(const juce::MouseEvent& event)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void BandThumbComponent::setPosition(float xCenter, float yCenter)
{
    setBounds(xCenter - thumbSize / 2, yCenter - thumbSize / 2, thumbSize, thumbSize);
}

