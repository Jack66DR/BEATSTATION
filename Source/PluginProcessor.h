#pragma once
#include <climits>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Params.h"
#include "Engine.h"
#include "Pattern.h"
#include "ControllerMap.h"

class BeatstationProcessor;

namespace bs
{
class PresetManager
{
public:
    explicit PresetManager (BeatstationProcessor& p) : proc (p) { refresh(); }
    static constexpr const char* extension = ".beatstation";
    static juce::File folder()
    {
        return juce::File::getSpecialLocation (juce::File::userMusicDirectory)
                  .getChildFile ("manners! DSP").getChildFile ("BEATSTATION").getChildFile ("Presets");
    }
    void refresh();
    int numFactory() const;
    const juce::StringArray& getNames() const { return names; }
    juce::String currentName() const;
    bool load (int index);
    void step (int delta);
    bool save (const juce::String& name);

private:
    BeatstationProcessor& proc;
    juce::StringArray names;
    juce::Array<juce::File> files;
};
}

class BeatstationProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    BeatstationProcessor();
    ~BeatstationProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "BEATSTATION"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 20.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // Full state including the pattern (used by presets too)
    juce::ValueTree makeState();
    void applyState (const juce::ValueTree&);

    // Pattern editing (message thread)
    void setStep (int layer, int step, int semi, float vel);
    void randomise (int layer);
    void clearLane (int layer);
    void audition (int layer, int step) { auditionReq[(size_t) layer].store (step); }
    int laneLength (int layer) const { return (int) state.getRawParameterValue (bs::lid (layer, bs::L::len))->load(); }

    // UI feedback
    // Host clock snapshot for the on-screen playhead
    double getClockPpq() const   { return clockPpq.load(); }
    double getClockBpm() const   { return clockBpm.load(); }
    double getClockTimeMs() const { return clockTimeMs.load(); }
    int stepForDisplay (int layer, double ppq) const
    {
        return bs::laneStep (bs::stepAt (ppq, bs::rateQuarters ((int) state.getRawParameterValue (bs::lid (layer, bs::L::rate))->load()),
                                         0.5 * state.getRawParameterValue (bs::shuffle)->load()), laneLength (layer));
    }
    float getActivity (int layer) const  { return activity[(size_t) layer].load(); }
    bool isTransportRunning() const      { return running.load(); }
    int getPage() const                  { return page.load(); }
    void setPage (int p)                 { page.store (juce::jlimit (0, 3, p)); }
    juce::String getLastMidiText() const;

    void applyController (int channel, bool isNote, int number, int value);

    juce::AudioProcessorValueTreeState state;
    bs::Pattern pattern;
    bs::PresetManager presets { *this };

private:
    void timerCallback() override;
    float raw (const juce::String& id) const { return state.getRawParameterValue (id)->load(); }
    void fillParams();
    void fireStep (int layer, int step);

    struct CtrlEvent { int channel; bool isNote; int number; int value; };
    juce::AbstractFifo fifo { 512 };
    std::array<CtrlEvent, 512> fifoData {};

    bs::Engine engine;
    std::array<bs::LayerParams, 2> lp;
    bs::GlobalParams gp;
    bs::ModMatrix mm;
    double preparedRate = 0.0;
    std::array<long long, 2> lastStep { { LLONG_MIN, LLONG_MIN } };
    juce::Random audioRng { juce::Time::currentTimeMillis() }; // fresh dice every session

    std::array<std::atomic<int>, 2> auditionReq { { -1, -1 } };
    std::atomic<double> clockPpq { 0.0 }, clockBpm { 120.0 }, clockTimeMs { 0.0 };
    std::array<std::atomic<float>, 2> activity {};
    std::atomic<bool> running { false };
    std::atomic<int> page { 0 };
    std::atomic<int> lastMidiCh { 0 }, lastMidiNum { -1 }, lastMidiVal { 0 };
    std::atomic<bool> lastMidiIsNote { false };
    juce::Random uiRng;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BeatstationProcessor)
};
