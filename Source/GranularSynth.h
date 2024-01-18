#pragma once

#include <JuceHeader.h>
#include "CircularBuffer.h"
#include "Utilities.h"

//==============================================================================
class AdsrData : public juce::ADSR
{
public:
    void update(const float attack, const float decay, const float sustain, const float release);
    
private:
    juce::ADSR::Parameters adsrParams;
};

//==============================================================================
class GranularSound : public juce::SynthesiserSound
{
public:
    GranularSound();
    
    bool appliesToNote(int midiNoteNumber) override;
    bool appliesToChannel(int midiNoteNumber) override;
};

//==============================================================================
class GranularVoice : public juce::SynthesiserVoice
{
public:
    GranularVoice();
    
    bool canPlaySound (juce::SynthesiserSound* sound) override;
    
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* /*sound*/, int /*currentPitchWheenPosition*/) override;
    
    void stopNote(float /*velocity*/, bool allowTailOff) override;
    
    void controllerMoved (int controllerNumber, int newControllerValue) override;
    
    void pitchWheelMoved (int newPitchWheelValue) override;
    
    void prepareToPlay (double sampleRate, int samplesPerBlock, int outputChannels);
    
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;
    
    void reset();
    
    void setReferencedBuffer(CircularBuffer<float>& circularBufferToReference);
    
    //using juce::SynthesiserVoice::renderNextBlock;
    
    AdsrData& getAdsr() { return adsr; }
    
private:
    double level { 0.0 };
    double tailOff { 0.0 };
    
    static constexpr int numChannelsToProcess { 2 };
    AdsrData adsr;
    juce::AudioBuffer<float> synthBuffer;
    
    juce::dsp::Gain<float> gain;
    bool isPrepared { false };
    
    int mGranBufferLength = 44100;
    std::shared_ptr<juce::AudioBuffer<float>> mReferencedRawBuffer = nullptr;
    
    std::vector<float> mReadPosition { 0.0f, 0.0f };
    float mPlaybackRate = 1.5f;
};
