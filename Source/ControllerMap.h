#pragma once
#include "Params.h"

// Novation Launch Control XL (original, absolute pots), factory User templates:
//   Knob rows CC 13-20 / 29-36 / 49-56, faders CC 77-84,
//   Track Focus buttons notes 41-44, 57-60, Track Control buttons notes 73-76, 89-92.
//
// BEATSTATION
//   User 1 (ch 1), the pattern, one column per step of the current page (8 steps):
//     Knob row 1  PUNCH step pitch      Knob row 2  PUNCH step velocity (fully down = no note)
//     Knob row 3  WASH step pitch       Faders      WASH step velocity  (fully down = no note)
//   User 2 (ch 2), the sound:
//     Knob row 1  PUNCH: DROP 1, SLOPE 1, FM, NOISE, CUTOFF, RES, GRIT, DECAY
//     Knob row 2  WASH:  DROP 1, SLOPE 1, FM, NOISE, CUTOFF, RES, GRIT, DECAY
//     Knob row 3  TEMPER, BIT DRIVER, BOOST, MUTATE ORDER, MUTATE PITCH, MUTATE VEL, SHUFFLE, GAIN
//     Faders      PUNCH LEVEL, WASH LEVEL, PUNCH F.ENV, WASH F.ENV, PUNCH ATTACK, WASH ATTACK, WIDTH, BOOST FREQ
//   Buttons (any channel):
//     Track Focus 1-4    page: steps 1-8, 9-16, 17-24, 25-32
//     Track Focus 5-8    RANDOM PUNCH, RANDOM WASH, CLEAR PUNCH, CLEAR WASH
//     Track Control 1-2  mute PUNCH, mute WASH
namespace bs::lcxl
{
    enum class Kind { none, stepPitch, stepVel, param, page, random, clear, mute };

    struct Target { Kind kind = Kind::none; int layer = 0; int col = -1; juce::String paramID; };

    inline int column (int n, int start) { return (n >= start && n < start + 8) ? n - start : -1; }
    inline int focusButton (int n)   { if (n >= 41 && n <= 44) return n - 41; if (n >= 57 && n <= 60) return n - 53; return -1; }
    inline int controlButton (int n) { if (n >= 73 && n <= 76) return n - 73; if (n >= 89 && n <= 92) return n - 85; return -1; }

    inline juce::String soundKnob (int layer, int col)
    {
        static const char* ids[] = { L::drop1, L::slope1, L::fm, L::noise, L::cutoff, L::res, L::grit, L::decay };
        return lid (layer, ids[col]);
    }

    inline Target resolve (int channel, bool isNote, int number)
    {
        Target t;
        if (! isNote)
        {
            int c;
            if (channel == 2)
            {
                if ((c = column (number, 13)) >= 0) { t.kind = Kind::param; t.paramID = soundKnob (0, c); return t; }
                if ((c = column (number, 29)) >= 0) { t.kind = Kind::param; t.paramID = soundKnob (1, c); return t; }
                if ((c = column (number, 49)) >= 0)
                {
                    static const juce::String g[] = { temper, bit, boost, mutOrder, mutPitch, mutVel, shuffle, gain };
                    t.kind = Kind::param; t.paramID = g[c]; return t;
                }
                if ((c = column (number, 77)) >= 0)
                {
                    static const juce::String f[] = { lid (0, L::level), lid (1, L::level), lid (0, L::fenv), lid (1, L::fenv),
                                                      lid (0, L::attack), lid (1, L::attack), width, boostFreq };
                    t.kind = Kind::param; t.paramID = f[c]; return t;
                }
            }
            else
            {
                if ((c = column (number, 13)) >= 0) { t.kind = Kind::stepPitch; t.layer = 0; t.col = c; return t; }
                if ((c = column (number, 29)) >= 0) { t.kind = Kind::stepVel;   t.layer = 0; t.col = c; return t; }
                if ((c = column (number, 49)) >= 0) { t.kind = Kind::stepPitch; t.layer = 1; t.col = c; return t; }
                if ((c = column (number, 77)) >= 0) { t.kind = Kind::stepVel;   t.layer = 1; t.col = c; return t; }
            }
        }
        if (const int b = focusButton (number); b >= 0)
        {
            if (b < 4) { t.kind = Kind::page; t.col = b; }
            else { t.kind = b < 6 ? Kind::random : Kind::clear; t.layer = (b - 4) % 2; }
            return t;
        }
        if (const int b = controlButton (number); b >= 0 && b < 2) { t.kind = Kind::mute; t.layer = b; t.paramID = lid (b, L::mute); return t; }
        return t;
    }
}
