#include "PresetManager.h"

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& apvts_)
    : apvts (apvts_)
{
    rescanPresets();
}

//==============================================================================
juce::File PresetManager::getFactoryPresetsDir()
{
    juce::File userDir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                             .getChildFile ("JQGunk/Factory Presets");
    if (userDir.isDirectory())
        return userDir;

    // Fallback for system-wide package installs (e.g. .deb / .rpm)
    juce::File systemDir ("/usr/share/JQGunk/Factory Presets");
    if (systemDir.isDirectory())
        return systemDir;

    return userDir; // default even if absent (install-presets target will populate it)
}

juce::File PresetManager::getUserPresetsDir()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("JQGunk/User Presets");
}

//==============================================================================
void PresetManager::rescanPresets()
{
    presets.clear();
    addPresetsFromDir (getFactoryPresetsDir(), true);
    addPresetsFromDir (getUserPresetsDir(),    false);
}

void PresetManager::addPresetsFromDir (const juce::File& dir, bool isFactory)
{
    if (! dir.isDirectory()) return;
    auto files = dir.findChildFiles (juce::File::findFiles, false, "*.jqgpreset");
    files.sort();
    for (auto& f : files)
        presets.push_back ({ f.getFileNameWithoutExtension(), f, isFactory });
}

//==============================================================================
bool PresetManager::loadPreset (int index)
{
    if (index < 0 || index >= (int) presets.size()) return false;
    auto xml = juce::XmlDocument::parse (presets[index].file);
    if (! xml || ! xml->hasTagName (apvts.state.getType())) return false;
    apvts.replaceState (juce::ValueTree::fromXml (*xml));
    currentIndex = index;
    return true;
}

bool PresetManager::saveUserPreset (const juce::String& name)
{
    auto dir = getUserPresetsDir();
    dir.createDirectory();
    auto file = dir.getChildFile (name + ".jqgpreset");
    auto state = apvts.copyState();
    auto xml = state.createXml();
    if (! xml) return false;
    xml->removeAttribute ("customWavetablePath");
    return xml->writeTo (file);
}

bool PresetManager::deleteUserPreset (int index)
{
    if (index < 0 || index >= (int) presets.size()) return false;
    if (presets[index].isFactory) return false;
    bool ok = presets[index].file.deleteFile();
    if (ok)
    {
        presets.erase (presets.begin() + index);
        currentIndex = -1;
    }
    return ok;
}
