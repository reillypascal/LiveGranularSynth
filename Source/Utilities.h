#pragma once

#include <JuceHeader.h>

inline float wrap(float a, float b)
{
    float remainder = a - (floor(a/b) * b);
    
    return (remainder < 0) ? remainder + b : remainder;
}

inline float scale(float input, float inLow, float inHi, float outLow, float outHi)
{
    float scaleFactor = (outHi - outLow)/(inHi - inLow);
    float offset = outLow - inLow;
    return (input * scaleFactor) + offset;
}
