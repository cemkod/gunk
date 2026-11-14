#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "GateSectionComponent.h"
#include "CombinedOscSectionComponent.h"
#include "FilterSectionComponent.h"
#include "SubOscSectionComponent.h"
#include "TransientSectionComponent.h"
#include "EnvelopeSectionComponent.h"
#include "OutputSectionComponent.h"
#include "IRSectionComponent.h"
#include "DriveSectionComponent.h"
#include "LFOSectionComponent.h"

// The MAIN screen: every section, laid out in signal-flow order.
class MainViewComponent : public juce::Component
{
public:
    explicit MainViewComponent (JQGunkAudioProcessor& p);

    void resized() override;

    // Called from the editor timer while this view is visible.
    void tick (bool gateOpen, bool transientFired);

private:
    JQGunkAudioProcessor& processor;

    GateSectionComponent          gateSection;
    EnvelopeSectionComponent      envelopeSection;
    FilterSectionComponent        filterSection;
    SubOscSectionComponent        subOscSection;
    CombinedOscSectionComponent   combinedOscSection;
    TransientSectionComponent     transientSection;
    IRSectionComponent            irSection;
    DriveSectionComponent         driveSection;
    OutputSectionComponent        outputSection;
    LFOSectionComponent           lfoSection;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainViewComponent)
};
