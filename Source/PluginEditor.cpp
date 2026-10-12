#include "PluginEditor.h"

using namespace bs;
using namespace bs::ui;

namespace { constexpr int W = 1360, H = 900; }

//==============================================================================
// Lane editor
float LaneEditor::colX (int c) const { return 8.0f + ((float) getWidth() - 16.0f) * ((float) c + 0.5f) / (float) maxSteps; }
int LaneEditor::colAt (float x) const { return juce::jlimit (0, maxSteps - 1, (int) ((x - 8.0f) / (((float) getWidth() - 16.0f) / (float) maxSteps))); }
float LaneEditor::semiY (int semi) const { return (float) getHeight() - 12.0f - ((float) getHeight() - 24.0f) * (float) semi / 36.0f; }
int LaneEditor::ySemi (float y) const { return juce::jlimit (0, 36, juce::roundToInt (((float) getHeight() - 12.0f - y) / (((float) getHeight() - 24.0f) / 36.0f))); }

void LaneEditor::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (col::deep);
    g.fillRoundedRectangle (r, 5.0f);

    const int len = proc.laneLength (layer);
    const int sc = (int) proc.state.getRawParameterValue (scale)->load();
    const int rootNote = (int) proc.state.getRawParameterValue (root)->load();
    const auto& steps = scaleSteps (sc);

    // pitch grid: scale rows faint, octave rows brighter with note names
    for (int oct = 0; oct <= 3; ++oct)
        for (int s : steps)
        {
            const int semi = oct * 12 + s;
            if (semi > 36) continue;
            const float y = semiY (semi);
            g.setColour (col::cream.withAlpha (s == 0 ? 0.22f : 0.06f));
            g.drawHorizontalLine ((int) y, 8.0f, r.getRight() - 8.0f);
            if (s == 0)
            {
                g.setColour (col::creamDim.withAlpha (0.7f));
                g.setFont (font (12.5f));
                g.drawText (bs::noteName (rootNote + semi), getWidth() - 44, juce::jlimit (0, getHeight() - 14, (int) y - 14), 38, 14, juce::Justification::right);
            }
        }

    const float colW = (r.getWidth() - 16.0f) / (float) maxSteps;
    const int pageStart = proc.getPage() * 8;
    const int playing = proc.getPlayingStep (layer);
    for (int c = 0; c < maxSteps; ++c)
    {
        const float x = colX (c);
        const bool inLen = c < len;
        if (c == playing && inLen)
        {
            g.setColour (col::amber.withAlpha (0.16f));
            g.fillRect (x - colW * 0.5f, 2.0f, colW, r.getHeight() - 4.0f);
        }
        g.setColour (col::cream.withAlpha (inLen ? ((c % 4) == 0 ? 0.35f : 0.16f) : 0.05f));
        g.drawVerticalLine ((int) x, 6.0f, r.getBottom() - 6.0f);
    }
    // page bracket: the 8 steps the Launch Control knobs are editing
    g.setColour (col::amber.withAlpha (0.8f));
    g.fillRect (colX (pageStart) - colW * 0.45f, r.getBottom() - 4.0f, colW * 7.9f, 3.0f);
    // steps beyond the lane length are greyed out
    if (len < maxSteps)
    {
        g.setColour (col::deep.withAlpha (0.65f));
        g.fillRect (colX (len) - colW * 0.5f, 0.0f, r.getRight() - (colX (len) - colW * 0.5f), r.getHeight());
    }

    const auto& ln = proc.pattern.lane[(size_t) layer];
    const juce::Colour noteCol = layer == 0 ? col::amber : col::steelHi;
    for (int c = 0; c < maxSteps; ++c)
    {
        const float v = ln.vel[(size_t) c].load();
        if (v <= 0.0f) continue;
        const int q = quantise (ln.semi[(size_t) c].load(), sc);
        const float rad = 4.0f + v * 11.0f, x = colX (c), y = semiY (q);
        const bool live = c == playing;
        g.setColour (noteCol.withAlpha (c < len ? (live ? 1.0f : 0.85f) : 0.25f));
        g.fillEllipse (x - rad, y - rad, rad * 2.0f, rad * 2.0f);
        g.setColour (col::deep);
        g.drawEllipse (x - rad, y - rad, rad * 2.0f, rad * 2.0f, 1.5f);
    }
}

void LaneEditor::mouseDown (const juce::MouseEvent& e)
{
    dragCol = colAt (e.position.x);
    auto& ln = proc.pattern.lane[(size_t) layer];
    const int sc = (int) proc.state.getRawParameterValue (scale)->load();
    moved = false; startX = e.position.x;
    if (e.mods.isPopupMenu()) { proc.setStep (layer, dragCol, ln.semi[(size_t) dragCol].load(), 0.0f); dragCol = -1; repaint(); return; }

    const float v = ln.vel[(size_t) dragCol].load();
    const float y = semiY (quantise (ln.semi[(size_t) dragCol].load(), sc));
    editingExisting = v > 0.0f && std::abs (e.position.y - y) <= 6.0f + v * 11.0f;
    if (editingExisting) { startVel = v; lastSemi = ln.semi[(size_t) dragCol].load(); }
    else
    {
        startVel = v > 0.0f ? v : 0.8f;
        lastSemi = quantise (ySemi (e.position.y), sc);
        proc.setStep (layer, dragCol, lastSemi, startVel);
        proc.audition (layer, dragCol);
        moved = true;
    }
    repaint();
}

void LaneEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (dragCol < 0) return;
    if (e.getDistanceFromDragStart() < 3 && ! moved) return;
    moved = true;
    const int sc = (int) proc.state.getRawParameterValue (scale)->load();
    const int semi = quantise (ySemi (e.position.y), sc);
    const float vel = juce::jlimit (0.05f, 1.0f, startVel + (e.position.x - startX) / 90.0f);
    proc.setStep (layer, dragCol, semi, vel);
    if (semi != lastSemi) { lastSemi = semi; proc.audition (layer, dragCol); }
    repaint();
}

void LaneEditor::mouseUp (const juce::MouseEvent&)
{
    if (dragCol >= 0 && editingExisting && ! moved)
        proc.setStep (layer, dragCol, proc.pattern.lane[(size_t) layer].semi[(size_t) dragCol].load(), 0.0f); // tap removes
    dragCol = -1;
    repaint();
}

//==============================================================================
BeatstationEditor::BeatstationEditor (BeatstationProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&look);
    auto& s = proc.state;

    for (int l = 0; l < 2; ++l)
    {
        lanes[(size_t) l] = std::make_unique<LaneEditor> (proc, l);
        addAndMakeVisible (*lanes[(size_t) l]);
        auto& sd = side[(size_t) l];
        sd.steps = std::make_unique<Knob> (s, lid (l, L::len), "STEPS");
        addAndMakeVisible (*sd.steps);
        sd.rate.addItemList (rateNames(), 1);
        addAndMakeVisible (sd.rate);
        sd.rateAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, lid (l, L::rate), sd.rate);
        sd.random.onClick = [this, l] { proc.randomise (l); };
        sd.clear.onClick = [this, l] { proc.clearLane (l); };
        sd.mute.setClickingTogglesState (true);
        sd.mute.getProperties().set ("env", true);
        sd.muteAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, lid (l, L::mute), sd.mute);
        for (auto* b : { &sd.random, &sd.clear, &sd.mute }) addAndMakeVisible (b);
    }

    auto mk = [&s, this] (std::vector<std::unique_ptr<Knob>>& v, const juce::String& id, const juce::String& label)
    { v.push_back (std::make_unique<Knob> (s, id, label)); addAndMakeVisible (*v.back()); };

    mk (seqKnobs, shuffle, "SHUFFLE"); mk (seqKnobs, root, "ROOT");
    mk (seqKnobs, mutOrder, "M.ORDER"); mk (seqKnobs, mutPitch, "M.PITCH"); mk (seqKnobs, mutVel, "M.VEL");
    scaleBox.addItemList (scaleNames(), 1);
    addAndMakeVisible (scaleBox);
    scaleAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, scale, scaleBox);
    for (int i = 0; i < 4; ++i)
    {
        auto& b = pageBtn[(size_t) i];
        b.setButtonText (juce::String (i * 8 + 1) + "-" + juce::String (i * 8 + 8));
        b.getProperties().set ("env", true);
        b.onClick = [this, i] { proc.setPage (i); };
        addAndMakeVisible (b);
    }

    static const std::pair<const char*, const char*> voiceSpec[] = {
        { L::osc1, "OSC 1" }, { L::tune1, "TUNE 1" }, { L::drop1, "DROP 1" }, { L::slope1, "SLOPE 1" }, { L::lvl1, "LEVEL 1" },
        { L::osc2, "OSC 2" }, { L::tune2, "TUNE 2" }, { L::drop2, "DROP 2" }, { L::slope2, "SLOPE 2" }, { L::lvl2, "LEVEL 2" },
        { L::fm, "FM" }, { L::ring, "RING" },
        { L::noise, "NOISE" }, { L::nslope, "N.DECAY" },
        { L::fmode, "MODE" }, { L::cutoff, "CUTOFF" }, { L::res, "RES" }, { L::grit, "GRIT" }, { L::fdecay, "F.DECAY" }, { L::fenv, "F.ENV" },
        { L::attack, "ATTACK" }, { L::decay, "DECAY" }, { L::envMode, "ENV" }, { L::level, "LEVEL" } };
    for (int l = 0; l < 2; ++l)
        for (auto& [id, label] : voiceSpec) mk (voiceKnobs[(size_t) l], lid (l, id), label);

    static const char* dn[] = { "DROP", "FM", "NOISE", "BAL", "CUTOFF", "LEVELS" };
    for (int d = 0; d < 6; ++d) mk (modKnobs, pmID (d), dn[d]);
    for (int d = 0; d < 6; ++d) mk (modKnobs, vmID (d), dn[d]);

    mk (outKnobs, bit, "BIT DRIVE"); mk (outKnobs, boost, "BOOST"); mk (outKnobs, boostFreq, "B.FREQ");
    mk (outKnobs, width, "WIDTH"); mk (outKnobs, gain, "GAIN");
    temperKnob = std::make_unique<Knob> (s, temper, "TEMPER", true);
    addAndMakeVisible (*temperKnob);

    for (auto* t : { &tabP, &tabW }) { t->getProperties().set ("env", true); t->setClickingTogglesState (false); addAndMakeVisible (t); }
    tabP.onClick = [this] { showLayer (0); };
    tabW.onClick = [this] { showLayer (1); };

    presetBtn.onClick = [this] { showPresetMenu(); };
    prevBtn.onClick = [this] { proc.presets.step (-1); repaint(); };
    nextBtn.onClick = [this] { proc.presets.step (1); repaint(); };
    saveBtn.onClick = [this] { savePreset(); };
    for (auto* b : { &prevBtn, &nextBtn, &presetBtn, &saveBtn }) addAndMakeVisible (b);

    setSize (W, H);

    // hammertone blue
    texture = juce::Image (juce::Image::RGB, W, H, true);
    {
        juce::Graphics g (texture);
        g.fillAll (col::paint);
        juce::Random r (1980);
        for (int i = 0; i < 2600; ++i)
        {
            const float x = r.nextFloat() * W, y = r.nextFloat() * H, d = 6.0f + r.nextFloat() * 22.0f;
            g.setColour ((r.nextBool() ? col::paintHi : col::recess).withAlpha (0.05f + 0.07f * r.nextFloat()));
            g.fillEllipse (x - d * 0.5f, y - d * 0.5f, d, d);
        }
        for (int i = 0; i < 26000; ++i)
        {
            g.setColour ((r.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.05f));
            g.fillRect (r.nextInt (W), r.nextInt (H), 1, 1);
        }
    }
    showLayer (0);
    startTimerHz (30);
}

BeatstationEditor::~BeatstationEditor() { stopTimer(); setLookAndFeel (nullptr); }

void BeatstationEditor::showLayer (int l)
{
    shownLayer = l;
    for (int k = 0; k < 2; ++k)
        for (auto& kn : voiceKnobs[(size_t) k]) kn->setVisible (k == l);
    tabP.setToggleState (l == 0, juce::dontSendNotification);
    tabW.setToggleState (l == 1, juce::dontSendNotification);
    repaint (voiceArea);
}

void BeatstationEditor::section (juce::Graphics& g, juce::Rectangle<int> a, const juce::String& title)
{
    auto f = a.toFloat();
    g.setColour (col::recess.withAlpha (0.85f));
    g.fillRoundedRectangle (f, 6.0f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawLine (f.getX() + 6, f.getY() + 1, f.getRight() - 6, f.getY() + 1, 2.0f);
    g.setColour (col::paintHi.withAlpha (0.5f));
    g.drawLine (f.getX() + 6, f.getBottom() - 1, f.getRight() - 6, f.getBottom() - 1, 1.0f);
    if (title.isEmpty()) return;
    g.setFont (font (15.0f, true));
    const int tw = juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), title) + 22;
    auto plate = juce::Rectangle<int> (a.getX() + 12, a.getY() + 6, tw, 22);
    g.setColour (col::deep);
    g.fillRoundedRectangle (plate.toFloat(), 3.0f);
    g.setColour (col::cream);
    g.drawText (title, plate, juce::Justification::centred);
}

void BeatstationEditor::paint (juce::Graphics& g)
{
    g.drawImageAt (texture, 0, 0);
    g.setColour (col::deep.withAlpha (0.9f));
    g.fillRect (topBar);
    g.setColour (col::hazard);
    g.fillRect (topBar.getX(), topBar.getBottom() - 3, topBar.getWidth(), 3);
    g.setColour (col::cream);
    g.setFont (font (32.0f, true).withExtraKerningFactor (0.18f));
    g.drawText ("BEATSTATION", 22, 6, 360, 40, juce::Justification::centredLeft);
    g.setFont (font (14.0f));
    g.setColour (col::creamDim);
    g.drawText ("manners! DSP", 24, 42, 300, 20, juce::Justification::centredLeft);
    g.setColour (col::amber);
    g.drawText (midiText, 1084, 8, W - 1104, 22, juce::Justification::centredRight);
    g.setColour (proc.isTransportRunning() ? col::amber : col::creamDim);
    g.setFont (font (14.0f, true));
    g.drawText (proc.isTransportRunning() ? "RUNNING WITH LOGIC" : "PRESS PLAY IN LOGIC", 840, 20, 220, 26, juce::Justification::centredLeft);
    g.setColour (col::creamDim);
    g.setFont (font (13.5f));
    g.drawFittedText ("U1 PATTERN / U2 SOUND", 1084, 36, W - 1104, 22, juce::Justification::centredRight, 1, 0.8f);

    // lanes
    section (g, laneArea, {});
    for (int l = 0; l < 2; ++l)
    {
        auto lr = lanes[(size_t) l]->getBounds();
        g.setFont (font (17.0f, true));
        g.setColour (l == 0 ? col::amber : col::steelHi);
        g.drawText (l == 0 ? "PUNCH" : "WASH", laneArea.getX() + 12, lr.getY(), 170, 22, juce::Justification::centredLeft);
    }
    section (g, seqArea, "SEQUENCER");
    g.setFont (font (13.5f, true));
    g.setColour (col::cream);
    g.drawText ("SCALE", scaleBox.getX(), scaleBox.getY() - 22, scaleBox.getWidth(), 18, juce::Justification::centred);
    g.drawText ("LAUNCH CONTROL PAGE", pageBtn[0].getX(), pageBtn[0].getY() - 22, pageBtn[3].getRight() - pageBtn[0].getX(), 18, juce::Justification::centred);

    section (g, voiceArea, "VOICE");
    g.setFont (font (13.0f, true));
    for (size_t i = 0; i < voiceGroups.size(); ++i)
    {
        static const char* names[] = { "VCO 1", "VCO 2", "MOD", "NOISE", "FILTER", "AMP" };
        auto gr = voiceGroups[i];
        g.setColour (col::cream.withAlpha (0.25f));
        g.drawHorizontalLine (gr.getY() + 7, (float) gr.getX() + 4, (float) gr.getRight() - 4);
        g.setColour (col::recess);
        const int tw = juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), names[i]) + 12;
        g.fillRect (gr.getCentreX() - tw / 2, gr.getY(), tw, 14);
        g.setColour (col::creamDim);
        g.drawText (names[i], gr.getCentreX() - tw / 2, gr.getY(), tw, 14, juce::Justification::centred);
    }

    section (g, modArea, "MOD MATRIX");
    g.setFont (font (13.5f, true));
    g.setColour (col::cream);
    g.drawText ("PITCH >", modArea.getX() + 8, modKnobs[0]->getY() + 38, 62, 20, juce::Justification::centredLeft);
    g.drawText ("VEL >", modArea.getX() + 8, modKnobs[6]->getY() + 38, 62, 20, juce::Justification::centredLeft);

    section (g, outArea, "OUTPUT");

    // TEMPER with hazard stripe
    section (g, temperArea, "TEMPER");
    auto stripe = temperArea.reduced (12).removeFromBottom (14).toFloat();
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (stripe.toNearestInt());
        g.setColour (col::bakelite); g.fillRect (stripe);
        g.setColour (col::hazard);
        for (float x = stripe.getX() - 40; x < stripe.getRight() + 40; x += 24)
        {
            juce::Path p;
            p.addQuadrilateral (x, stripe.getBottom(), x + 12, stripe.getBottom(), x + 12 + stripe.getHeight(), stripe.getY(), x + stripe.getHeight(), stripe.getY());
            g.fillPath (p);
        }
    }
    auto kb = temperKnob->getBounds();
    g.setFont (font (15.0f, true));
    g.setColour (col::cream);
    g.drawText ("CALM", kb.getX() - 70, kb.getCentreY() + 14, 64, 20, juce::Justification::centredRight);
    g.drawText ("CHAOS", kb.getRight() + 6, kb.getCentreY() + 14, 70, 20, juce::Justification::centredLeft);
    g.setFont (font (22.0f, true));
    g.setColour (col::amber);
    g.drawText (temperKnob->text(), kb.getRight() + 84, kb.getY() + 20, temperArea.getRight() - kb.getRight() - 94, 30, juce::Justification::centredLeft);
    g.setFont (font (14.0f));
    g.setColour (col::creamDim);
    const float tv = proc.state.getRawParameterValue (temper)->load();
    const juce::String mood = tv < 0.3f ? "rolling waves" : tv < 0.45f ? "settling down" : tv <= 0.55f ? "as programmed" : tv < 0.8f ? "getting nasty" : "riot on the shop floor";
    g.drawText (mood, kb.getRight() + 84, kb.getY() + 52, temperArea.getRight() - kb.getRight() - 94, 20, juce::Justification::centredLeft);
}

void BeatstationEditor::resized()
{
    topBar = { 0, 0, W, 66 };
    prevBtn.setBounds (400, 18, 32, 30);
    presetBtn.setBounds (436, 18, 250, 30);
    nextBtn.setBounds (690, 18, 32, 30);
    saveBtn.setBounds (730, 18, 70, 30);

    laneArea = { 20, 74, W - 40, 300 };
    const int canvasX = 200;
    lanes[0]->setBounds (canvasX, laneArea.getY() + 8, laneArea.getRight() - canvasX - 8, 160);
    lanes[1]->setBounds (canvasX, laneArea.getY() + 8 + 160 + 8, laneArea.getRight() - canvasX - 8, 116);
    for (int l = 0; l < 2; ++l)
    {
        auto lr = lanes[(size_t) l]->getBounds();
        auto& sd = side[(size_t) l];
        auto colm = juce::Rectangle<int> (laneArea.getX() + 8, lr.getY() + 24, canvasX - laneArea.getX() - 16, lr.getHeight() - 24);
        sd.steps->setBounds (colm.removeFromLeft (62).withHeight (92));
        colm.removeFromLeft (6);
        const int bh = l == 0 ? 30 : 22;
        sd.rate.setBounds (colm.removeFromTop (bh).reduced (0, 1));
        sd.random.setBounds (colm.removeFromTop (bh).reduced (0, 1));
        sd.clear.setBounds (colm.removeFromTop (bh).reduced (0, 1));
        sd.mute.setBounds (colm.removeFromTop (bh).reduced (0, 1));
    }

    seqArea = { 20, 382, W - 40, 104 };
    {
        int x = seqArea.getX() + 140;
        for (auto& k : seqKnobs) { k->setBounds (x, seqArea.getY() + 4, 76, 96); x += 80; }
        scaleBox.setBounds (x + 20, seqArea.getY() + 44, 150, 30);
        int px = seqArea.getRight() - 4 * 78 - 20;
        for (auto& b : pageBtn) { b.setBounds (px, seqArea.getY() + 44, 72, 30); px += 78; }
    }

    voiceArea = { 20, 494, 880, 254 };
    tabP.setBounds (voiceArea.getX() + 100, voiceArea.getY() + 5, 90, 24);
    tabW.setBounds (voiceArea.getX() + 194, voiceArea.getY() + 5, 90, 24);
    {
        const int cell = 70, x0 = voiceArea.getX() + 10;
        const int y1 = voiceArea.getY() + 50, y2 = voiceArea.getY() + 156;
        for (int l = 0; l < 2; ++l)
            for (int i = 0; i < 24; ++i)
                voiceKnobs[(size_t) l][(size_t) i]->setBounds (x0 + (i % 12) * cell, i < 12 ? y1 : y2, cell, 92);
        auto grp = [&] (int from, int count, int y) { return juce::Rectangle<int> (x0 + from * cell, y - 16, count * cell, 14); };
        voiceGroups = { grp (0, 5, y1), grp (5, 5, y1), grp (10, 2, y1), grp (0, 2, y2), grp (2, 6, y2), grp (8, 4, y2) };
    }

    modArea = { 912, 494, W - 20 - 912, 254 };
    for (int i = 0; i < 12; ++i)
        modKnobs[(size_t) i]->setBounds (modArea.getX() + 66 + (i % 6) * 58, modArea.getY() + (i < 6 ? 34 : 140), 58, 96);

    outArea = { 20, 758, 520, 132 };
    for (int i = 0; i < 5; ++i) outKnobs[(size_t) i]->setBounds (outArea.getX() + 14 + i * 98, outArea.getY() + 32, 92, 96);

    temperArea = { 552, 758, W - 20 - 552, 132 };
    temperKnob->setBounds (temperArea.getX() + 220, temperArea.getY() + 4, 118, 104);
}

void BeatstationEditor::timerCallback()
{
    for (auto& l : lanes) l->repaint();
    const auto name = proc.presets.currentName();
    if (presetBtn.getButtonText() != name) presetBtn.setButtonText (name);
    for (int i = 0; i < 4; ++i) pageBtn[(size_t) i].setToggleState (proc.getPage() == i, juce::dontSendNotification);
    const auto m = proc.getLastMidiText();
    if (m != midiText) midiText = m;
    repaint (topBar);
    repaint (temperArea);
}

void BeatstationEditor::showPresetMenu()
{
    proc.presets.refresh();
    juce::PopupMenu menu;
    const auto& names = proc.presets.getNames();
    menu.addSectionHeader ("FACTORY");
    for (int i = 0; i < names.size(); ++i)
    {
        if (i == proc.presets.numFactory()) menu.addSectionHeader ("YOURS");
        menu.addItem (i + 1, names[i], true, names[i] == proc.presets.currentName());
    }
    menu.addSeparator();
    menu.addItem (10000, "Reveal presets folder");
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (presetBtn), [this] (int r)
    {
        if (r == 10000) { auto f = PresetManager::folder(); f.createDirectory(); f.revealToUser(); return; }
        if (r > 0) { proc.presets.load (r - 1); repaint(); }
    });
}

void BeatstationEditor::savePreset()
{
    saveWindow = std::make_unique<juce::AlertWindow> ("Save preset", "Name this beat:", juce::MessageBoxIconType::NoIcon, this);
    saveWindow->addTextEditor ("name", proc.presets.currentName());
    saveWindow->addButton ("SAVE", 1, juce::KeyPress (juce::KeyPress::returnKey));
    saveWindow->addButton ("CANCEL", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    saveWindow->enterModalState (true, juce::ModalCallbackFunction::create ([this] (int result)
    {
        if (result == 1 && saveWindow != nullptr && ! proc.presets.save (saveWindow->getTextEditorContents ("name")))
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "BEATSTATION", "Couldn't save that preset.");
        saveWindow.reset();
        repaint();
    }), false);
}
