/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "CompressorBand.h"

namespace params
{
    enum Names
    {
        LOW_MID_CROSSOVER_FREQ,
        MID_HIGH_CROSSOVER_FREQ,
        BYPASS_GLOBAL,

        ATTACK_LOW,
        ATTACK_MID,
        ATTACK_HIGH,

        RELEASE_LOW,
        RELEASE_MID,
        RELEASE_HIGH,

        THRESHOLD_LOW,
        THRESHOLD_MID,
        THRESHOLD_HIGH,

        RATIO_LOW,
        RATIO_MID,
        RATIO_HIGH,

        BYPASS_LOW,
        BYPASS_MID,
        BYPASS_HIGH,

        MUTE_LOW,
        MUTE_MID,
        MUTE_HIGH,

        SOLO_LOW,
        SOLO_MID,
        SOLO_HIGH,
    };

    inline const std::map<Names, juce::String> getParams()
    {
        static std::map<Names, juce::String> params =
        {
            { LOW_MID_CROSSOVER_FREQ, "Low-Mid Crossover Frequency" },
            { MID_HIGH_CROSSOVER_FREQ, "Mid-High Crossover Frequency" },
            { BYPASS_GLOBAL, "Bypass Global" },
            { ATTACK_LOW, "Attack Low Band" },
            { ATTACK_MID, "Attack Mid Band" },
            { ATTACK_HIGH, "Attack High Band" },
            { RELEASE_LOW, "Release Low Band" },
            { RELEASE_MID, "Release Mid Band" },
            { RELEASE_HIGH, "Release High Band" },
            { THRESHOLD_LOW, "Threshold Low Band" },
            { THRESHOLD_MID, "Threshold Mid Band" },
            { THRESHOLD_HIGH, "Threshold High Band" },
            { RATIO_LOW, "Ratio Low Band" },
            { RATIO_MID, "Ratio Mid Band" },
            { RATIO_HIGH, "Ratio High Band" },
            { BYPASS_LOW, "Bypass Low Band" },
            { BYPASS_MID, "Bypass Mid Band" },
            { BYPASS_HIGH, "Bypass High Band" },
            { MUTE_LOW, "Mute Low Band" },
            { MUTE_MID, "Mute Mid Band" },
            { MUTE_HIGH, "Mute High Band" },
            { SOLO_LOW, "Solo Low Band" },
            { SOLO_MID, "Solo Mid Band" },
            { SOLO_HIGH, "Solo High Band" },
        };

        return params;
    }
}

//==============================================================================
/**
*/
class SimpleMBCompAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SimpleMBCompAudioProcessor();
    ~SimpleMBCompAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    using APVTS = juce::AudioProcessorValueTreeState;
    static APVTS::ParameterLayout createParameterLayout();

    APVTS apvts { *this, nullptr, "Parameters", createParameterLayout() };

private:
    std::array<CompressorBand, 3> compressorBands;
    CompressorBand& lowBandComp = compressorBands[0];
    CompressorBand& midBandComp = compressorBands[1];
    CompressorBand& highBandComp = compressorBands[2];

    using Filter = juce::dsp::LinkwitzRileyFilter<float>;

    //     cutoff 0   cutoff 1
    Filter lowpass0,  allpass1,
           highpass0, lowpass1,
                      highpass1;

    juce::AudioParameterFloat* lowMidCrossoverFreq{ nullptr };
    juce::AudioParameterFloat* midHighCrossoverFreq{ nullptr };
    juce::AudioParameterBool* globalBypass{ nullptr };

    std::array<juce::AudioBuffer<float>, 3> filterBuffers;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleMBCompAudioProcessor)
};
