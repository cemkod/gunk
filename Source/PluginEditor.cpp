#include "PluginEditor.h"
#include "UIConstants.h"
#include "DebugLog.h"

//==============================================================================
JQGunkAudioProcessorEditor::JQGunkAudioProcessorEditor (JQGunkAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      topBar (p),
      mainView (p),
      modView (p),
      knobAnnotator (p.apvts)
{
    topBar.onViewChanged = [this] (bool showMod)
    {
        modView .setVisible ( showMod);
        mainView.setVisible (! showMod);
    };

    canvas.addAndMakeVisible (topBar);
    canvas.addAndMakeVisible (mainView);
    canvas.addChildComponent (modView);
    canvas.onResized = [this] { layoutCanvas(); };
    addAndMakeVisible (canvas);

    // Set LookAndFeel after the sections are children so propagation covers them
    setLookAndFeel (&lookAndFeel);

    // Read the saved size first: setResizeLimits() resizes us, and resized() records the width.
    const int savedWidth = processor.getEditorWidth();

    // Resizable with a fixed aspect ratio, 75 %..150 % of the design canvas.
    // setResizeLimits creates the default constrainer, so set it before the aspect ratio.
    setResizable (true, true);
    setResizeLimits (UIConst::editorMinWidth, UIConst::editorMinWidth * UIConst::editorHeight / UIConst::editorWidth,
                     UIConst::editorMaxWidth, UIConst::editorMaxWidth * UIConst::editorHeight / UIConst::editorWidth);
    getConstrainer()->setFixedAspectRatio (UIConst::editorAspect);

    canvas.setBounds (0, 0, UIConst::editorWidth, UIConst::editorHeight);
    const int w = savedWidth > 0 ? juce::jlimit (UIConst::editorMinWidth, UIConst::editorMaxWidth, savedWidth)
                                 : UIConst::editorWidth;
    setSize (w, juce::roundToInt (w / UIConst::editorAspect));

    knobAnnotator.build (canvas);
    startTimerHz (UIConst::uiTimerHz);
}

JQGunkAudioProcessorEditor::~JQGunkAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

//==============================================================================
void JQGunkAudioProcessorEditor::timerCallback()
{
#ifdef ENABLE_DEBUG_LOG
    const double t0 = juce::Time::getMillisecondCounterHiRes();
#endif

    const bool gateOpen       = processor.isGateOpen();
    const bool transientFired = processor.consumeTransient();

    topBar.update (gateOpen, transientFired);
    knobAnnotator.refresh();
    // Only the visible view pulls live data, so idle cost stays flat
    if (mainView.isVisible())
        mainView.tick (gateOpen, transientFired);
    else
        modView.tick();

#ifdef ENABLE_DEBUG_LOG
    const double now = juce::Time::getMillisecondCounterHiRes();
    const double ms = now - t0;
    timerTotalMs += ms;
    timerMaxMs = juce::jmax (timerMaxMs, ms);
    ++timerCount;
    if (timerLastLogMs < 0.0)
        timerLastLogMs = now;
    if (now - timerLastLogMs >= kTimerLogIntervalMs)
    {
        dbgLog ("UI timer (last 30 s): avg=" + juce::String (timerTotalMs / juce::jmax (1, timerCount), 3)
                + "ms max=" + juce::String (timerMaxMs, 3) + "ms");
        timerTotalMs = timerMaxMs = 0.0;
        timerCount = 0;
        timerLastLogMs = now;
    }
#endif
}

//==============================================================================
void JQGunkAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (BassLookAndFeel::bg);
}

void JQGunkAudioProcessorEditor::resized()
{
    canvas.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) UIConst::editorWidth));
    processor.setEditorWidth (getWidth());
}

void JQGunkAudioProcessorEditor::layoutCanvas()
{
    auto area = canvas.getLocalBounds();
    topBar.setBounds (area.removeFromTop (UIConst::topBarH));
    area.reduce (UIConst::editorPad, UIConst::editorPad);
    mainView.setBounds (area);
    modView .setBounds (area);
}
