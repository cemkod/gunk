#pragma once

namespace UIConst {
    // Editor dimensions
    constexpr int editorWidth    = 994;
    constexpr int editorHeight   = 613;
    constexpr int editorPad      = 8;
    constexpr int headerHeight   = 52;
    constexpr int labelRowH      = 14;
    constexpr int presetStripH   = 24;
    constexpr int sectionGap     = 6;
    constexpr int oscSectionH    = 305;
    constexpr int topRowH        = 202;  // gate+envelope+filter+output row
    constexpr int oscColW        = 155;  // fixed column width

    // Gate LED
    constexpr int gateLedSize    = 8;
    constexpr int gateLedPad     = 6;

    // Timers
    constexpr int uiTimerHz      = 30;

    // LookAndFeel
    constexpr float sectionCornerRadius  = 4.0f;
    // Knob proportions, as fractions of the knob's outer radius / body radius
    constexpr float knobArcStrokeRatio    = 0.16f;  // arc thickness / outer radius
    constexpr float knobBodyGapRatio      = 0.08f;  // gap between arc and body / outer radius
    constexpr float knobPointerInnerRatio = 0.30f;  // pointer start / body radius
    constexpr float knobPointerOuterRatio = 0.92f;  // pointer end / body radius
    constexpr float knobPointerWidthRatio = 0.09f;  // pointer thickness / outer radius
    constexpr int   knobNameH             = 12;     // name label row inside a knob cell
    constexpr int   knobValueH            = 13;     // value text row inside a knob cell
    constexpr float linearTrackH         = 5.0f;
    constexpr float linearThumbW         = 7.0f;
    constexpr int   powerToggleW         = 26;
    constexpr int   powerToggleH         = 12;
    constexpr int   sliderTextBoxW       = 50;
    constexpr int   sliderTextBoxH       = 16;
    constexpr float uiFontSize           = 10.0f;
    constexpr float buttonCornerRadius   = 4.0f;
    constexpr float comboBoxCornerRadius = 4.0f;

    // Section header bar
    constexpr int   sectionHeaderH   = 18;
    constexpr int   sectionInnerPad  = 6;
    constexpr float fontSectionTitle = 11.0f;
    constexpr float fontKnobLabel    = 9.0f;
    constexpr float fontKnobValue    = 10.0f;
    // Barlow's line height is ~1.2 em, so it renders smaller than the old system font
    // at the same JUCE height; every uiFont() size is scaled by this.
    constexpr float fontScale        = 1.25f;
    constexpr float fontButtonText   = 9.5f;

    // Layout
    constexpr int   knobRowH          = 48;
    constexpr int   knobLabelH        = 13;
    constexpr int   knobCellH         = knobRowH + knobLabelH;  // knob + name + value
    constexpr int   buttonH           = 22;
    constexpr int   knobGap           = 3;

    // Display heights
    constexpr int   displayH_small    = 44;
    constexpr int   displayH_filter   = 70;
    constexpr int   displayH_waveform = 59;

    // Display style
    constexpr float displayCornerRadius = 4.0f;
    constexpr float displayStrokeWidth  = 1.5f;
    constexpr float displayFillAlpha    = 0.12f;
}
