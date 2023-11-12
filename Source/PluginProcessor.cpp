/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ParametricEQ2AudioProcessor::ParametricEQ2AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
	: AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
		.withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
		.withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
	)
#endif
{
}

ParametricEQ2AudioProcessor::~ParametricEQ2AudioProcessor()
{
}

//==============================================================================
const juce::String ParametricEQ2AudioProcessor::getName() const
{
	return JucePlugin_Name;
}

bool ParametricEQ2AudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
	return true;
#else
	return false;
#endif
}

bool ParametricEQ2AudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
	return true;
#else
	return false;
#endif
}

bool ParametricEQ2AudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
	return true;
#else
	return false;
#endif
}

double ParametricEQ2AudioProcessor::getTailLengthSeconds() const
{
	return 0.0;
}

int ParametricEQ2AudioProcessor::getNumPrograms()
{
	return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
	// so this should be at least 1, even if you're not really implementing programs.
}

int ParametricEQ2AudioProcessor::getCurrentProgram()
{
	return 0;
}

void ParametricEQ2AudioProcessor::setCurrentProgram(int index)
{
}

const juce::String ParametricEQ2AudioProcessor::getProgramName(int index)
{
	return {};
}

void ParametricEQ2AudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void ParametricEQ2AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	juce::dsp::ProcessSpec spec;

	spec.maximumBlockSize = samplesPerBlock;

	spec.numChannels = 1;

	spec.sampleRate = sampleRate;

	leftChain.prepare(spec);
	rightChain.prepare(spec);

	auto chainSettings = getChainSettings(apvts);
}

void ParametricEQ2AudioProcessor::releaseResources()
{
	// When playback stops, you can use this as an opportunity to free up any
	// spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ParametricEQ2AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
	juce::ignoreUnused(layouts);
	return true;
#else
	// This is the place where you check if the layout is supported.
	// In this template code we only support mono or stereo.
	// Some plugin hosts, such as certain GarageBand versions, will only
	// load plugins that support stereo bus layouts.
	if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
		&& layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
		return false;

	// This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;
#endif

	return true;
#endif
}
#endif

void ParametricEQ2AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	juce::ScopedNoDenormals noDenormals;
	auto totalNumInputChannels = getTotalNumInputChannels();
	auto totalNumOutputChannels = getTotalNumOutputChannels();

	// In case we have more outputs than inputs, this code clears any output
	// channels that didn't contain input data, (because these aren't
	// guaranteed to be empty - they may contain garbage).
	// This is here to avoid people getting screaming feedback
	// when they first compile a plugin, but obviously you don't need to keep
	// this code if your algorithm always overwrites all the output channels.
	for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

	auto chainSettings = getChainSettings(apvts);

	updateFilters();

	juce::dsp::AudioBlock<float> block(buffer);
	auto leftBlock = block.getSingleChannelBlock(0);
	auto rightBlock = block.getSingleChannelBlock(1);

	juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
	juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

	leftChain.process(leftContext);
	rightChain.process(rightContext);
}

//==============================================================================
bool ParametricEQ2AudioProcessor::hasEditor() const
{
	return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ParametricEQ2AudioProcessor::createEditor()
{
	//return new ParametricEQ2AudioProcessorEditor(*this);
	return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void ParametricEQ2AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
	// You should use this method to store your parameters in the memory block.
	// You could do that either as raw data, or use the XML or ValueTree classes
	// as intermediaries to make it easy to save and load complex data.
	juce::MemoryOutputStream mos(destData, true);
	apvts.state.writeToStream(mos);
}

void ParametricEQ2AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
	// You should use this method to restore your parameters from this memory block,
	// whose contents will have been created by the getStateInformation() call.
	auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
	if (tree.isValid())
	{
		apvts.replaceState(tree);
		updateFilters();
	}
}

void ParametricEQ2AudioProcessor::updateFilters()
{
	auto chainSettings = getChainSettings(apvts);

	updateBand<0>(chainSettings, leftChain, rightChain, getSampleRate());
	updateBand<1>(chainSettings, leftChain, rightChain, getSampleRate());
	updateBand<2>(chainSettings, leftChain, rightChain, getSampleRate());
}

void updateCoefficients(Coefficients& old, const Coefficients& replacement)
{
	*old = *replacement;
}

juce::String getParameterId(int bandNumber, juce::String bandParameter)
{
	juce::String str;
	return str << "band" << bandNumber << "_" << bandParameter;
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
	ChainSettings settings;

	for (int i = 0; i < sizeof(settings.bandSettings) / sizeof(settings.bandSettings[0]); ++i) {
		settings.bandSettings[i].band_freq = apvts.getRawParameterValue(getParameterId(i + 1, "freq"))->load();
		settings.bandSettings[i].band_gain = apvts.getRawParameterValue(getParameterId(i + 1, "gain"))->load();
		settings.bandSettings[i].band_slope = static_cast<Slope>(apvts.getRawParameterValue(getParameterId(i + 1, "slope"))->load());
		settings.bandSettings[i].band_type = static_cast<BandType>(apvts.getRawParameterValue(getParameterId(i + 1, "type"))->load());
	}

	return settings;
}

juce::AudioProcessorValueTreeState::ParameterLayout ParametricEQ2AudioProcessor::createParameterLayout()
{
	juce::AudioProcessorValueTreeState::ParameterLayout layout;

	//Band Frequencies
	layout.add(
		std::make_unique<juce::AudioParameterFloat>(
			"band1_freq",
			"Band 1 Freq",
			juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
			20.f
		)
	);

	layout.add(
		std::make_unique<juce::AudioParameterFloat>(
			"band2_freq",
			"Band 2 Freq",
			juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
			1000.f
		)
	);

	layout.add(
		std::make_unique<juce::AudioParameterFloat>(
			"band3_freq",
			"Band 3 Freq",
			juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
			20000.f
		)
	);

	//Band gains
	layout.add(
		std::make_unique<juce::AudioParameterFloat>(
			"band1_gain",
			"Band 1 Gain",
			juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
			0.f
		)
	);

	layout.add(
		std::make_unique<juce::AudioParameterFloat>(
			"band2_gain",
			"Band 2 Gain",
			juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
			0.f
		)
	);

	layout.add(
		std::make_unique<juce::AudioParameterFloat>(
			"band3_gain",
			"Band 3 Gain",
			juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
			0.f
		)
	);

	//Band Types
	juce::StringArray bandTypes;
	bandTypes.add("Low Pass");
	bandTypes.add("Band Pass");
	bandTypes.add("High Pass");

	layout.add(
		std::make_unique<juce::AudioParameterChoice>(
			"band1_type",
			"Band 1 Type",
			bandTypes,
			1
		)
	);

	layout.add(
		std::make_unique<juce::AudioParameterChoice>(
			"band2_type",
			"Band 2 Type",
			bandTypes,
			1
		)
	);

	layout.add(
		std::make_unique<juce::AudioParameterChoice>(
			"band3_type",
			"Band 3 Type",
			bandTypes,
			1
		)
	);

	//Band slopes
	juce::StringArray bandSlopes;
	for (int i = 0; i < 4; ++i) {
		juce::String str;
		str << (12 + i * 12);
		str << " db/Oct";
		bandSlopes.add(str);
	}

	layout.add(
		std::make_unique<juce::AudioParameterChoice>(
			"band1_slope",
			"Band 1 Slope",
			bandSlopes,
			0
		)
	);

	layout.add(
		std::make_unique<juce::AudioParameterChoice>(
			"band2_slope",
			"Band 2 Slope",
			bandSlopes,
			0
		)
	);

	layout.add(
		std::make_unique<juce::AudioParameterChoice>(
			"band3_slope",
			"Band 3 Slope",
			bandSlopes,
			0
		)
	);

	return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new ParametricEQ2AudioProcessor();
}
