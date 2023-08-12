/*
  ==============================================================================

 - need gain (from copyFromWithRamp)?

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

template <typename SampleType>
class CircularBuffer
{
public:
    CircularBuffer(int bufferSize)
    {
        jassert(bufferSize >= 0);
        
        mTotalSize = bufferSize > 4 ? bufferSize : 4;
        mCircularBuffer.get()->setSize(static_cast<int>(mCircularBuffer.get()->getNumChannels()), mTotalSize, false, false, false);
        mCircularBuffer.get()->clear();
        mNumSamples = mCircularBuffer.get()->getNumSamples();
    }
    
    ~CircularBuffer() = default;
    
    //==============================================================================
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        jassert(spec.numChannels > 0);
        
        mCircularBuffer.get()->setSize(static_cast<int>(spec.numChannels), mTotalSize, false, false, false);
        
        mWritePosition.resize(spec.numChannels);
        
        mSampleRate = spec.sampleRate;
        
        reset();
    }
    
    //==============================================================================
    void reset()
    {
        std::fill(mWritePosition.begin(), mWritePosition.end(), 0);
        
        mCircularBuffer.get()->clear();
    }
    
    //==============================================================================
    void fillNextBlock(int channel, const int inBufferLength, const SampleType* inBufferData)
    {
        if (mCircularBuffer.get()->getNumSamples() > inBufferLength + mWritePosition.at(channel))
        {
            mCircularBuffer.get()->copyFromWithRamp(channel, mWritePosition.at(channel), inBufferData, inBufferLength, 1, 1);
        }
        else
        {
            const int bufferRemaining = mCircularBuffer.get()->getNumSamples() - mWritePosition.at(channel);
            
            mCircularBuffer.get()->copyFromWithRamp(channel, mWritePosition.at(channel), inBufferData, bufferRemaining, 1, 1);
            mCircularBuffer.get()->copyFromWithRamp(channel, 0, inBufferData, inBufferLength - bufferRemaining, 1, 1);
        }
        
        mWritePosition.at(channel) += inBufferLength;
        mWritePosition.at(channel) %= mTotalSize;
    }
    
    //==============================================================================
    SampleType readSample(int channel, SampleType readPosition)
    {
        // look at DelayLine implementation
        SampleType intpart;
        readPosFrac = modf(readPosition, &intpart);
        readPosInt = static_cast<int>(intpart);
        
        int index1 = readPosInt;
        int index2 = readPosInt + 1;
        
        if (index2 >= mTotalSize)
        {
            index1 %= mTotalSize;
            index2 %= mTotalSize;
        }
        
        SampleType value1 = mCircularBuffer.get()->getSample(channel, index1);
        SampleType value2 = mCircularBuffer.get()->getSample(channel, index2);
        
        // add difference between samples scaled by position between them
        return value1 + (readPosFrac * (value2 - value1));
    }
    
    //==============================================================================
    int getBufferSize()
    {
        return mCircularBuffer.get()->getNumSamples();
    }
    
    //==============================================================================
    const juce::String getName() const { return "CircularBuffer"; };
    
    //==============================================================================
    std::shared_ptr<juce::AudioBuffer<SampleType>> getReferencedBuffer() { return mCircularBuffer; }
    
private:
    std::shared_ptr<juce::AudioBuffer<SampleType>> mCircularBuffer = std::make_shared<juce::AudioBuffer<SampleType>>();
    
    std::vector<int> mWritePosition { 0, 0 };
    int mSampleRate { 44100 };
    
    int mNumSamples { 0 };
    int mTotalSize { 0 };
    
    SampleType readPosFrac { 0 };
    int readPosInt { 0 };
};
