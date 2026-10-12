// Offline test for BEATSTATION: drives the processor with a fake host transport.
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include <chrono>

static int failures = 0;
#define CHECK(cond, msg) do { if (! (cond)) { ++failures; std::printf ("FAIL: %s\n", msg); } else std::printf ("ok:   %s\n", msg); } while (0)

struct FakeHost : juce::AudioPlayHead
{
    double ppq = 0.0, bpm = 120.0; bool playing = true;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p; p.setIsPlaying (playing); p.setPpqPosition (ppq); p.setBpm (bpm); return p;
    }
};

static constexpr double SR = 48000.0;

static void setParam (BeatstationProcessor& p, const juce::String& id, float real)
{
    auto* prm = p.state.getParameter (id);
    prm->setValueNotifyingHost (prm->convertTo0to1 (real));
}
static float getReal (BeatstationProcessor& p, const juce::String& id) { return p.state.getRawParameterValue (id)->load(); }

// Render seconds of audio with the fake host advancing; returns mono (left) samples.
static std::vector<float> run (BeatstationProcessor& p, FakeHost& host, double seconds, int block = 256)
{
    std::vector<float> out;
    juce::AudioBuffer<float> buf (2, block);
    juce::MidiBuffer midi;
    const int total = (int) (seconds * SR);
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = std::min (block, total - pos);
        buf.setSize (2, n, false, false, true);
        buf.clear();
        p.processBlock (buf, midi);
        for (int i = 0; i < n; ++i) out.push_back (buf.getSample (0, i));
        if (host.playing) host.ppq += n * host.bpm / 60.0 / SR;
    }
    return out;
}

static std::vector<int> onsets (const std::vector<float>& x, float thresh = 0.02f, int gap = 2400)
{
    std::vector<int> o; int last = -gap;
    for (int i = 0; i < (int) x.size(); ++i)
        if (std::abs (x[(size_t) i]) > thresh && i - last >= gap) { o.push_back (i); last = i; }
        else if (std::abs (x[(size_t) i]) > thresh) last = i;
    return o;
}

static void blank (BeatstationProcessor& p)
{
    p.clearLane (0); p.clearLane (1);
    for (auto id : { bs::mutOrder, bs::mutPitch, bs::mutVel, bs::shuffle }) setParam (p, id, 0.0f);
    setParam (p, bs::temper, 0.5f);
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File outDir = argc > 1 ? juce::File (juce::String (argv[1])) : juce::File();

    // ---- timing locked to the host
    {
        BeatstationProcessor p; FakeHost host; p.setPlayHead (&host); p.prepareToPlay (SR, 256);
        blank (p);
        setParam (p, bs::lid (0, bs::L::decay), 0.05f);
        p.setStep (0, 0, 0, 1.0f);  // one hit per bar (16 x 1/16)
        auto x = run (p, host, 4.2);
        auto o = onsets (x);
        std::printf ("      onsets (samples): "); for (int v : o) std::printf ("%d ", v); std::printf ("\n");
        bool ok = o.size() == 3;
        for (size_t i = 0; ok && i < o.size(); ++i) ok = std::abs (o[i] - (int) (i * 96000)) < 300;
        CHECK (ok, "PUNCH hits land on the bar at 120 BPM (within 6 ms)");

        host.playing = false;
        auto s = run (p, host, 2.0);
        auto o2 = onsets (s);
        CHECK (o2.empty(), "nothing plays while Logic is stopped");
    }
    {
        BeatstationProcessor p; FakeHost host; p.setPlayHead (&host); p.prepareToPlay (SR, 256);
        blank (p);
        setParam (p, bs::lid (0, bs::L::decay), 0.05f);
        setParam (p, bs::shuffle, 1.0f);
        p.setStep (0, 1, 0, 1.0f);  // the off-beat 16th
        auto o = onsets (run (p, host, 1.9));
        const int expect = (int) (0.375 * 24000); // 1/16 = 0.25 quarter, swung to 0.375 quarter at full shuffle
        CHECK (! o.empty() && std::abs (o[0] - expect) < 300, "SHUFFLE pushes the off-beat 16th late");
    }
    {
        BeatstationProcessor p; FakeHost host; p.setPlayHead (&host); p.prepareToPlay (SR, 256);
        blank (p);
        setParam (p, bs::lid (1, bs::L::envMode), 0.0f);
        setParam (p, bs::lid (1, bs::L::attack), 0.001f);
        setParam (p, bs::lid (1, bs::L::decay), 0.08f);
        setParam (p, bs::lid (1, bs::L::rate), 0.0f); setParam (p, bs::lid (1, bs::L::len), 3.0f);
        setParam (p, bs::lid (1, bs::L::cutoff), 6000.0f);
        p.setStep (1, 0, 12, 1.0f);
        auto o = onsets (run (p, host, 4.0));
        CHECK (o.size() >= 3 && std::abs (o[1] - o[0] - 72000) < 300, "WASH has its own length and rate (3 x 1/4)");
    }

    // ---- WAVE envelope rolls instead of restarting
    {
        auto ripple = [] (int envMode)
        {
            BeatstationProcessor p; FakeHost host; p.setPlayHead (&host); p.prepareToPlay (SR, 256);
            blank (p);
            setParam (p, bs::lid (0, bs::L::level), 0.0f);
            setParam (p, bs::lid (1, bs::L::envMode), (float) envMode);
            setParam (p, bs::lid (1, bs::L::attack), 0.3f); setParam (p, bs::lid (1, bs::L::decay), 2.5f);
            setParam (p, bs::lid (1, bs::L::rate), 1.0f);
            for (int i = 0; i < 16; ++i) p.setStep (1, i, 0, 0.8f);
            auto x = run (p, host, 6.0);
            float mn = 1e9f, mx = 0;
            for (size_t i = (size_t) (2 * SR); i + 2400 < x.size(); i += 2400)
            {
                double s = 0; for (size_t j = 0; j < 2400; ++j) s += (double) x[i + j] * x[i + j];
                const float r = (float) std::sqrt (s / 2400); mn = std::min (mn, r); mx = std::max (mx, r);
            }
            return mn / std::max (1e-9f, mx);
        };
        const float wave = ripple (1), hit = ripple (0);
        std::printf ("      level ratio min/max over 50 ms: WAVE %.2f, HIT %.2f\n", wave, hit);
        CHECK (wave > 0.4f, "WAVE mode rolls smoothly instead of restarting each hit");
    }

    // ---- every factory preset, at calm, neutral and chaos
    {
        BeatstationProcessor p; FakeHost host; p.setPlayHead (&host); p.prepareToPlay (SR, 512);
        for (int i = 0; i < p.presets.numFactory(); ++i)
        {
            CHECK (p.presets.load (i), ("load " + p.presets.getNames()[i]).toRawUTF8());
            const float baseTemper = getReal (p, bs::temper);
            bool fine = true;
            for (float t : { baseTemper, 0.0f, 1.0f })
            {
                setParam (p, bs::temper, t);
                auto x = run (p, host, 5.0, 512);
                double s = 0, pk = 0; bool finite = true;
                for (float v : x) { if (! std::isfinite (v)) finite = false; s += (double) v * v; pk = std::max (pk, (double) std::abs (v)); }
                const double rms = std::sqrt (s / (double) x.size());
                if (t == baseTemper) std::printf ("      %-18s rms %.3f peak %.3f\n", p.presets.getNames()[i].toRawUTF8(), rms, pk);
                if (! finite || pk > 1.0 || rms < 0.003) { fine = false; std::printf ("      !! temper %.1f rms %.4f peak %.3f finite %d\n", t, rms, pk, (int) finite); }
                if (outDir != juce::File() && t == baseTemper)
                {
                    p.presets.load (i); host.ppq = 0;
                    juce::AudioBuffer<float> cap (2, (int) (SR * 12));
                    juce::AudioBuffer<float> buf (2, 512); juce::MidiBuffer m;
                    for (int pos = 0; pos < cap.getNumSamples(); pos += 512)
                    {
                        buf.clear(); p.processBlock (buf, m);
                        for (int c = 0; c < 2; ++c) cap.copyFrom (c, pos, buf, c, 0, std::min (512, cap.getNumSamples() - pos));
                        host.ppq += 512 * host.bpm / 60.0 / SR;
                    }
                    auto f = outDir.getChildFile (juce::File::createLegalFileName (p.presets.getNames()[i]).replace (" ", "_") + ".wav");
                    f.deleteFile();
                    juce::WavAudioFormat wav;
                    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), SR, 2, 24, {}, 0));
                    if (w) w->writeFromAudioSampleBuffer (cap, 0, cap.getNumSamples());
                }
            }
            CHECK (fine, "renders finite, audible and under 0 dBFS at calm, neutral and chaos");
        }
    }

    // ---- Launch Control map
    {
        BeatstationProcessor p; p.prepareToPlay (SR, 256); blank (p);
        p.applyController (1, false, 29, 127);
        CHECK (p.pattern.lane[0].vel[0].load() > 0.99f, "U1 row 2 knob 1 = PUNCH step 1 velocity");
        p.applyController (1, false, 13, 127);
        CHECK (p.pattern.lane[0].semi[0].load() == bs::quantise (36, (int) getReal (p, bs::scale)), "U1 row 1 knob 1 = PUNCH step 1 pitch");
        p.applyController (1, true, 42, 127);
        CHECK (p.getPage() == 1, "Track Focus 2 = page 9-16");
        p.applyController (1, false, 84, 100);
        CHECK (std::abs (p.pattern.lane[1].vel[15].load() - 100.0f / 127.0f) < 0.01f, "U1 fader 8 on page 2 = WASH step 16 velocity");
        p.applyController (1, false, 84, 0);
        CHECK (p.pattern.lane[1].vel[15].load() == 0.0f, "fader fully down removes the WASH note");
        p.applyController (2, false, 49, 0);
        CHECK (getReal (p, bs::temper) < 0.01f, "U2 row 3 knob 1 = TEMPER");
        p.applyController (2, false, 34, 127);
        CHECK (getReal (p, bs::lid (1, bs::L::res)) > 0.99f, "U2 row 2 knob 6 = WASH resonance");
        p.applyController (2, false, 78, 0);
        CHECK (getReal (p, bs::lid (1, bs::L::level)) < 0.01f, "U2 fader 2 = WASH level");
        p.applyController (1, true, 74, 127);
        CHECK (getReal (p, bs::lid (1, bs::L::mute)) > 0.5f, "Track Control 2 mutes WASH");
        p.applyController (1, true, 57, 127);
        bool any = false; for (int i = 0; i < 16; ++i) any |= p.pattern.lane[0].vel[(size_t) i].load() > 0;
        CHECK (any, "Track Focus 5 randomises PUNCH");
        p.applyController (1, true, 59, 127);
        any = false; for (int i = 0; i < 32; ++i) any |= p.pattern.lane[0].vel[(size_t) i].load() > 0;
        CHECK (! any, "Track Focus 7 clears PUNCH");
    }

    // ---- state and presets carry the pattern
    {
        BeatstationProcessor a; a.prepareToPlay (SR, 256); blank (a);
        a.setStep (0, 5, 7, 0.6f); a.setStep (1, 2, 12, 0.9f); setParam (a, bs::temper, 0.8f);
        juce::MemoryBlock mb; a.getStateInformation (mb);
        BeatstationProcessor b; b.setStateInformation (mb.getData(), (int) mb.getSize());
        CHECK (b.pattern.lane[0].semi[5].load() == 7 && std::abs (b.pattern.lane[1].vel[2].load() - 0.9f) < 0.01f
               && std::abs (getReal (b, bs::temper) - 0.8f) < 0.01f, "Logic project recall restores sound and pattern");
        CHECK (a.presets.save ("zz test beat"), "save user preset");
        a.clearLane (0);
        a.presets.refresh();
        const int idx = a.presets.getNames().indexOf ("zz test beat");
        CHECK (idx >= a.presets.numFactory() && a.presets.load (idx) && a.pattern.lane[0].semi[5].load() == 7, "user preset brings its pattern back");
        bs::PresetManager::folder().getChildFile (juce::String ("zz test beat") + bs::PresetManager::extension).deleteFile();
    }

    // ---- CPU, worst case
    {
        BeatstationProcessor p; FakeHost host; p.setPlayHead (&host); p.prepareToPlay (SR, 512);
        p.presets.load (p.presets.numFactory() - 1);
        const auto t0 = std::chrono::steady_clock::now();
        run (p, host, 20.0, 512);
        const double secs = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        std::printf ("      CPU: %.1f%% of one core\n", 100.0 * secs / 20.0);
    }

    std::printf ("\n%s (%d failures)\n", failures == 0 ? "ALL PASSED" : "FAILED", failures);
    return failures == 0 ? 0 : 1;
}
