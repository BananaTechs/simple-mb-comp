#pragma once

#include <JuceHeader.h>

class CompressorBand
{
public:
    void setParams(juce::AudioProcessorValueTreeState& apvts);
    void prepare(juce::dsp::ProcessSpec& spec);
    void updateCompressorSettings();
    void process(juce::AudioBuffer<float>& buffer);

private:
    juce::AudioParameterFloat* attack{ nullptr };
    juce::AudioParameterFloat* release{ nullptr };
    juce::AudioParameterFloat* threshold{ nullptr };
    juce::AudioParameterChoice* ratio{ nullptr };
    juce::AudioParameterBool* bypass{ nullptr };

    juce::dsp::Compressor<float> compressor;
};