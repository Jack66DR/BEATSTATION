#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <vector>
#include <chrono>

// BEATSTATION sound engine. No JUCE dependency.
//   Layer: one mono percussive/bass voice. VCO1 + VCO2 + noise, each with a pitch
//          drop envelope, FM 1->2, ring / cross-ring, an 808-ish metal circuit on VCO1,
//          a multimode filter with resonance and GRIT, and a HIT or WAVE amp envelope.
//   Engine: PUNCH and WASH layers, boost EQ, bit driver, TEMPER calm filter, width.
namespace bs
{
constexpr double kTwoPi = 6.283185307179586;

struct Rng
{
    uint32_t s = 0x1234567u ^ (uint32_t) (reinterpret_cast<uintptr_t> (this) >> 4) ^ (uint32_t) std::chrono::steady_clock::now().time_since_epoch().count() | 1u;
    float u() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (s & 0xFFFFFF) / 16777216.0f; }
    float bi() { return u() * 2.0f - 1.0f; }
};

struct LayerParams
{
    int osc1 = 0, osc2 = 2, ring = 0, fmode = 0, envMode = 0;
    float tune1 = 0, tune2 = 12, drop1 = 24, drop2 = 12, slope1 = 0.06f, slope2 = 0.04f;
    float lvl1 = 0.85f, lvl2 = 0.3f, fm = 0.15f, noise = 0.2f, nslope = 0.05f;
    float cutoff = 4000, res = 0.2f, grit = 0.1f, fdecay = 0.15f, fenv = 0.4f;
    float attack = 0.001f, decay = 0.35f, level = 0.85f;
    bool mute = false;
};

struct ModMatrix { std::array<float, 6> pm {}, vm { { 0, 0, 0, 0, 0, 1 } }; };

struct Temper   // derived from the TEMPER knob: 0 calm .. 0.5 neutral .. 1 chaos
{
    float calm = 0, chaos = 0;
    static Temper from (float t) { Temper x; const float c = (t - 0.5f) * 2.0f; x.calm = std::max (0.0f, -c); x.chaos = std::max (0.0f, c); return x; }
};

class Layer
{
public:
    void prepare (double rate) { sr = rate; reset(); }
    void reset()
    {
        ph1 = ph2 = 0; prev1 = prev2 = 0; for (auto& m : mph) m = 0;
        peg1 = peg2 = nenv = fenvV = 0; amp = 0; stage = 0; gainSm = gainT = 0;
        ic1 = ic2 = 0; hpMetal = 0; gritLp = 0;
    }

    // midi note, velocity 0..1, pitch position -1..1 (low..high in the lane), mod matrix, temper
    void trigger (float midi, float vel, float pitchPos, const ModMatrix& mm, const Temper& t, Rng& rng)
    {
        note = midi;
        const float vb = vel * 2.0f - 1.0f;
        auto m = [&] (int d) { return mm.pm[(size_t) d] * pitchPos + mm.vm[(size_t) d] * vb; };
        dropOff = m (0) * 24.0f;   // no random pitch from TEMPER: notes stay as written
        (void) t; (void) rng;
        fmOff = m (1) * 0.6f;
        noiseOff = m (2);
        balOff = m (3);
        cutOct = m (4) * 4.0f;
        // Levels: velocity side defaults to "louder when harder"; pitch side tilts gain by register.
        const float vmL = mm.vm[5], pmL = mm.pm[5];
        float g = vmL >= 0 ? (1.0f - vmL) + vmL * vel : (1.0f + vmL) + (-vmL) * (1.0f - vel);
        g *= std::max (0.0f, 1.0f + 0.5f * pmL * pitchPos);
        gainT = g;
        peg1 = peg2 = 1.0f; nenv = 1.0f; fenvV = 1.0f;
        stage = 1;
        if (! started) { gainSm = gainT; started = true; }
        if (envModeLatched == 1) // WAVE: each hit pushes the swell higher instead of restarting it
            waveTarget = std::min (1.0f, amp + (1.0f - amp) * 0.85f);
        else
            waveTarget = 1.0f;
    }

    // Renders one sample (mono). p must already include TEMPER adjustments.
    float process (const LayerParams& p, const Temper& t, Rng& rng)
    {
        envModeLatched = p.envMode;
        if (stage == 0 && amp < 1.0e-6f) { amp = 0; return 0.0f; }

        // envelopes
        peg1 *= coef (p.slope1); peg2 *= coef (p.slope2);
        nenv *= coef (p.nslope); fenvV *= coef (p.fdecay);
        const float attackT = p.attack + t.calm * 0.05f;
        if (p.envMode == 0)
        {
            if (stage == 1) { amp += (1.15f - amp) * one (attackT * 0.6f); if (amp >= 1.0f) { amp = 1.0f; stage = 2; } }
            else if (stage == 2) { amp *= coef (p.decay); if (amp < 1.0e-5f) { amp = 0; stage = 0; } }
        }
        else
        {
            if (stage == 1) { amp += (waveTarget - amp) * one (std::max (0.01f, attackT)); if (amp >= waveTarget * 0.98f) stage = 2; }
            else if (stage == 2) { amp *= coef (p.decay); if (amp < 1.0e-5f) { amp = 0; stage = 0; } }
        }
        gainSm += (gainT - gainSm) * one (p.envMode == 1 ? 0.06f : 0.002f);

        // pitches
        const float drop1 = (p.drop1 + dropOff) * peg1, drop2 = (p.drop2 + dropOff) * peg2;
        const float f1 = clampHz (mtof (note + p.tune1 + drop1));
        const float f2 = clampHz (mtof (note + p.tune2 + drop2));
        const double dt1 = f1 / sr, dt2 = f2 / sr;

        // VCO1
        ph1 += dt1; if (ph1 >= 1.0) ph1 -= 1.0;
        float o1;
        if (p.osc1 == 3) o1 = metal (f1, p, t);
        else o1 = wave (p.osc1, ph1, dt1);

        // VCO2 with FM from VCO1 (phase modulation)
        const float fmIdx = std::clamp (p.fm + fmOff + t.chaos * 0.3f, 0.0f, 1.6f) * 1.3f;
        ph2 += dt2; if (ph2 >= 1.0) ph2 -= 1.0;
        double th2 = ph2 + fmIdx * o1; th2 -= std::floor (th2);
        double d2 = th2 - prev2; d2 -= std::floor (d2); if (d2 > 0.5) d2 = 1.0 - d2; d2 = std::clamp (d2, 1.0e-5, 0.25);
        prev2 = th2;
        float o2 = wave (p.osc2, th2, d2);
        if (p.ring == 1) o2 = o1 * o2;
        else if (p.ring == 2) o2 = 0.5f * (o1 * sgnSoft (o2) + o2 * sgnSoft (o1));

        // mix
        const float b = std::clamp (balOff, -1.0f, 1.0f);
        const float g1 = std::clamp (1.0f + b, 0.0f, 1.0f), g2 = std::clamp (1.0f - b, 0.0f, 1.0f);
        const float nz = rng.bi();
        const float noiseLvl = std::clamp (p.noise + noiseOff, 0.0f, 1.0f);
        float x = o1 * p.lvl1 * g1 + o2 * p.lvl2 * g2 + nz * noiseLvl * nenv;

        // filter: TPT state variable, with GRIT = resonant noise in the machine
        const float grit = std::clamp (p.grit * (1.0f - t.calm) + t.chaos * 0.35f, 0.0f, 1.0f);
        gritLp += (nz - gritLp) * 0.02f;
        float fc = p.cutoff * std::exp2 (p.fenv * 7.0f * fenvV + cutOct + grit * 1.8f * gritLp);
        fc = std::clamp (fc, 20.0f, (float) (sr * 0.45));
        x += grit * nz * 0.35f * (0.3f + amp);
        const float res = std::clamp (p.res + grit * 0.25f, 0.0f, 0.97f);
        const float g = (float) std::tan (3.14159265 * fc / sr), k = 2.0f - 2.0f * res;
        const float a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, a3 = g * a2;
        const float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1; ic2 = 2.0f * v2 - ic2;
        float y = p.fmode == 0 ? v2 : x - k * v1 - v2;
        y = std::tanh (y * 1.2f);

        lastAmp = amp * gainSm;
        return p.mute ? 0.0f : y * amp * gainSm * p.level;
    }

    float activity() const { return lastAmp; }

private:
    static float mtof (float m) { return 440.0f * std::exp2 ((m - 69.0f) / 12.0f); }
    float clampHz (float f) const { return std::clamp (f, 5.0f, (float) (sr * 0.45)); }
    float coef (float seconds) const { return std::exp (-6.9f / (std::max (0.0005f, seconds) * (float) sr)); }
    float one (float seconds) const { return 1.0f - std::exp (-1.0f / (std::max (0.0002f, seconds) * (float) sr)); }
    static float sgnSoft (float v) { return std::tanh (v * 8.0f); }

    static float blep (double t, double dt)
    {
        if (t < dt)       { t /= dt; return (float) (t + t - t * t - 1.0); }
        if (t > 1.0 - dt) { t = (t - 1.0) / dt; return (float) (t * t + t + t + 1.0); }
        return 0.0f;
    }

    static float wave (int w, double th, double dt)
    {
        switch (w)
        {
            case 0:  return (float) std::sin (kTwoPi * th);
            case 1:  { double u = th + 0.25; u -= std::floor (u); return (float) (1.0 - 4.0 * std::fabs (u - 0.5)); }
            default: { double t2 = th + 0.5; t2 -= std::floor (t2);
                       return 0.8f * ((th < 0.5 ? 1.0f : -1.0f) + blep (th, dt) - blep (t2, dt)); }
        }
    }

    // Loosely 808: six detuned pulse oscillators at inharmonic ratios, then a highpass.
    float metal (float f1, const LayerParams&, const Temper&)
    {
        static const float ratios[6] = { 1.0f, 1.4826f, 1.8003f, 2.5460f, 2.6303f, 3.8966f };
        float s = 0.0f;
        for (int i = 0; i < 6; ++i)
        {
            const double dt = std::min (0.45, (double) f1 * 2.4 * ratios[i] / sr);
            mph[(size_t) i] += dt; if (mph[(size_t) i] >= 1.0) mph[(size_t) i] -= 1.0;
            s += wave (2, mph[(size_t) i], dt);
        }
        s *= 0.25f;
        hpMetal += (s - hpMetal) * 0.25f;
        return (s - hpMetal) * 1.6f;
    }

    double sr = 48000.0;
    double ph1 = 0, ph2 = 0, prev1 = 0, prev2 = 0;
    std::array<double, 6> mph {};
    float note = 48, peg1 = 0, peg2 = 0, nenv = 0, fenvV = 0, amp = 0, waveTarget = 1, gainSm = 0, gainT = 0, lastAmp = 0;
    float dropOff = 0, fmOff = 0, noiseOff = 0, balOff = 0, cutOct = 0;
    float ic1 = 0, ic2 = 0, hpMetal = 0, gritLp = 0;
    int stage = 0, envModeLatched = 0;
    bool started = false;
};

struct GlobalParams
{
    float bit = 0.1f, boost = 0.0f, boostFreq = 120.0f, width = 0.5f, gainDb = -6.0f;
    Temper t;
};

class Engine
{
public:
    void prepare (double rate)
    {
        sr = rate;
        for (auto& l : layers) l.prepare (rate);
        haas.assign ((size_t) (rate * 0.03) + 4, 0.0f); hw = 0;
        reset();
    }
    void reset()
    {
        for (auto& l : layers) l.reset();
        std::fill (haas.begin(), haas.end(), 0.0f);
        for (auto& c : ch) c = {};
        holdCount = 0;
    }

    void trigger (int layer, float midi, float vel, float pitchPos, const ModMatrix& mm, const Temper& t)
    {
        layers[(size_t) layer].trigger (midi, vel, pitchPos, mm, t, rng);
    }

    float activity (int layer) const { return layers[(size_t) layer].activity(); }

    // Renders [start, end) into L/R.
    void render (const std::array<LayerParams, 2>& lp, const GlobalParams& gp, float* L, float* R, int start, int end)
    {
        // per-block constants
        const float bit = std::clamp (gp.bit * (1.0f - gp.t.calm) + gp.t.chaos * 0.3f, 0.0f, 1.0f);
        const float drive = 1.0f + bit * bit * 24.0f;
        const float driveNorm = 1.0f / (0.6f + 0.4f * std::sqrt (drive));
        const float levels = std::exp2 (16.0f - bit * 12.0f);     // 16 bits down to 4
        const int holdN = 1 + (int) (std::max (0.0f, bit - 0.5f) * 2.0f * 7.0f);
        const float calmHz = 20000.0f * std::exp2 (-gp.t.calm * 4.6f);
        const float gain = std::pow (10.0f, gp.gainDb / 20.0f);
        const int haasD = 1 + (int) (gp.width * 0.012 * sr);
        for (auto& c : ch)
        {
            c.peak.setPeak (gp.boostFreq, 1.2, gp.boost * 18.0f, sr);
            c.calm.setLP (calmHz, sr);
        }

        for (int n = start; n < end; ++n)
        {
            const float p = layers[0].process (lp[0], gp.t, rng);
            const float w = layers[1].process (lp[1], gp.t, rng);
            haas[(size_t) hw] = w;
            int rd = hw - haasD; if (rd < 0) rd += (int) haas.size();
            const float wR = haas[(size_t) rd];
            if (++hw >= (int) haas.size()) hw = 0;
            const float wL = w;
            float in[2] = { p + wL * (1.0f - 0.3f * gp.width), p + (gp.width > 0 ? wR : w) };

            for (int c = 0; c < 2; ++c)
            {
                auto& s = ch[(size_t) c];
                float x = s.peak.process (in[c]);
                // bit driver: overdrive, then bit reduction, then sample-hold at the top
                if (bit > 0.001f)
                {
                    x = std::tanh (x * drive) * driveNorm;
                    x = std::round (x * levels) / levels;
                    if (holdN > 1) { if (holdCount % holdN == 0) s.held = x; x = s.held; }
                }
                x = s.calm.process (x);
                const float y = x - s.dcIn + 0.9995f * s.dc; s.dcIn = x; s.dc = y;
                (c == 0 ? L : R)[n] = safety (y * gain);
            }
            ++holdCount;
        }
    }

private:
    struct Biquad
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        void setPeak (double f, double q, double db, double sr)
        {
            const double A = std::pow (10.0, db / 40.0), w = kTwoPi * std::min (f, sr * 0.45) / sr, al = std::sin (w) / (2.0 * q), c = std::cos (w);
            const double a0 = 1.0 + al / A;
            b0 = (float) ((1.0 + al * A) / a0); b1 = (float) (-2.0 * c / a0); b2 = (float) ((1.0 - al * A) / a0);
            a1 = (float) (-2.0 * c / a0); a2 = (float) ((1.0 - al / A) / a0);
        }
        void setLP (double f, double sr)
        {
            const double w = kTwoPi * std::min (f, sr * 0.45) / sr, al = std::sin (w) / (2.0 * 0.7071), c = std::cos (w), a0 = 1.0 + al;
            b0 = (float) ((1.0 - c) * 0.5 / a0); b1 = (float) ((1.0 - c) / a0); b2 = b0; a1 = (float) (-2.0 * c / a0); a2 = (float) ((1.0 - al) / a0);
        }
        float process (float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    };
    struct Chan { Biquad peak, calm; float held = 0, dc = 0, dcIn = 0; };

    static float safety (float x)
    {
        const float ax = std::fabs (x);
        if (! (ax < 1.0e6f)) return 0.0f;
        if (ax <= 0.8f) return x;
        const float y = 0.8f + 0.2f * std::tanh ((ax - 0.8f) / 0.2f);
        return x < 0 ? -y : y;
    }

    double sr = 48000.0;
    std::array<Layer, 2> layers;
    std::array<Chan, 2> ch;
    std::vector<float> haas; int hw = 0;
    long holdCount = 0;
    Rng rng;
};
}
