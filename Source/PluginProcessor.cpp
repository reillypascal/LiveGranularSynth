/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Utilities.h"

//==============================================================================
LiveGranularSynthAudioProcessor::LiveGranularSynthAudioProcessor()
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
    synth.addSound(new GranularSound());
    for (int i = 0; i < mNumVoices; ++i)
        synth.addVoice(new GranularVoice());
    
    synth.setNoteStealingEnabled(true);
}

LiveGranularSynthAudioProcessor::~LiveGranularSynthAudioProcessor()
{
}

//==============================================================================
const juce::String LiveGranularSynthAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool LiveGranularSynthAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool LiveGranularSynthAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool LiveGranularSynthAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double LiveGranularSynthAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int LiveGranularSynthAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int LiveGranularSynthAudioProcessor::getCurrentProgram()
{
    return 0;
}

void LiveGranularSynthAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String LiveGranularSynthAudioProcessor::getProgramName (int index)
{
    return {};
}

void LiveGranularSynthAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void LiveGranularSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.sampleRate = sampleRate;
    spec.numChannels = getTotalNumInputChannels();
    
    mCircularBuffer.prepare(spec);
    
    mReadPosition.resize(getTotalNumInputChannels());
    std::fill(mReadPosition.begin(), mReadPosition.end(), 0.0f);
    
    synth.setCurrentPlaybackSampleRate(sampleRate);
    
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto voice = dynamic_cast<GranularVoice*>(synth.getVoice(i)))
        {
            voice->prepareToPlay(sampleRate,
                                 samplesPerBlock,
                                 getTotalNumOutputChannels());
            
            voice->setReferencedBuffer(mCircularBuffer);
        }
    }
}

void LiveGranularSynthAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool LiveGranularSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void LiveGranularSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        mCircularBuffer.fillNextBlock(channel, buffer.getNumSamples(), channelData);
    }
    
    //setParams(); // includes setVoiceParams()
    updateGrainParams();
    
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
}

//==============================================================================
bool LiveGranularSynthAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* LiveGranularSynthAudioProcessor::createEditor()
{
    return new LiveGranularSynthAudioProcessorEditor (*this);
}

//==============================================================================
void LiveGranularSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void LiveGranularSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

void LiveGranularSynthAudioProcessor::updateGrainParams()
{
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto voice = dynamic_cast<GranularVoice*>(synth.getVoice(i)))
        {
            float attack = 50.0f;
            float release = 50.0f;
            
            auto& adsr = voice->getAdsr();
            
            adsr.update(attack, 0.0f, 1.0f, release);
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LiveGranularSynthAudioProcessor();
}
