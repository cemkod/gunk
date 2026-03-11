#pragma once
#include <JuceHeader.h>

#ifdef ENABLE_DEBUG_LOG
inline void dbgLog (const juce::String& msg)
{
    auto f = juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                .getChildFile ("bass-synth-debug.log");
    f.appendText ("[" + juce::Time::getCurrentTime().toString (true, true, true, true) + "] "
                  + msg + "\n");
}
#else
inline void dbgLog (const juce::String&) {}
#endif
