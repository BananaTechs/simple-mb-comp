#pragma once

#include "CompressorBand.h"
#include "PluginProcessor.h"

void CompressorBand::setParams(juce::AudioProcessorValueTreeState& apvts)
{
    attack = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(SimpleMBCompAudioProcessor::ATTACK));
    release = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(SimpleMBCompAudioProcessor::RELEASE));
    threshold = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(SimpleMBCompAudioProcessor::THRESHOLD));
    ratio = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(SimpleMBCompAudioProcessor::RATIO));
    bypass = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(SimpleMBCompAudioProcessor::BYPASS));
}

void CompressorBand::prepare(juce::dsp::ProcessSpec& spec)
{
    compressor.prepare(spec);
}

void CompressorBand::updateCompressorSettings()
{
    compressor.setAttack(attack->get());
    compressor.setRelease(release->get());
    compressor.setThreshold(threshold->get());
    compressor.setRatio(ratio->getCurrentChoiceName().getFloatValue());
}

void CompressorBand::process(juce::AudioBuffer<float>& buffer)
{
    auto block = juce::dsp::AudioBlock<float>(buffer);
    auto context = juce::dsp::ProcessContextReplacing<float>(block);

    context.isBypassed = bypass->get();

    compressor.process(context);
}