// Renders the editor to a PNG (used under Xvfb to check the layout).
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

struct Host : juce::AudioPlayHead
{
    double ppq = 0;
    juce::Optional<PositionInfo> getPosition() const override { PositionInfo p; p.setIsPlaying (true); p.setPpqPosition (ppq); p.setBpm (120); return p; }
};

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File out (argc > 1 ? juce::String (argv[1]) : juce::String ("snapshot.png"));
    BeatstationProcessor p; Host h; p.setPlayHead (&h);
    p.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> buf (2, 512); juce::MidiBuffer m;
    for (int i = 0; i < 120; ++i) { buf.clear(); p.processBlock (buf, m); h.ppq += 512 * 2.0 / 48000.0; }
    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    ed->setVisible (true);
    juce::MessageManager::getInstance()->runDispatchLoopUntil (300);
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    out.deleteFile();
    juce::FileOutputStream os (out);
    juce::PNGImageFormat().writeImageToStream (img, os);
    std::printf ("wrote %s\n", out.getFullPathName().toRawUTF8());
    return 0;
}
