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
    bool saveUserPreset (const juce::String& name,
                         const juce::String& wt1Path = {},
                         const juce::String& wt2Path = {},
                         const juce::String& transientPath = {},
                         const juce::String& irPath = {});
    bool deleteUserPreset (int index);

    // Index `delta` steps from the current preset, wrapping at both ends; -1 if there are none.
    int getAdjacentIndex (int delta) const;

    int getCurrentIndex() const { return currentIndex; }
    void setCurrentIndex (int i) { currentIndex = i; }

    // Replaces the APVTS state from xml. APVTS leaves a parameter that has no
    // <PARAM> child at its previous value, so this adds an empty child for each
    // missing parameter first, which APVTS resets to the default.
    static void replaceStateFilling (juce::AudioProcessorValueTreeState& apvts,
                                     const juce::XmlElement& xml);

    static juce::File getFactoryPresetsDir();
    static juce::File getUserPresetsDir();

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::vector<PresetEntry> presets;
    int currentIndex = -1;

    void addPresetsFromDir (const juce::File& dir, bool isFactory);
};
