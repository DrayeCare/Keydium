#include <JuceHeader.h>
#include "gui/MainComponent.h"

class MidiPianoApplication : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override { return "Midi Piano KeyMapper"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String&) override {
        mainWindow.reset(new MainWindow(getApplicationName()));
    }

    void shutdown() override {
        mainWindow = nullptr;
    }

    class MainWindow : public juce::DocumentWindow {
    public:
        MainWindow(juce::String name)
            : DocumentWindow(name, juce::Colours::black, DocumentWindow::allButtons) {
            setUsingNativeTitleBar(true);
            setContentOwned(new MainComponent(), true);
            setResizable(true, true);
            centreWithSize(1200, 680); // <-- UKURAN BARU BIAR GAK KETUTUP DOCK
            setVisible(true);
        }

        void closeButtonPressed() override {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(MidiPianoApplication)