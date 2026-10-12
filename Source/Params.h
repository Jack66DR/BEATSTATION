#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// BEATSTATION parameters. Two voices (PUNCH and WASH) share one layout, prefixed p_ / w_.
namespace bs
{
    constexpr int maxSteps = 32;

    inline juce::String lid (int layer, const char* name) { return juce::String (layer == 0 ? "p_" : "w_") + name; }

    // Per-layer parameter names
    namespace L
    {
        constexpr const char* osc1 = "osc1";  constexpr const char* osc2 = "osc2";
        constexpr const char* tune1 = "tune1"; constexpr const char* tune2 = "tune2";
        constexpr const char* drop1 = "drop1"; constexpr const char* drop2 = "drop2";
        constexpr const char* slope1 = "slope1"; constexpr const char* slope2 = "slope2";
        constexpr const char* lvl1 = "lvl1";  constexpr const char* lvl2 = "lvl2";
        constexpr const char* fm = "fm";      constexpr const char* ring = "ring";
        constexpr const char* noise = "noise"; constexpr const char* nslope = "nslope";
        constexpr const char* fmode = "fmode"; constexpr const char* cutoff = "cutoff";
        constexpr const char* res = "res";    constexpr const char* grit = "grit";
        constexpr const char* fdecay = "fdecay"; constexpr const char* fenv = "fenv";
        constexpr const char* attack = "attack"; constexpr const char* decay = "decay";
        constexpr const char* envMode = "envmode"; constexpr const char* level = "level";
        constexpr const char* mute = "mute";
        constexpr const char* len = "len";    constexpr const char* rate = "rate";
    }

    // Shared modulation matrix: sequencer pitch and velocity -> 6 destinations
    inline const juce::StringArray& modDests() { static juce::StringArray a { "drop", "fm", "noise", "bal", "cut", "lvl" }; return a; }
    inline juce::String pmID (int d) { return "pm_" + modDests()[d]; }
    inline juce::String vmID (int d) { return "vm_" + modDests()[d]; }

    static const juce::String shuffle = "shuffle", root = "root", scale = "scale",
        mutOrder = "mutOrder", mutPitch = "mutPitch", mutVel = "mutVel",
        temper = "temper", bit = "bit", boost = "boost", boostFreq = "boostFreq",
        width = "width", gain = "gain";

    inline const juce::StringArray& osc1Names()  { static juce::StringArray a { "SINE", "TRI", "SQR", "METAL" }; return a; }
    inline const juce::StringArray& osc2Names()  { static juce::StringArray a { "SINE", "TRI", "SQR" }; return a; }
    inline const juce::StringArray& ringNames()  { static juce::StringArray a { "OFF", "RING", "XRING" }; return a; }
    inline const juce::StringArray& fmodeNames() { static juce::StringArray a { "LP", "HP" }; return a; }
    inline const juce::StringArray& envNames()   { static juce::StringArray a { "HIT", "WAVE" }; return a; }
    inline const juce::StringArray& rateNames()  { static juce::StringArray a { "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" }; return a; }
    inline double rateQuarters (int r) { static const double q[] = { 1.0, 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125 }; return q[juce::jlimit (0, 5, r)]; }
    inline const juce::StringArray& scaleNames() { static juce::StringArray a { "CHROMATIC", "MINOR", "MAJOR", "PENT MINOR", "PHRYGIAN", "DORIAN" }; return a; }

    inline const std::vector<int>& scaleSteps (int s)
    {
        static const std::vector<int> sc[] = {
            { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 }, { 0, 2, 3, 5, 7, 8, 10 }, { 0, 2, 4, 5, 7, 9, 11 },
            { 0, 3, 5, 7, 10 }, { 0, 1, 3, 5, 7, 8, 10 }, { 0, 2, 3, 5, 7, 9, 10 } };
        return sc[juce::jlimit (0, 5, s)];
    }

    // Nearest in-scale semitone (0..36) to a raw offset
    inline int quantise (int semis, int s)
    {
        const auto& st = scaleSteps (s);
        int best = 0, bestD = 99;
        for (int oct = 0; oct <= 3; ++oct)
            for (int x : st)
            {
                const int v = oct * 12 + x;
                if (v > 36) continue;
                const int d = std::abs (v - semis);
                if (d < bestD) { bestD = d; best = v; }
            }
        return best;
    }

    inline juce::String noteName (int midi)
    {
        static const char* n[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        return juce::String (n[((midi % 12) + 12) % 12]) + juce::String (midi / 12 - 1);
    }

    inline juce::NormalisableRange<float> logRange (float lo, float hi)
    {
        return { lo, hi,
                 [] (float s, float e, float n) { return s * std::pow (e / s, n); },
                 [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); },
                 [] (float s, float e, float v) { return juce::jlimit (s, e, v); } };
    }

    inline juce::String pct (float v)  { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; }
    inline juce::String bip (float v)  { const int i = juce::roundToInt (v * 100.0f); return (i > 0 ? "+" : "") + juce::String (i); }
    inline juce::String secs (float s) { return s < 1.0f ? juce::String (juce::roundToInt (s * 1000.0f)) + "ms" : juce::String (s, 2) + "s"; }
    inline juce::String hz (float v)   { return v >= 1000.0f ? juce::String (v / 1000.0f, 1) + "k" : juce::String (juce::roundToInt (v)); }
    inline juce::String semis (float v) { const float r = std::round (v * 10.0f) / 10.0f; return (r > 0 ? "+" : "") + juce::String (r, 1) + "st"; }

    inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        using namespace juce;
        AudioProcessorValueTreeState::ParameterLayout layout;
        auto A = [] (std::function<String (float)> f)
        { return AudioParameterFloatAttributes().withStringFromValueFunction ([f] (float v, int) { return f (v); }); };

        for (int l = 0; l < 2; ++l)
        {
            const bool w = l == 1;
            const String nm = w ? "Wash " : "Punch ";
            auto F = [&] (const char* id, const String& name, NormalisableRange<float> r, float def, std::function<String (float)> f)
            { layout.add (std::make_unique<AudioParameterFloat> (ParameterID { lid (l, id), 1 }, nm + name, r, def, A (f))); };
            auto C = [&] (const char* id, const String& name, const StringArray& c, int def)
            { layout.add (std::make_unique<AudioParameterChoice> (ParameterID { lid (l, id), 1 }, nm + name, c, def)); };
            NormalisableRange<float> unit (0.0f, 1.0f), bipo (-1.0f, 1.0f);

            C (L::osc1, "Osc 1 Wave", osc1Names(), w ? 1 : 0);
            C (L::osc2, "Osc 2 Wave", osc2Names(), w ? 0 : 2);
            F (L::tune1, "Osc 1 Tune", { -24.0f, 24.0f }, 0.0f, semis);
            F (L::tune2, "Osc 2 Tune", { -24.0f, 24.0f }, w ? 7.0f : 12.0f, semis);
            F (L::drop1, "Osc 1 Drop", { -48.0f, 48.0f }, w ? 0.0f : 24.0f, semis);
            F (L::drop2, "Osc 2 Drop", { -48.0f, 48.0f }, w ? 0.0f : 12.0f, semis);
            F (L::slope1, "Osc 1 Slope", logRange (0.002f, 2.0f), 0.06f, secs);
            F (L::slope2, "Osc 2 Slope", logRange (0.002f, 2.0f), 0.04f, secs);
            F (L::lvl1, "Osc 1 Level", unit, w ? 0.6f : 0.85f, pct);
            F (L::lvl2, "Osc 2 Level", unit, w ? 0.5f : 0.3f, pct);
            F (L::fm, "FM", unit, w ? 0.1f : 0.15f, pct);
            C (L::ring, "Ring Mod", ringNames(), 0);
            F (L::noise, "Noise Level", unit, w ? 0.15f : 0.2f, pct);
            F (L::nslope, "Noise Decay", logRange (0.005f, 4.0f), w ? 1.2f : 0.05f, secs);
            C (L::fmode, "Filter Mode", fmodeNames(), 0);
            F (L::cutoff, "Cutoff", logRange (30.0f, 18000.0f), w ? 900.0f : 4000.0f, hz);
            F (L::res, "Resonance", unit, w ? 0.3f : 0.2f, pct);
            F (L::grit, "Grit", unit, w ? 0.0f : 0.1f, pct);
            F (L::fdecay, "Filter Decay", logRange (0.005f, 6.0f), w ? 1.5f : 0.15f, secs);
            F (L::fenv, "Filter Env", bipo, w ? 0.3f : 0.4f, bip);
            F (L::attack, "Attack", logRange (0.0005f, 6.0f), w ? 0.8f : 0.001f, secs);
            F (L::decay, "Decay", logRange (0.01f, 16.0f), w ? 4.0f : 0.35f, secs);
            C (L::envMode, "Envelope Mode", envNames(), w ? 1 : 0);
            F (L::level, "Level", unit, w ? 0.6f : 0.85f, pct);
            layout.add (std::make_unique<AudioParameterBool> (ParameterID { lid (l, L::mute), 1 }, nm + "Mute", false));
            layout.add (std::make_unique<AudioParameterInt> (ParameterID { lid (l, L::len), 1 }, nm + "Steps", 1, maxSteps, 16));
            C (L::rate, "Step Rate", rateNames(), w ? 0 : 3);
        }

        static const char* dn[] = { "Drop", "FM", "Noise", "Osc Balance", "Cutoff", "Levels" };
        for (int d = 0; d < 6; ++d)
        {
            layout.add (std::make_unique<AudioParameterFloat> (ParameterID { pmID (d), 1 }, String ("Pitch > ") + dn[d],
                NormalisableRange<float> (-1.0f, 1.0f), 0.0f, A (bip)));
            layout.add (std::make_unique<AudioParameterFloat> (ParameterID { vmID (d), 1 }, String ("Velocity > ") + dn[d],
                NormalisableRange<float> (-1.0f, 1.0f), d == 5 ? 1.0f : 0.0f, A (bip)));
        }

        NormalisableRange<float> unit (0.0f, 1.0f);
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { shuffle, 1 }, "Shuffle", unit, 0.0f, A (pct)));
        layout.add (std::make_unique<AudioParameterInt> (ParameterID { root, 1 }, "Root Note", 24, 60, 36, AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return noteName (v); })));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { scale, 1 }, "Scale", scaleNames(), 1));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { mutOrder, 1 }, "Mutate Order", unit, 0.0f, A (pct)));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { mutPitch, 1 }, "Mutate Pitch", unit, 0.0f, A (pct)));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { mutVel, 1 }, "Mutate Velocity", unit, 0.0f, A (pct)));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { temper, 1 }, "Temper", unit, 0.5f,
            A ([] (float v) { const int c = juce::roundToInt ((v - 0.5f) * 200.0f); return c == 0 ? String ("NEUTRAL") : c < 0 ? "CALM " + String (-c) : "CHAOS " + String (c); })));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { bit, 1 }, "Bit Driver", unit, 0.1f, A (pct)));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { boost, 1 }, "Boost", unit, 0.0f, A ([] (float v) { return "+" + String (v * 18.0f, 1) + "dB"; })));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { boostFreq, 1 }, "Boost Freq", logRange (40.0f, 8000.0f), 120.0f, A (hz)));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { width, 1 }, "Wash Width", unit, 0.5f, A (pct)));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { gain, 1 }, "Output Gain", NormalisableRange<float> (-36.0f, 12.0f), -6.0f,
            A ([] (float v) { return (v > 0 ? "+" : "") + String (v, 1) + "dB"; })));
        return layout;
    }
}
