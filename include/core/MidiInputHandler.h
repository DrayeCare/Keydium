#pragma once

#include <juce_audio_devices/juce_audio_devices.h>

#include <atomic>
#include <functional>
#include <mutex>
#include <vector>

namespace Core {

/** A decoded, thread-friendly MIDI note event. */
struct MidiNoteEvent {
    int note      = -1;
    int velocity  = 0;
    int channel   = 0;      // 1..16
    bool noteOn   = false;  // velocity-0 Note On is reported as Note Off
    double timestamp = 0.0;
};

/**
 * Owns one JUCE MIDI input device and delivers decoded note events on the
 * JUCE message thread. The audio/MIDI callback never touches UI or mapping
 * state directly: it decodes, filters and queues, then triggers AsyncUpdater.
 */
class MidiInputHandler : private juce::MidiInputCallback,
                         private juce::AsyncUpdater {
public:
    using EventCallback = std::function<void(const MidiNoteEvent&)>;

    MidiInputHandler();
    ~MidiInputHandler() override;

    /** Actual devices reported by the OS (never hardcoded). */
    juce::Array<juce::MidiDeviceInfo> getAvailableDevices() const;

    /** Opens and starts a device by its OS identifier. Closes any previous one. */
    bool openDevice(const juce::String& deviceIdentifier);
    void closeDevice();

    bool isConnected() const noexcept          { return accepting.load(); }
    juce::String getConnectedDeviceName() const;
    juce::String getConnectedIdentifier() const;

    /** 0 = Omni (all channels), 1..16 = single channel. */
    void setChannelFilter(int channel) noexcept;
    int  getChannelFilter() const noexcept     { return channelFilter.load(); }

    void setEventCallback(EventCallback cb);
    void clearEventCallback();

    // Pure helpers: usable in unit tests without any MIDI hardware.
    static bool decodeNoteMessage(const juce::MidiMessage& msg, MidiNoteEvent& out);
    static bool shouldAcceptChannel(int filter, int channel) noexcept;

private:
    void handleIncomingMidiMessage(juce::MidiInput* source,
                                   const juce::MidiMessage& message) override;
    void handleAsyncUpdate() override;

    std::unique_ptr<juce::MidiInput> midiInput;
    juce::String connectedIdentifier;
    juce::String connectedName;

    std::atomic<bool> accepting { false };
    std::atomic<int>  channelFilter { 0 };

    mutable std::mutex queueMutex;
    std::vector<MidiNoteEvent> pending;
    EventCallback eventCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiInputHandler)
};

} // namespace Core
