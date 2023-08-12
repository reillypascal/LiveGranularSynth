#include "GranularSynth.h"

//==============================================================================
void AdsrData::update(const float attack, const float decay, const float sustain, const float release)
{
    adsrParams.attack = attack;
    adsrParams.decay = decay;
    adsrParams.sustain = sustain;
    adsrParams.release = release;
    
    setParameters(adsrParams);
}

//==============================================================================
GranularSound::GranularSound() {}

bool GranularSound::appliesToNote(int midiNoteNumber) { return true; }
bool GranularSound::appliesToChannel(int midiChannel) { return true; }

//==============================================================================
GranularVoice::GranularVoice() {}

bool GranularVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<GranularSound*>(sound) != nullptr;
}

void GranularVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int /*currentPitchWheenPosition*/)
{
    adsr.noteOn();
}

void GranularVoice::stopNote(float velocity, bool allowTailOff)
{
    adsr.noteOff();
    
    if (! allowTailOff || adsr.isActive())
        clearCurrentNote();
}

void GranularVoice::controllerMoved(int controllerNumber, int newControllerValue) {}

void GranularVoice::pitchWheelMoved(int newPitchWheelValue) {}

void GranularVoice::prepareToPlay(double sampleRate, int samplesPerBlock, int outputChannels)
{
    reset();
    
    adsr.setSampleRate(sampleRate);
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = outputChannels;
    
    for (int ch = 0; ch < numChannelsToProcess; ++ch)
    {
        
    }
    
    gain.prepare(spec);
    gain.setGainLinear(0.1f);
    
    isPrepared = true;
}

void GranularVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    jassert(isPrepared);
    
    if (! isVoiceActive())
        return;
    
    // prepare synthBuffer
    synthBuffer.setSize(outputBuffer.getNumChannels(), numSamples, false, false, true);
    
    synthBuffer.clear();
    
    // play from circular buffer
    for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
    {
        auto* channelData = outputBuffer.getWritePointer (channel);
        
        for (int sample = 0; sample < outputBuffer.getNumSamples(); ++sample)
        {
            channelData[sample] = mReferencedRawBuffer->getSample(channel, mReadPosition[channel]);

            mReadPosition[channel] += mPlaybackRate;
            mReadPosition[channel] = wrap(mReadPosition[channel], static_cast<float>(mGranBufferLength));
        }
    }
    
    // gain/env
    juce::dsp::AudioBlock<float> audioBlock { synthBuffer };
    gain.process(juce::dsp::ProcessContextReplacing<float>(audioBlock));
    adsr.applyEnvelopeToBuffer(synthBuffer, 0, synthBuffer.getNumSamples());
    
    for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
    {
        outputBuffer.addFrom( channel, startSample, synthBuffer, channel, 0, numSamples);
        
        if (! adsr.isActive())
            clearCurrentNote();
    }
}

void GranularVoice::reset()
{
    gain.reset();
    adsr.reset();
}

void GranularVoice::setReferencedBuffer(CircularBuffer<float>& circularBufferToReference)
{
    mReferencedRawBuffer = circularBufferToReference.getReferencedBuffer();
    
    mGranBufferLength = mReferencedRawBuffer->getNumSamples();
}
