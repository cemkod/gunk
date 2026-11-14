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
    constexpr float knobArcStrokeWidth   = 3.5f;
    constexpr float knobBodyRadiusRatio  = 0.78f;
    constexpr float knobPointerPosRatio  = 0.85f;   // pointer line end, fraction of body radius
    constexpr float knobPointerWidth     = 2.0f;
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
    constexpr float fontButtonText   = 9.5f;

    // Layout
    constexpr int   knobRowH          = 48;
    constexpr int   knobLabelH        = 13;
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
