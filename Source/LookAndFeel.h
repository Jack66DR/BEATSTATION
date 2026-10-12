#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Factory machinery: hammertone industrial blue paint, bakelite knobs, cream engraving,
// amber pilot lamps, red mushroom kick buttons.
namespace bs::ui
{
    namespace col
    {
        const juce::Colour paint     { 0xff46647e };
        const juce::Colour paintHi   { 0xff5a7d9a };
        const juce::Colour recess    { 0xff31495f };
        const juce::Colour deep      { 0xff1b2733 };
        const juce::Colour cream     { 0xfff0ede6 };
        const juce::Colour creamDim  { 0xffd9d4c7 };
        const juce::Colour amber     { 0xfff2b33d };
        const juce::Colour red       { 0xffd0452f };
        const juce::Colour bakelite  { 0xff1a1c1b };
        const juce::Colour steelHi   { 0xffc3cac6 };
        const juce::Colour steelLo   { 0xff7b8580 };
        const juce::Colour hazard    { 0xffe8b520 };
    }

    inline juce::Font font (float h, bool bold = false)
    {
        return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain)).withExtraKerningFactor (0.06f);
    }

    class DSLook : public juce::LookAndFeel_V4
    {
    public:
        DSLook()
        {
            setColour (juce::ComboBox::backgroundColourId, col::deep);
            setColour (juce::ComboBox::textColourId, col::cream);
            setColour (juce::ComboBox::outlineColourId, col::recess.darker (0.3f));
            setColour (juce::ComboBox::arrowColourId, col::amber);
            setColour (juce::PopupMenu::backgroundColourId, col::deep);
            setColour (juce::PopupMenu::textColourId, col::cream);
            setColour (juce::PopupMenu::highlightedBackgroundColourId, col::paint);
            setColour (juce::PopupMenu::highlightedTextColourId, col::cream);
            setColour (juce::TextButton::buttonColourId, col::deep);
            setColour (juce::TextButton::textColourOffId, col::cream);
            setColour (juce::TextButton::textColourOnId, col::cream);
            setColour (juce::AlertWindow::backgroundColourId, col::recess);
            setColour (juce::AlertWindow::textColourId, col::cream);
            setColour (juce::TextEditor::backgroundColourId, col::deep);
            setColour (juce::TextEditor::textColourId, col::cream);
            setColour (juce::TextEditor::outlineColourId, col::paint);
        }

        juce::Font getComboBoxFont (juce::ComboBox&) override        { return font (15.0f, true); }
        juce::Font getPopupMenuFont() override                        { return font (16.0f); }
        juce::Font getTextButtonFont (juce::TextButton&, int) override { return font (15.0f, true); }
        juce::Font getAlertWindowMessageFont() override               { return font (16.0f); }

        void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                               float a0, float a1, juce::Slider& s) override
        {
            const bool big = w > 100;
            auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (big ? 10.0f : 4.0f);
            const float r = std::min (b.getWidth(), b.getHeight()) * 0.5f;
            const auto c = b.getCentre();
            const float ang = a0 + pos * (a1 - a0);
            const bool stepped = s.getInterval() >= 1.0;

            // scale ticks engraved into the paint
            g.setColour (col::cream.withAlpha (0.55f));
            const int ticks = stepped ? (int) (s.getMaximum() - s.getMinimum()) + 1 : 11;
            for (int i = 0; i < ticks; ++i)
            {
                const float a = a0 + (a1 - a0) * (float) i / (float) std::max (1, ticks - 1);
                const auto p1 = c.getPointOnCircumference (r + 1.0f, a), p2 = c.getPointOnCircumference (r - (big ? 6.0f : 3.0f), a);
                g.drawLine ({ p1, p2 }, big ? 2.0f : 1.2f);
            }

            // value arc
            const float rr = r - (big ? 9.0f : 5.0f);
            juce::Path arc;
            arc.addCentredArc (c.x, c.y, rr, rr, 0.0f, a0, ang, true);
            g.setColour ((big ? col::red : col::amber).withAlpha (0.9f));
            g.strokePath (arc, juce::PathStrokeType (big ? 4.0f : 2.5f));

            // bakelite body with knurled skirt
            const float kr = rr - (big ? 7.0f : 4.0f);
            g.setColour (juce::Colours::black.withAlpha (0.45f));
            g.fillEllipse (c.x - kr + 2.0f, c.y - kr + 3.0f, kr * 2.0f, kr * 2.0f);
            juce::ColourGradient body (col::bakelite.brighter (0.35f), c.x - kr * 0.5f, c.y - kr * 0.7f,
                                       col::bakelite, c.x + kr * 0.4f, c.y + kr * 0.8f, true);
            g.setGradientFill (body);
            g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);
            g.setColour (juce::Colours::white.withAlpha (0.07f));
            const int knurl = big ? 36 : 20;
            for (int i = 0; i < knurl; ++i)
            {
                const float a = juce::MathConstants<float>::twoPi * (float) i / (float) knurl;
                g.drawLine ({ c.getPointOnCircumference (kr, a), c.getPointOnCircumference (kr * 0.82f, a) }, 1.0f);
            }
            g.setColour (col::bakelite.brighter (0.15f));
            g.fillEllipse (c.x - kr * 0.72f, c.y - kr * 0.72f, kr * 1.44f, kr * 1.44f);

            // pointer
            g.setColour (col::cream);
            g.drawLine ({ c.getPointOnCircumference (kr * 0.2f, ang), c.getPointOnCircumference (kr * 0.92f, ang) }, big ? 4.0f : 2.4f);
        }

        void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos,
                               float, float, juce::Slider::SliderStyle, juce::Slider&) override
        {
            auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
            const float cx = b.getCentreX();
            // slot
            g.setColour (col::deep);
            g.fillRoundedRectangle (cx - 3.5f, b.getY(), 7.0f, b.getHeight(), 3.0f);
            // level fill
            g.setColour (col::amber.withAlpha (0.8f));
            g.fillRoundedRectangle (cx - 1.5f, sliderPos, 3.0f, b.getBottom() - sliderPos, 1.5f);
            // scale
            g.setColour (col::cream.withAlpha (0.45f));
            for (int i = 0; i <= 10; ++i)
            {
                const float ty = b.getY() + b.getHeight() * (float) i / 10.0f;
                const float len = (i % 5 == 0) ? 9.0f : 5.0f;
                g.drawLine (cx - 10.0f - len, ty, cx - 10.0f, ty, 1.0f);
            }
            // cap
            auto cap = juce::Rectangle<float> (cx - 14.0f, sliderPos - 9.0f, 28.0f, 18.0f);
            g.setColour (juce::Colours::black.withAlpha (0.4f));
            g.fillRoundedRectangle (cap.translated (1.5f, 2.5f), 3.0f);
            juce::ColourGradient grad (col::bakelite.brighter (0.4f), cap.getX(), cap.getY(), col::bakelite, cap.getX(), cap.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (cap, 3.0f);
            g.setColour (col::cream);
            g.drawLine (cap.getX() + 4.0f, cap.getCentreY(), cap.getRight() - 4.0f, cap.getCentreY(), 2.0f);
        }

        void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool down) override
        {
            auto r = b.getLocalBounds().toFloat().reduced (1.0f);
            const bool on = b.getToggleState();
            g.setColour (col::deep);
            g.fillRoundedRectangle (r, 4.0f);
            auto lamp = r.removeFromLeft (22.0f).withSizeKeepingCentre (14.0f, 14.0f).translated (2.0f, 0.0f);
            if (on)
            {
                g.setColour (col::amber.withAlpha (0.35f));
                g.fillEllipse (lamp.expanded (4.0f));
            }
            g.setColour (on ? col::amber : col::amber.darker (2.2f));
            g.fillEllipse (lamp);
            g.setColour (juce::Colours::white.withAlpha (on ? 0.6f : 0.15f));
            g.fillEllipse (lamp.reduced (lamp.getWidth() * 0.3f).translated (-1.5f, -1.5f));
            g.setColour ((over || down) ? col::cream : col::creamDim);
            g.setFont (font (15.0f, true));
            g.drawFittedText (b.getButtonText(), r.reduced (2.0f, 0.0f).toNearestInt(), juce::Justification::centred, 1, 0.9f);
        }

        void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
        {
            auto r = b.getLocalBounds().toFloat().reduced (1.0f);
            const bool kick = b.getProperties().getWithDefault ("kick", false);
            if (kick)
            {
                g.setColour (col::deep);
                g.fillRoundedRectangle (r, 4.0f);
                auto m = r.withSizeKeepingCentre (18.0f, 18.0f);
                if (r.getWidth() >= 70.0f) m = m.withX (r.getX() + 4.0f);
                juce::ColourGradient grad (col::red.brighter (down ? 0.6f : 0.25f), m.getX(), m.getY(), col::red.darker (0.4f), m.getX(), m.getBottom(), false);
                g.setGradientFill (grad);
                g.fillEllipse (m);
                return;
            }
            const bool envOn = b.getProperties().getWithDefault ("env", false) && b.getToggleState();
            g.setColour (down ? col::paint : (over ? col::recess.brighter (0.2f) : col::deep));
            g.fillRoundedRectangle (r, 4.0f);
            g.setColour (envOn ? col::amber : col::paintHi.withAlpha (0.6f));
            g.drawRoundedRectangle (r, 4.0f, envOn ? 2.0f : 1.0f);
        }

        void drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool) override
        {
            const bool kick = b.getProperties().getWithDefault ("kick", false);
            g.setFont (font (15.0f, true));
            g.setColour (b.getProperties().getWithDefault ("env", false) && b.getToggleState() ? col::amber : col::cream);
            auto r = b.getLocalBounds();
            if (kick && r.getWidth() < 70) return; // mushroom only
            if (kick) r.removeFromLeft (24);
            g.drawFittedText (b.getButtonText(), r, juce::Justification::centred, 1, 0.9f);
        }
    };
}
