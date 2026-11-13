#pragma once
#include <JuceHeader.h>
#include <map>
#include <set>
#include "PianoKeyboard.h"

class MainComponent : public juce::Component {
public:
    MainComponent();
    ~MainComponent() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    
private:
    enum class ActiveTab { Mapping, Latency, Sandbox, Guide };
    ActiveTab currentTab = ActiveTab::Mapping;

    // Tab buttons
    juce::TextButton tabMapping   {"Mapping & Piano GUI"};
    juce::TextButton tabLatency   {"Latency & Delay Timer"};
    juce::TextButton tabSandbox   {"Game & Typing Sandbox"};
    juce::TextButton tabGuide     {"Guide"};
    juce::TextButton exportBtn    {"Export Native Script"};
    
    // MIDI controls
    juce::Label      midiInputLabel {"", "MIDI Input:"};
    juce::ComboBox   midiInputCombo;
    juce::Label      channelLabel   {"", "Channel:"};
    juce::ComboBox   channelCombo;
    juce::Label      soundLabel     {"", "Sound:"};
    juce::ComboBox   soundCombo;
    
    // Piano panel header
    juce::Label      pianoTitle   {"", "Interactive Piano Keybed"};
    juce::Label      pianoSubtitle{"", "Left-click to test | Right-click to assign key"};
    juce::TextButton octaveDownBtn {"<"};
    juce::TextButton octaveUpBtn   {">"};
    juce::Label      octaveLabel   {"", "Octave 2"};
    juce::TextButton keys49Btn {"49-Key"};
    juce::TextButton keys61Btn {"61-Key"};
    juce::TextButton keys88Btn {"88-Key"};
    
    PianoKeyboard pianoKeyboard;
    
    // Legend — bullet digambar vektor di paint(), string di sini plain ASCII
    juce::Label legendWhite {"", "White Keys"};
    juce::Label legendBlack {"", "Black Keys"};
    juce::Label legendMapped{"", "Active / Mapped Key Badge"};
    
    // Bottom keyboard matrix panel
    juce::Label matrixTitle {"", "Simulated PC Keyboard Output Matrix"};
    juce::Label matrixSub   {"", "Keys light up when triggered by your piano"};
    juce::Label activeKeysLabel {"", "Active Keys: 0"};

    // Sandbox / Placeholder components untuk tab lain
    juce::TextEditor sandboxEditor;
    
    // State mapping & tombol aktif
    std::map<int, juce::String> noteToKeyMap;
    std::set<juce::String> activePcKeys;
    juce::Rectangle<int> matrixArea;
    
    void setupStyle();
    void setupCallbacks();
    void switchTab(ActiveTab newTab);
    void setKeyCountMode(int whiteKeys, juce::TextButton& activeBtn);
    void drawKeyboardMatrix(juce::Graphics& g, juce::Rectangle<int> bounds);
    void promptAssignKey(int midiNote);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};