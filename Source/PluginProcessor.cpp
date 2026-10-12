#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace bs;

//==============================================================================
// Factory presets: parameter values plus a pattern for each lane ("semi:vel" per step).
namespace
{
    struct Factory
    {
        juce::String name;
        std::vector<std::pair<juce::String, float>> values;
        juce::String punch, wash;
    };

    juce::String pat (std::initializer_list<std::pair<int, float>> steps)
    {
        juce::StringArray a;
        for (auto& [s, v] : steps) a.add (juce::String (s) + ":" + juce::String (v, 2));
        return a.joinIntoString (",");
    }

    const std::vector<Factory>& factory()
    {
        static const std::vector<Factory> f = []
        {
            std::vector<Factory> v;
            const juce::String P = "p_", W = "w_";
            v.push_back ({ "Iron Pulse",
                { { P + "len", 16 }, { W + "len", 4 }, { W + "rate", 0 } },
                pat ({ {0,1}, {0,0}, {12,.35f}, {0,0}, {0,.9f}, {0,0}, {7,.45f}, {0,.3f}, {0,1}, {0,0}, {12,.4f}, {0,0}, {0,.85f}, {3,.5f}, {0,0}, {10,.4f} }),
                pat ({ {0,.8f}, {7,.6f}, {12,.7f}, {3,.6f} }) });
            v.push_back ({ "Rhythmic Waves",
                { { temper, 0.2f }, { P + "envmode", 1 }, { P + "attack", 0.08f }, { P + "decay", 1.4f }, { P + "cutoff", 650.0f },
                  { P + "res", 0.45f }, { P + "osc2", 0 }, { P + "drop1", 7.0f }, { P + "noise", 0.05f }, { P + "grit", 0.0f },
                  { P + "len", 7 }, { P + "rate", 1 }, { W + "len", 5 }, { W + "rate", 0 }, { W + "attack", 1.6f }, { W + "decay", 7.0f },
                  { W + "cutoff", 480.0f }, { bit, 0.0f }, { width, 0.8f } },
                pat ({ {0,.9f}, {0,0}, {7,.6f}, {12,.7f}, {0,0}, {5,.5f}, {3,.6f} }),
                pat ({ {0,.8f}, {0,0}, {7,.7f}, {0,0}, {10,.6f} }) });
            v.push_back ({ "Foundry",
                { { P + "osc1", 3 }, { P + "ring", 2 }, { P + "fmode", 1 }, { P + "cutoff", 2600.0f }, { P + "decay", 0.12f },
                  { P + "drop1", 0.0f }, { P + "drop2", 5.0f }, { P + "noise", 0.35f }, { P + "nslope", 0.04f }, { P + "res", 0.35f },
                  { W + "osc1", 0 }, { W + "cutoff", 300.0f }, { W + "len", 8 }, { W + "rate", 1 }, { boost, 0.35f }, { boostFreq, 2400.0f },
                  { vmID (4), 0.6f } },
                pat ({ {12,.7f}, {12,.35f}, {24,.6f}, {12,.3f}, {12,.8f}, {12,.3f}, {19,.6f}, {12,.4f},
                       {12,.7f}, {24,.3f}, {12,.6f}, {12,.3f}, {15,.9f}, {12,.3f}, {24,.5f}, {12,.45f} }),
                pat ({ {0,.7f}, {0,0}, {0,0}, {5,.5f}, {0,0}, {0,0}, {3,.6f}, {0,0} }) });
            v.push_back ({ "Static Tide",
                { { P + "noise", 0.8f }, { P + "nslope", 0.25f }, { P + "grit", 0.5f }, { P + "cutoff", 1800.0f }, { P + "lvl1", 0.4f },
                  { P + "lvl2", 0.0f }, { W + "noise", 0.55f }, { W + "nslope", 3.0f }, { W + "fenv", 0.6f }, { W + "fdecay", 3.0f },
                  { W + "len", 3 }, { W + "rate", 0 }, { vmID (2), 0.5f }, { pmID (4), 0.4f } },
                pat ({ {0,.9f}, {0,0}, {0,.4f}, {0,0}, {0,.7f}, {0,.3f}, {0,0}, {0,.5f}, {0,.9f}, {0,0}, {0,.4f}, {0,.6f}, {0,0}, {0,.3f}, {0,.8f}, {0,0} }),
                pat ({ {0,.7f}, {7,.5f}, {12,.6f} }) });
            v.push_back ({ "Shop Floor Riot",
                { { temper, 0.95f }, { mutOrder, 0.6f }, { mutPitch, 0.5f }, { mutVel, 0.5f }, { bit, 0.7f }, { P + "fm", 0.6f },
                  { P + "ring", 2 }, { P + "grit", 0.5f }, { shuffle, 0.35f }, { W + "fm", 0.5f }, { W + "len", 6 }, { W + "rate", 1 },
                  { pmID (1), 0.7f }, { vmID (0), 0.6f } },
                pat ({ {0,1}, {12,.5f}, {3,.7f}, {0,0}, {24,.6f}, {0,.9f}, {7,.4f}, {15,.6f}, {0,1}, {0,0}, {22,.5f}, {10,.7f}, {0,.8f}, {27,.4f}, {0,0}, {12,.9f} }),
                pat ({ {0,.8f}, {0,0}, {6,.7f}, {0,0}, {13,.6f}, {1,.5f} }) });
            return v;
        }();
        return f;
    }
}

void PresetManager::refresh()
{
    names.clear(); files.clear();
    for (auto& p : factory()) { names.add (p.name); files.add ({}); }
    if (auto dir = folder(); dir.isDirectory())
    {
        auto found = dir.findChildFiles (juce::File::findFiles, false, juce::String ("*") + extension);
        found.sort();
        for (auto& f : found) { names.add (f.getFileNameWithoutExtension()); files.add (f); }
    }
}

int PresetManager::numFactory() const { return (int) factory().size(); }
juce::String PresetManager::currentName() const { return proc.state.state.getProperty ("presetName", "INIT").toString(); }

bool PresetManager::load (int index)
{
    if (! juce::isPositiveAndBelow (index, names.size())) return false;
    auto setNorm = [] (juce::RangedAudioParameter* p, float n) { p->beginChangeGesture(); p->setValueNotifyingHost (n); p->endChangeGesture(); };
    if (index < numFactory())
    {
        for (auto* p : proc.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p)) setNorm (rp, rp->getDefaultValue());
        const auto& f = factory()[(size_t) index];
        for (auto& [id, v] : f.values)
            if (auto* p = proc.state.getParameter (id)) setNorm (p, p->convertTo0to1 (v));
        proc.pattern.lane[0].fromString (f.punch);
        proc.pattern.lane[1].fromString (f.wash);
    }
    else
    {
        auto xml = juce::XmlDocument::parse (files[index]);
        if (xml == nullptr) return false;
        proc.applyState (juce::ValueTree::fromXml (*xml));
    }
    proc.state.state.setProperty ("presetName", names[index], nullptr);
    return true;
}

void PresetManager::step (int delta)
{
    refresh();
    if (names.isEmpty()) return;
    const int i = names.indexOf (currentName());
    load (i < 0 ? 0 : (i + delta + names.size()) % names.size());
}

bool PresetManager::save (const juce::String& raw)
{
    const auto name = juce::File::createLegalFileName (raw.trim());
    if (name.isEmpty()) return false;
    auto dir = folder();
    if (! dir.createDirectory()) return false;
    proc.state.state.setProperty ("presetName", name, nullptr);
    auto xml = proc.makeState().createXml();
    const bool ok = xml != nullptr && xml->writeTo (dir.getChildFile (name + extension));
    refresh();
    return ok;
}

//==============================================================================
BeatstationProcessor::BeatstationProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "BEATSTATION", createLayout())
{
    state.state.setProperty ("presetName", "INIT", nullptr);
    presets.load (0);
    state.state.setProperty ("presetName", factory()[0].name, nullptr);
    startTimerHz (60);
}

BeatstationProcessor::~BeatstationProcessor() { stopTimer(); }

bool BeatstationProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void BeatstationProcessor::prepareToPlay (double sampleRate, int)
{
    if (sampleRate != preparedRate) { engine.prepare (sampleRate); preparedRate = sampleRate; }
}

void BeatstationProcessor::fillParams()
{
    for (int l = 0; l < 2; ++l)
    {
        auto& p = lp[(size_t) l];
        auto r = [&] (const char* id) { return raw (lid (l, id)); };
        p.osc1 = (int) r (L::osc1); p.osc2 = (int) r (L::osc2); p.ring = (int) r (L::ring);
        p.fmode = (int) r (L::fmode); p.envMode = (int) r (L::envMode);
        p.tune1 = r (L::tune1); p.tune2 = r (L::tune2); p.drop1 = r (L::drop1); p.drop2 = r (L::drop2);
        p.slope1 = r (L::slope1); p.slope2 = r (L::slope2); p.lvl1 = r (L::lvl1); p.lvl2 = r (L::lvl2);
        p.fm = r (L::fm); p.noise = r (L::noise); p.nslope = r (L::nslope);
        p.cutoff = r (L::cutoff); p.res = r (L::res); p.grit = r (L::grit); p.fdecay = r (L::fdecay); p.fenv = r (L::fenv);
        p.attack = r (L::attack); p.decay = r (L::decay); p.level = r (L::level); p.mute = r (L::mute) > 0.5f;
    }
    for (int d = 0; d < 6; ++d) { mm.pm[(size_t) d] = raw (pmID (d)); mm.vm[(size_t) d] = raw (vmID (d)); }
    gp.t = Temper::from (raw (temper));
    gp.bit = raw (bit); gp.boost = raw (boost); gp.boostFreq = raw (boostFreq); gp.width = raw (width); gp.gainDb = raw (gain);
}

void BeatstationProcessor::fireStep (int layer, int stepIdx)
{
    const auto& lane = pattern.lane[(size_t) layer];
    const int len = laneLength (layer);
    int semi = lane.semi[(size_t) stepIdx].load();
    float vel = lane.vel[(size_t) stepIdx].load();

    // MUTATE: real-time changes that never touch the stored pattern.
    // TEMPER deliberately has no say here: it shapes the sound, never the notes or events.
    const float chaosBoost = 0.0f;
    if (audioRng.nextFloat() < raw (mutOrder) + chaosBoost)
        for (int tries = 0; tries < 8; ++tries)
        {
            const int j = audioRng.nextInt (len);
            if (lane.vel[(size_t) j].load() > 0.0f) { semi = lane.semi[(size_t) j].load(); vel = lane.vel[(size_t) j].load(); break; }
        }
    const int sc = (int) raw (scale);
    if (vel > 0.0f && audioRng.nextFloat() < raw (mutPitch) + chaosBoost)
        semi = juce::jlimit (0, 36, semi + (audioRng.nextBool() ? 1 : -1) * (1 + audioRng.nextInt (4)));
    if (vel > 0.0f && audioRng.nextFloat() < raw (mutVel) + chaosBoost)
        vel = juce::jlimit (0.05f, 1.0f, vel * (0.3f + 1.0f * audioRng.nextFloat()));
    if (vel <= 0.001f) return;

    const int q = quantise (semi, sc);
    const float midi = (float) ((int) raw (root) + q);
    engine.trigger (layer, midi, vel, (float) q / 18.0f - 1.0f, mm, gp.t);
}

void BeatstationProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    if (preparedRate <= 0.0) prepareToPlay (getSampleRate() > 0 ? getSampleRate() : 48000.0, buffer.getNumSamples());

    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        const bool isCC = m.isController();
        if (! isCC && ! m.isNoteOn() && ! m.isNoteOff()) continue;
        const int ch = m.getChannel(), num = isCC ? m.getControllerNumber() : m.getNoteNumber();
        const int val = isCC ? m.getControllerValue() : (m.isNoteOn() ? m.getVelocity() : 0);
        lastMidiCh.store (ch); lastMidiNum.store (num); lastMidiVal.store (val); lastMidiIsNote.store (! isCC);
        if (lcxl::resolve (ch, ! isCC, num).kind == lcxl::Kind::none || fifo.getFreeSpace() < 1) continue;
        const auto sc = fifo.write (1);
        auto& slot = sc.blockSize1 > 0 ? fifoData[(size_t) sc.startIndex1] : fifoData[(size_t) sc.startIndex2];
        slot = { ch, ! isCC, num, val };
    }
    midi.clear();

    fillParams();
    const int n = buffer.getNumSamples();
    auto* Lp = buffer.getWritePointer (0);
    auto* Rp = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : Lp;

    // Auditions from the editor fire even when the transport is stopped
    for (int l = 0; l < 2; ++l)
        if (const int a = auditionReq[(size_t) l].exchange (-1); a >= 0) fireStep (l, a);

    // Host clock
    bool playing = false; double ppq0 = 0, bpm = 120;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            playing = pos->getIsPlaying();
            if (auto p = pos->getPpqPosition()) ppq0 = *p; else playing = false;
            if (auto b = pos->getBpm()) bpm = *b;
        }
    running.store (playing);
    if (playing)
    {
        clockPpq.store (ppq0); clockBpm.store (bpm);
        clockTimeMs.store (juce::Time::getMillisecondCounterHiRes());
    }

    int seg = 0;
    if (playing)
    {
        const double qps = bpm / 60.0 / preparedRate;
        const double swing = 0.5 * raw (shuffle);
        double stepQ[2]; int len[2];
        for (int l = 0; l < 2; ++l) { stepQ[l] = rateQuarters ((int) raw (lid (l, L::rate))); len[l] = laneLength (l); }
        for (int i = 0; i < n; ++i)
        {
            const double ppq = ppq0 + i * qps;
            for (int l = 0; l < 2; ++l)
            {
                const long long s = stepAt (ppq, stepQ[l], swing);
                if (s != lastStep[(size_t) l])
                {
                    if (i > seg) { engine.render (lp, gp, Lp, Rp == Lp ? Lp : Rp, seg, i); seg = i; }
                    lastStep[(size_t) l] = s;
                    fireStep (l, laneStep (s, len[l]));
                }
            }
        }
    }
    else
    {
        lastStep = { LLONG_MIN, LLONG_MIN };
    }

    if (Rp == Lp)
    {
        // mono: render to a scratch right channel and fold down
        juce::HeapBlock<float> tmp ((size_t) n);
        engine.render (lp, gp, Lp, tmp.get(), seg, n);
        for (int i = seg; i < n; ++i) Lp[i] = 0.5f * (Lp[i] + tmp[i]);
    }
    else
        engine.render (lp, gp, Lp, Rp, seg, n);
    for (int c = 2; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, n);
    for (int l = 0; l < 2; ++l) activity[(size_t) l].store (engine.activity (l));
}

//==============================================================================
void BeatstationProcessor::setStep (int layer, int step, int semi, float vel)
{
    pattern.lane[(size_t) layer].set (step, semi, vel);
}

void BeatstationProcessor::randomise (int layer)
{
    pattern.randomise (layer, laneLength (layer), (int) raw (scale), uiRng);
}

void BeatstationProcessor::clearLane (int layer) { pattern.lane[(size_t) layer].clear(); }

void BeatstationProcessor::timerCallback()
{
    const auto sc = fifo.read (fifo.getNumReady());
    for (int i = 0; i < sc.blockSize1; ++i) { auto& e = fifoData[(size_t) (sc.startIndex1 + i)]; applyController (e.channel, e.isNote, e.number, e.value); }
    for (int i = 0; i < sc.blockSize2; ++i) { auto& e = fifoData[(size_t) (sc.startIndex2 + i)]; applyController (e.channel, e.isNote, e.number, e.value); }
}

void BeatstationProcessor::applyController (int channel, bool isNote, int number, int value)
{
    using K = lcxl::Kind;
    const auto t = lcxl::resolve (channel, isNote, number);
    auto setNorm = [this] (const juce::String& id, float n)
    {
        if (auto* p = state.getParameter (id)) { p->beginChangeGesture(); p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, n)); p->endChangeGesture(); }
    };
    const int stepIdx = page.load() * 8 + t.col;
    switch (t.kind)
    {
        case K::stepPitch:
        {
            auto& ln = pattern.lane[(size_t) t.layer];
            const int semi = quantise (juce::roundToInt ((float) value / 127.0f * 36.0f), (int) raw (scale));
            ln.set (stepIdx, semi, ln.vel[(size_t) stepIdx].load());
            break;
        }
        case K::stepVel:
        {
            auto& ln = pattern.lane[(size_t) t.layer];
            ln.set (stepIdx, ln.semi[(size_t) stepIdx].load(), value <= 1 ? 0.0f : (float) value / 127.0f);
            break;
        }
        case K::param:  setNorm (t.paramID, (float) value / 127.0f); break;
        case K::page:   if (value > 0) setPage (t.col); break;
        case K::random: if (value > 0) randomise (t.layer); break;
        case K::clear:  if (value > 0) clearLane (t.layer); break;
        case K::mute:   if (value > 0) setNorm (t.paramID, raw (t.paramID) > 0.5f ? 0.0f : 1.0f); break;
        default: break;
    }
}

juce::String BeatstationProcessor::getLastMidiText() const
{
    const int num = lastMidiNum.load();
    if (num < 0) return "MIDI: waiting";
    return "MIDI CH" + juce::String (lastMidiCh.load()) + (lastMidiIsNote.load() ? " NOTE " : " CC ") + juce::String (num) + " = " + juce::String (lastMidiVal.load());
}

juce::ValueTree BeatstationProcessor::makeState()
{
    auto copy = state.copyState();
    pattern.writeTo (copy);
    return copy;
}

void BeatstationProcessor::applyState (const juce::ValueTree& tree)
{
    if (! tree.hasType (state.state.getType())) return;
    pattern.readFrom (tree);
    auto clean = tree.createCopy();
    if (auto p = clean.getChildWithName ("PATTERN"); p.isValid()) clean.removeChild (p, nullptr);
    state.replaceState (clean);
}

void BeatstationProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = makeState().createXml()) copyXmlToBinary (*xml, dest);
}

void BeatstationProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size)) applyState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* BeatstationProcessor::createEditor() { return new BeatstationEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BeatstationProcessor(); }
