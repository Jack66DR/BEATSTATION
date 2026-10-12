#pragma once
#include <array>
#include <atomic>
#include <juce_data_structures/juce_data_structures.h>
#include "Params.h"

// Two lanes of up to 32 steps. Each step: pitch (semitones above ROOT, 0..36) and
// velocity (0 = no note). Written from the message thread, read on the audio thread.
namespace bs
{
struct Lane
{
    std::array<std::atomic<int>, maxSteps> semi {};
    std::array<std::atomic<float>, maxSteps> vel {};

    void clear() { for (int i = 0; i < maxSteps; ++i) { semi[(size_t) i].store (0); vel[(size_t) i].store (0.0f); } }
    void set (int i, int s, float v) { semi[(size_t) i].store (juce::jlimit (0, 36, s)); vel[(size_t) i].store (juce::jlimit (0.0f, 1.0f, v)); }

    juce::String toString() const
    {
        juce::StringArray a;
        for (int i = 0; i < maxSteps; ++i)
            a.add (juce::String (semi[(size_t) i].load()) + ":" + juce::String (vel[(size_t) i].load(), 3));
        return a.joinIntoString (",");
    }
    void fromString (const juce::String& s)
    {
        clear();
        auto parts = juce::StringArray::fromTokens (s, ",", "");
        for (int i = 0; i < std::min (maxSteps, parts.size()); ++i)
        {
            auto kv = juce::StringArray::fromTokens (parts[i], ":", "");
            if (kv.size() == 2) set (i, kv[0].getIntValue(), kv[1].getFloatValue());
        }
    }
};

struct Pattern
{
    std::array<Lane, 2> lane;

    void writeTo (juce::ValueTree& state) const
    {
        auto t = state.getOrCreateChildWithName ("PATTERN", nullptr);
        t.setProperty ("punch", lane[0].toString(), nullptr);
        t.setProperty ("wash", lane[1].toString(), nullptr);
    }
    void readFrom (const juce::ValueTree& state)
    {
        auto t = state.getChildWithName ("PATTERN");
        if (! t.isValid()) return;
        lane[0].fromString (t.getProperty ("punch").toString());
        lane[1].fromString (t.getProperty ("wash").toString());
    }

    // Random pattern: PUNCH busy and percussive, WASH sparse and low.
    void randomise (int l, int len, int scaleIdx, juce::Random& r)
    {
        auto& ln = lane[(size_t) l];
        ln.clear();
        const auto& sc = scaleSteps (scaleIdx);
        const float density = l == 0 ? 0.55f : 0.28f;
        for (int i = 0; i < len; ++i)
        {
            const bool downbeat = (i % 4) == 0;
            if (r.nextFloat() < (downbeat ? density + 0.25f : density))
            {
                const int oct = l == 0 ? (r.nextFloat() < 0.6f ? 0 : r.nextInt (3)) : r.nextInt (2);
                const int semi = oct * 12 + sc[(size_t) r.nextInt ((int) sc.size())];
                const float v = l == 0 ? 0.35f + 0.65f * r.nextFloat() : 0.4f + 0.5f * r.nextFloat();
                ln.set (i, std::min (36, semi), downbeat ? std::max (v, 0.8f) : v);
            }
        }
    }
};
}
