#pragma once

namespace UIConst {
    // Editor dimensions
    constexpr int editorWidth    = 660;
    constexpr int editorHeight   = 660;
    constexpr int editorPad      = 10;
    constexpr int headerHeight   = 60;
    constexpr int labelRowH      = 16;
    constexpr int presetStripH   = 28;
    constexpr int sectionGap     = 8;
    constexpr int gateSectionH   = 149;
    constexpr int oscSectionH    = 272;
    constexpr int filterSectionH = 261;
    constexpr int topRowH        = 261;  // gate+filter row height
    constexpr int oscColW        = 165;  // editorWidth / 4

    // Gate LED
    constexpr int gateLedSize    = 8;
    constexpr int gateLedPad     = 6;

    // Timers
    constexpr int uiTimerHz      = 30;

    // LookAndFeel
    constexpr float sectionCornerRadius  = 4.0f;
    constexpr float knobArcStrokeWidth   = 3.0f;
    constexpr float knobBodyRadiusRatio  = 0.65f;
    constexpr float knobPointerPosRatio  = 0.6f;
    constexpr float knobPointerRadius    = 3.0f;
    constexpr int   sliderTextBoxW       = 60;
    constexpr int   sliderTextBoxH       = 20;
    constexpr float uiFontSize           = 11.0f;
    constexpr float buttonCornerRadius   = 4.0f;
    constexpr float comboBoxCornerRadius = 4.0f;
}
