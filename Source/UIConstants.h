#pragma once

namespace UIConst {
    // Barlow's line height is ~1.2 em, so it renders smaller than the old system font
    // at the same JUCE height; every uiFont() size is scaled by this.
    constexpr float fontScale = 1.25f;
    // Converts a rendered text height (px on the design canvas) to a uiFont() size.
    constexpr float px (float rendered) { return rendered / fontScale; }

    // Design canvas. Everything is laid out at this size and scaled to the window
    // with a transform (see PluginEditor::resized), so the aspect ratio is fixed.
    constexpr int   editorWidth     = 1200;
    constexpr int   editorHeight    = 800;
    constexpr int   editorMinWidth  = 900;    // 75 %
    constexpr int   editorMaxWidth  = 1800;   // 150 %
    constexpr double editorAspect   = (double) editorWidth / editorHeight;
    constexpr int   editorPad       = 10;
    constexpr int   topBarH         = 56;
    constexpr int   sectionGap      = 10;

    // Timers
    constexpr int uiTimerHz = 30;

    // LookAndFeel
    constexpr float sectionCornerRadius  = 6.0f;
    // Knob proportions, as fractions of the knob's outer radius / body radius
    constexpr float knobArcStrokeRatio    = 0.14f;  // arc thickness / outer radius
    constexpr float knobBodyGapRatio      = 0.08f;  // gap between arc and body / outer radius
    constexpr float knobPointerInnerRatio = 0.30f;  // pointer start / body radius
    constexpr float knobPointerOuterRatio = 0.92f;  // pointer end / body radius
    constexpr float knobPointerWidthRatio = 0.08f;  // pointer thickness / outer radius

    // Knob tiers: dial diameter in px
    constexpr int   knobHeroD       = 76;
    constexpr int   knobStandardD   = 52;
    constexpr int   knobSmallD      = 40;
    constexpr int   knobNameH       = 16;     // name label row inside a knob cell
    constexpr int   knobValueH      = 20;     // value text row inside a knob cell
    constexpr int   knobNameGap     = 3;      // space between dial and name

    constexpr float linearTrackH    = 6.0f;
    constexpr float linearThumbW    = 9.0f;
    constexpr int   powerToggleW    = 36;
    constexpr int   powerToggleH    = 18;
    constexpr float buttonCornerRadius   = 5.0f;
    constexpr float comboBoxCornerRadius = 5.0f;

    // Type scale (rendered px)
    constexpr float fontSectionTitle = px (16.0f);
    constexpr float fontKnobLabel    = px (12.0f);
    constexpr float fontKnobValue    = px (15.0f);
    constexpr float fontKnobValueHero  = px (18.0f);
    constexpr float fontKnobValueSmall = px (13.0f);
    constexpr float fontButtonText   = px (14.0f);
    constexpr float uiFontSize       = px (14.0f);   // combo text
    constexpr float fontCaption      = px (12.0f);   // small dim captions (WINDOW, SHAPE, ...)

    // Section header bar
    constexpr int   sectionHeaderH  = 30;
    constexpr int   sectionInnerPad = 12;

    // Layout
    constexpr int   buttonH         = 28;
    constexpr int   knobGap         = 6;
    constexpr int   captionH        = 16;

    // Display style
    constexpr float displayCornerRadius = 5.0f;
    constexpr float displayStrokeWidth  = 1.8f;
    constexpr float displayFillAlpha    = 0.12f;
}
