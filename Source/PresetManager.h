#pragma once
#include <JuceHeader.h>

class PresetManager
{
public:
    struct PresetEntry {
        juce::String name;
        juce::File   file;
        bool         isFactory;
    };

    explicit PresetManager (juce::AudioProcessorValueTreeState& apvts);

    void rescanPresets();
    const std::vector<PresetEntry>& getPresets() const { return presets; }

    bool loadPreset (int index);
    bool saveUserPreset (const juce::String& name);
    bool deleteUserPreset (int index);

    int getCurrentIndex() const { return currentIndex; }
    void setCurrentIndex (int i) { currentIndex = i; }

    static juce::File getFactoryPresetsDir();
    static juce::File getUserPresetsDir();

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::vector<PresetEntry> presets;
    int currentIndex = -1;

    void addPresetsFromDir (const juce::File& dir, bool isFactory);
};
