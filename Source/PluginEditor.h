#pragma once
#include "PluginProcessor.h"
#include "LookAndFeel.h"

namespace bs::ui
{
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& label, bool big = false)
        : name (label), isBig (big), param (s.getParameter (id))
    {
        slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
        slider.setMouseDragSensitivity (big ? 400 : 220);
        addAndMakeVisible (slider);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
        slider.onValueChange = [this] { repaint(); };
        slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue())); // double-click resets
    }
    void resized() override
    {
        auto r = getLocalBounds();
        if (! isBig) { r.removeFromTop (19); r.removeFromBottom (19); }
        slider.setBounds (r);
    }
    void paint (juce::Graphics& g) override
    {
        if (isBig) return;
        auto r = getLocalBounds();
        g.setColour (col::cream);
        g.setFont (font (13.5f, true));
        g.drawFittedText (name, r.removeFromTop (18), juce::Justification::centred, 1, 0.8f);
        g.setFont (font (13.5f));
        g.setColour (col::creamDim);
        g.drawFittedText (text(), r.removeFromBottom (18), juce::Justification::centred, 1, 0.8f);
    }
    juce::String text() const { return param->getCurrentValueAsText(); }
    juce::Slider slider;
private:
    juce::String name;
    bool isBig;
    juce::RangedAudioParameter* param;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
};

// Step lane: drag up/down for pitch, left/right for velocity (circle size). Click a note to remove it.
class LaneEditor : public juce::Component
{
public:
    LaneEditor (BeatstationProcessor& p, int l) : proc (p), layer (l) {}
    void setPlayStep (int s) { if (s != playStep) { playStep = s; repaint(); } }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
private:
    float colX (int c) const;
    int colAt (float x) const;
    float semiY (int semi) const;
    int ySemi (float y) const;
    BeatstationProcessor& proc;
    int layer, dragCol = -1, lastSemi = -1, playStep = -1;
    bool editingExisting = false, moved = false;
    float startVel = 0.8f, startX = 0;
};
}

class BeatstationEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BeatstationEditor (BeatstationProcessor&);
    ~BeatstationEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void section (juce::Graphics&, juce::Rectangle<int>, const juce::String&);
    void showPresetMenu();
    void savePreset();
    void showLayer (int l);

    BeatstationProcessor& proc;
    bs::ui::DSLook look;
    juce::Image texture;

    std::array<std::unique_ptr<bs::ui::LaneEditor>, 2> lanes;
    struct LaneSide
    {
        std::unique_ptr<bs::ui::Knob> steps;
        juce::ComboBox rate;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rateAttach;
        juce::TextButton random { "RANDOM" }, clear { "CLEAR" }, mute { "MUTE" };
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> muteAttach;
    };
    std::array<LaneSide, 2> side;

    std::vector<std::unique_ptr<bs::ui::Knob>> seqKnobs, modKnobs, outKnobs;
    std::array<std::vector<std::unique_ptr<bs::ui::Knob>>, 2> voiceKnobs;
    juce::ComboBox scaleBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> scaleAttach;
    std::array<juce::TextButton, 4> pageBtn;
    juce::TextButton tabP { "PUNCH" }, tabW { "WASH" };
    int shownLayer = 0;
    double displayPpq = 0.0, lastFrameMs = 0.0;
    bool displayLocked = false;
    int laneFrame = 0;
    std::unique_ptr<bs::ui::Knob> temperKnob;

    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, presetBtn, saveBtn { "SAVE" };
    std::unique_ptr<juce::AlertWindow> saveWindow;
    juce::String midiText;

    juce::Rectangle<int> topBar, laneArea, seqArea, voiceArea, modArea, outArea, temperArea;
    std::vector<juce::Rectangle<int>> voiceGroups;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BeatstationEditor)
};
