/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleMBCompAudioProcessor::SimpleMBCompAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    using namespace params;
    const auto& params = getParams();

    auto setFloatParam = [&apvts = this->apvts, &params](auto& param, const auto& paramName)
    {
        param = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(params.at(paramName)));
        jassert(param != nullptr);
    };

    auto setChoiceParam = [&apvts = this->apvts, &params](auto& param, const auto& paramName)
    {
        param = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(params.at(paramName)));
        jassert(param != nullptr);
    };

    auto setBoolParam = [&apvts = this->apvts, &params](auto& param, const auto& paramName)
    {
        param = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(params.at(paramName)));
        jassert(param != nullptr);
    };
    

    setFloatParam(lowBandComp.attack, Names::ATTACK_LOW);
    setFloatParam(midBandComp.attack, Names::ATTACK_MID);
    setFloatParam(highBandComp.attack, Names::ATTACK_HIGH);
    setFloatParam(lowBandComp.release, Names::RELEASE_LOW);
    setFloatParam(midBandComp.release, Names::RELEASE_MID);
    setFloatParam(highBandComp.release, Names::RELEASE_HIGH);
    setFloatParam(lowBandComp.threshold, Names::THRESHOLD_LOW);
    setFloatParam(midBandComp.threshold, Names::THRESHOLD_MID);
    setFloatParam(highBandComp.threshold, Names::THRESHOLD_HIGH);
    setChoiceParam(lowBandComp.ratio, Names::RATIO_LOW);
    setChoiceParam(midBandComp.ratio, Names::RATIO_MID);
    setChoiceParam(highBandComp.ratio, Names::RATIO_HIGH);
    setBoolParam(lowBandComp.bypass, Names::BYPASS_LOW);
    setBoolParam(midBandComp.bypass, Names::BYPASS_MID);
    setBoolParam(highBandComp.bypass, Names::BYPASS_HIGH);
    setBoolParam(lowBandComp.mute, Names::MUTE_LOW);
    setBoolParam(midBandComp.mute, Names::MUTE_MID);
    setBoolParam(highBandComp.mute, Names::MUTE_HIGH);
    setBoolParam(lowBandComp.solo, Names::SOLO_LOW);
    setBoolParam(midBandComp.solo, Names::SOLO_MID);
    setBoolParam(highBandComp.solo, Names::SOLO_HIGH);

    setFloatParam(lowMidCrossoverFreq, Names::LOW_MID_CROSSOVER_FREQ);
    setFloatParam(midHighCrossoverFreq, Names::MID_HIGH_CROSSOVER_FREQ);
    setBoolParam(globalBypass, Names::BYPASS_GLOBAL);

    lowpass0.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    highpass0.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
    lowpass1.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    highpass1.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
    allpass1.setType(juce::dsp::LinkwitzRileyFilterType::allpass);
}

SimpleMBCompAudioProcessor::~SimpleMBCompAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleMBCompAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleMBCompAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleMBCompAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleMBCompAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleMBCompAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleMBCompAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleMBCompAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleMBCompAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimpleMBCompAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimpleMBCompAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleMBCompAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.sampleRate = sampleRate;
    spec.numChannels = getTotalNumOutputChannels();

    lowBandComp.prepare(spec);
    midBandComp.prepare(spec);
    highBandComp.prepare(spec);
    lowpass0.prepare(spec);
    highpass0.prepare(spec);
    lowpass1.prepare(spec);
    highpass1.prepare(spec);
    allpass1.prepare(spec);

    for (auto& buffer : filterBuffers)
    {
        buffer.setSize(spec.numChannels, samplesPerBlock);
    }
}

void SimpleMBCompAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleMBCompAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
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

void SimpleMBCompAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    filterBuffers[0] = buffer;
    filterBuffers[1] = buffer;

    auto cutoff0 = lowMidCrossoverFreq->get();
    auto cutoff1 = midHighCrossoverFreq->get();

    lowpass0.setCutoffFrequency(cutoff0);
    highpass0.setCutoffFrequency(cutoff0);
    lowpass1.setCutoffFrequency(cutoff1);
    highpass1.setCutoffFrequency(cutoff1);
    allpass1.setCutoffFrequency(cutoff1);


    auto fb0Block = juce::dsp::AudioBlock<float>(filterBuffers[0]);
    auto fb1Block = juce::dsp::AudioBlock<float>(filterBuffers[1]);
    auto fb2Block = juce::dsp::AudioBlock<float>(filterBuffers[2]);
    auto fb0Ctx = juce::dsp::ProcessContextReplacing<float>(fb0Block);
    auto fb1Ctx = juce::dsp::ProcessContextReplacing<float>(fb1Block);
    auto fb2Ctx = juce::dsp::ProcessContextReplacing<float>(fb2Block);

    lowpass0.process(fb0Ctx);
    highpass0.process(fb1Ctx);

    buffer.clear();

    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    auto addToBuffer = [ns = numSamples, nc = numChannels](juce::AudioBuffer<float>& dest, const auto& source)
    {
        for (int i = 0; i < nc; i++)
        {
            dest.addFrom(i, 0, source, i, 0, ns);
        }
    };

    filterBuffers[2] = filterBuffers[1];

    lowpass1.process(fb1Ctx);
    highpass1.process(fb2Ctx);
    allpass1.process(fb0Ctx);

    jassert(compressorBands.size() == filterBuffers.size());

    if (globalBypass->get())
    {
        for (auto i = 0; i < filterBuffers.size(); i++)
        {
            addToBuffer(buffer, filterBuffers[i]);
        }
        return;
    }

    bool isSoloed = false;
    for (auto i = 0; i < compressorBands.size(); i++)
    {
        compressorBands[i].updateCompressorSettings();
        compressorBands[i].process(filterBuffers[i]);
        isSoloed |= compressorBands[i].solo->get();
    }

    for (auto i = 0; i < filterBuffers.size(); i++)
    {
        if (isSoloed && !compressorBands[i].solo->get())
        {
            continue;
        }

        addToBuffer(buffer, filterBuffers[i]);
    }
}

//==============================================================================
bool SimpleMBCompAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleMBCompAudioProcessor::createEditor()
{
    //return new SimpleMBCompAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SimpleMBCompAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void SimpleMBCompAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);

    if (tree.isValid())
    {
        apvts.replaceState(tree);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpleMBCompAudioProcessor::createParameterLayout()
{
    using namespace juce;
    using namespace params;
    APVTS::ParameterLayout layout;

    const auto& params = getParams();

    //==============================================================================

    layout.add(std::make_unique<AudioParameterBool>(
        params.at(Names::BYPASS_GLOBAL),
        params.at(Names::BYPASS_GLOBAL),
        false));

    layout.add(std::make_unique<AudioParameterFloat>(
        params.at(Names::LOW_MID_CROSSOVER_FREQ),
        params.at(Names::LOW_MID_CROSSOVER_FREQ),
        NormalisableRange<float>(20, 20000, 1, 0.25),
        500));

    layout.add(std::make_unique<AudioParameterFloat>(
        params.at(Names::MID_HIGH_CROSSOVER_FREQ),
        params.at(Names::MID_HIGH_CROSSOVER_FREQ),
        NormalisableRange<float>(20, 20000, 1, 0.5),
        3000));

    //==============================================================================

    auto attackRange = NormalisableRange<float>(1, 500, 1, 0.5);
    auto releaseRange = NormalisableRange<float>(1, 500, 1, 1);
    auto thresholdRange = NormalisableRange<float>(-60, 12, 1, 1);

    auto ratioChoices = std::vector<double>{ 1, 1.5, 2, 3, 4, 5, 6, 8, 12, 16, 24, 32, 64, 128 };
    juce::StringArray sa;
    for (auto choice : ratioChoices)
    {
        sa.add(juce::String(choice, 1));
    }

    //==============================================================================

    layout.add(std::make_unique<AudioParameterFloat>(
        params.at(Names::THRESHOLD_LOW),
        params.at(Names::THRESHOLD_LOW),
        thresholdRange,
        0));
    layout.add(std::make_unique<AudioParameterFloat>(
        params.at(Names::ATTACK_LOW),
        params.at(Names::ATTACK_LOW),
        attackRange,
        50));
    layout.add(std::make_unique<AudioParameterFloat>(
        params.at(Names::RELEASE_LOW),
        params.at(Names::RELEASE_LOW),
        releaseRange,
        250));
    layout.add(std::make_unique<AudioParameterChoice>(
        params.at(Names::RATIO_LOW),
        params.at(Names::RATIO_LOW),
        sa,
        3));
    layout.add(std::make_unique<AudioParameterBool>(
        params.at(Names::BYPASS_LOW),
        params.at(Names::BYPASS_LOW),
        false));
    layout.add(std::make_unique<AudioParameterBool>(
        params.at(Names::MUTE_LOW),
        params.at(Names::MUTE_LOW),
        false));
    layout.add(std::make_unique<AudioParameterBool>(
        params.at(Names::SOLO_LOW),
        params.at(Names::SOLO_LOW),
        false));

    layout.add(std::make_unique<AudioParameterFloat>(
        params.at(Names::THRESHOLD_MID),
        params.at(Names::THRESHOLD_MID),
        thresholdRange,
        0));
    layout.add(std::make_unique<AudioParameterFloat>(
        params.at(Names::ATTACK_MID),
        params.at(Names::ATTACK_MID),
        attackRange,
        50));
    layout.add(std::make_unique<AudioParameterFloat>(
        params.at(Names::RELEASE_MID),
        params.at(Names::RELEASE_MID),
        releaseRange,
        250));
    layout.add(std::make_unique<AudioParameterChoice>(
        params.at(Names::RATIO_MID),
        params.at(Names::RATIO_MID),
        sa,
        3));
    layout.add(std::make_unique<AudioParameterBool>(
        params.at(Names::BYPASS_MID),
        params.at(Names::BYPASS_MID),
        false));
    layout.add(std::make_unique<AudioParameterBool>(
        params.at(Names::MUTE_MID),
        params.at(Names::MUTE_MID),
        false));
    layout.add(std::make_unique<AudioParameterBool>(
        params.at(Names::SOLO_MID),
        params.at(Names::SOLO_MID),
        false));

    layout.add(std::make_unique<AudioParameterFloat>(
        params.at(Names::THRESHOLD_HIGH),
        params.at(Names::THRESHOLD_HIGH),
        thresholdRange,
        0));
    layout.add(std::make_unique<AudioParameterFloat>(
        params.at(Names::ATTACK_HIGH),
        params.at(Names::ATTACK_HIGH),
        attackRange,
        50));
    layout.add(std::make_unique<AudioParameterFloat>(
        params.at(Names::RELEASE_HIGH),
        params.at(Names::RELEASE_HIGH),
        releaseRange,
        250));
    layout.add(std::make_unique<AudioParameterChoice>(
        params.at(Names::RATIO_HIGH),
        params.at(Names::RATIO_HIGH),
        sa,
        3));
    layout.add(std::make_unique<AudioParameterBool>(
        params.at(Names::BYPASS_HIGH),
        params.at(Names::BYPASS_HIGH),
        false));
    layout.add(std::make_unique<AudioParameterBool>(
        params.at(Names::MUTE_HIGH),
        params.at(Names::MUTE_HIGH),
        false));
    layout.add(std::make_unique<AudioParameterBool>(
        params.at(Names::SOLO_HIGH),
        params.at(Names::SOLO_HIGH),
        false));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleMBCompAudioProcessor();
}
