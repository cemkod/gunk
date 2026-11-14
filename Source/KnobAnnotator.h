#pragma once

#include <JuceHeader.h>
#include <array>
#include "LookAndFeel.h"
#include "ModMatrix.h"
#include "SectionPanel.h"

// Keeps every knob's tooltip and mod dots current:
//   tooltip = full name, range, the knob's own description, and the slots that modulate it;
//   dots    = one per modulating slot, in the source's colour (see BassLookAndFeel::setModDots).
// Knobs are found by walking the component tree; mod targets come from setModTarget().
class KnobAnnotator
{
public:
    explicit KnobAnnotator (juce::AudioProcessorValueTreeState& apvts)
    {
        for (int i = 0; i < kSlots; ++i)
        {
            const juce::String n (i);
            slots[(size_t) i] = { apvts.getRawParameterValue ("modSlot" + n + "Source"),
                                  apvts.getRawParameterValue ("modSlot" + n + "Target"),
                                  apvts.getRawParameterValue ("modSlot" + n + "Amount") };
        }
    }

    // Collects the rotary knobs under root. Call once, after the UI is built.
    void build (juce::Component& root)
    {
        entries.clear();
        collect (root);
        refresh (true);
    }

    // Cheap when nothing changed: compares the 8 slots against the last snapshot.
    void refresh (bool force = false)
    {
        std::array<Snapshot, kSlots> now;
        for (int i = 0; i < kSlots; ++i)
        {
            auto& p = slots[(size_t) i];
            now[(size_t) i] = { (int) p.source->load(), (int) p.target->load() + 1,
                                juce::roundToInt (p.amount->load() * 100.0f) };
        }
        if (! force && now == last) return;
        last = now;

        for (auto& e : entries)
        {
            juce::String text = e.title + "\n"
                              + e.slider->getTextFromValue (e.slider->getMinimum()) + " to "
                              + e.slider->getTextFromValue (e.slider->getMaximum());
            if (e.description.isNotEmpty())
                text << "\n" << e.description;

            juce::Array<juce::Colour> dots;
            if (e.target > 0)
                for (int i = 0; i < kSlots; ++i)
                {
                    const auto& sn = now[(size_t) i];
                    if (sn.source == 0 || sn.target != e.target) continue;
                    dots.add (BassLookAndFeel::modSourceColour (sn.source));
                    const float amt = (float) sn.amountX100 / 100.0f;
                    text << "\nSlot " << (i + 1) << ": " << BassLookAndFeel::modSourceNames[sn.source]
                         << " " << (amt >= 0.0f ? "+" : "") << juce::String (amt, 2);
                }
            BassLookAndFeel::setModDots (*e.slider, dots);
            e.slider->setTooltip (text);
        }
    }

private:
    static constexpr int kSlots = 8;

    struct SlotParams { std::atomic<float>* source; std::atomic<float>* target; std::atomic<float>* amount; };
    struct Snapshot
    {
        int source = -1, target = -1, amountX100 = 0;
        bool operator== (const Snapshot& o) const { return source == o.source && target == o.target && amountX100 == o.amountX100; }
    };
    struct Entry
    {
        juce::Slider* slider = nullptr;
        int target = 0;
        juce::String title, description;
    };

    void collect (juce::Component& root)
    {
        if (auto* s = dynamic_cast<juce::Slider*> (&root); s != nullptr && s->isRotary())
        {
            Entry e;
            e.slider = s;
            e.target = BassLookAndFeel::getModTarget (*s);
            e.description = s->getTooltip();
            const auto name = BassLookAndFeel::getKnobName (*s);
            if (e.target > 0)
                e.title = ModMatrix::kTargetNames[e.target - 1];
            else if (auto* panel = root.findParentComponentOfClass<SectionPanel>())
                e.title = panel->getTitle() + "  " + name;
            else
                e.title = name;
            entries.push_back (e);
        }
        for (auto* c : root.getChildren())
            collect (*c);
    }

    std::array<SlotParams, kSlots> slots {};
    std::array<Snapshot, kSlots>   last {};
    std::vector<Entry> entries;
};
