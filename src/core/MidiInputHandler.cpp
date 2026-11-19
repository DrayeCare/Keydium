#include "core/MidiInputHandler.h"

namespace Core {

MidiInputHandler::MidiInputHandler() = default;

MidiInputHandler::~MidiInputHandler() {
    // Stop the callback source first; no callback can reference this after reset.
    closeDevice();
    clearEventCallback();
}

juce::Array<juce::MidiDeviceInfo> MidiInputHandler::getAvailableDevices() const {
    return juce::MidiInput::getAvailableDevices();
}

bool MidiInputHandler::openDevice(const juce::String& deviceIdentifier) {
    closeDevice();
    if (deviceIdentifier.isEmpty()) return false;

    // Resolve a human-readable name from the live OS device list.
    juce::String name = deviceIdentifier;
    for (const auto& info : getAvailableDevices()) {
        if (info.identifier == deviceIdentifier) {
            name = info.name;
            break;
        }
    }

    auto input = juce::MidiInput::openDevice(deviceIdentifier, this);
    if (input == nullptr) return false;

    // Start before publishing the device: early messages are ignored because
    // accepting is still false.
    input->start();
    midiInput = std::move(input);
    connectedIdentifier = deviceIdentifier;
    connectedName = name;
    accepting.store(true);
    return true;
}

void MidiInputHandler::closeDevice() {
    accepting.store(false);
    if (midiInput) {
        midiInput->stop();
        midiInput.reset();
    }
    connectedIdentifier.clear();
    connectedName.clear();

    // Events queued from a device that is being switched away from must not
    // fire after the close.
    std::lock_guard<std::mutex> lock(queueMutex);
    pending.clear();
}

juce::String MidiInputHandler::getConnectedDeviceName() const {
    return connectedName;
}

juce::String MidiInputHandler::getConnectedIdentifier() const {
    return connectedIdentifier;
}

void MidiInputHandler::setChannelFilter(int channel) noexcept {
    channelFilter.store((channel >= 1 && channel <= 16) ? channel : 0);
}

bool MidiInputHandler::shouldAcceptChannel(int filter, int channel) noexcept {
    if (filter < 1 || filter > 16) return true; // Omni
    return channel >= 1 && channel <= 16 && channel == filter;
}

bool MidiInputHandler::decodeNoteMessage(const juce::MidiMessage& msg,
                                          MidiNoteEvent& out) {
    if (!msg.isNoteOnOrOff()) return false;

    out.note      = msg.getNoteNumber();
    out.velocity  = static_cast<int>(msg.getVelocity());
    out.channel   = msg.getChannel();
    // isNoteOff(true) treats Note On with velocity 0 as Note Off (MIDI spec).
    out.noteOn    = !msg.isNoteOff(true);
    out.timestamp = msg.getTimeStamp();
    return true;
}

void MidiInputHandler::handleIncomingMidiMessage(juce::MidiInput* source,
                                                  const juce::MidiMessage& message) {
    // High-priority system thread: decode, filter, enqueue. No UI, no mapping.
    if (!accepting.load() || source != midiInput.get()) return;

    MidiNoteEvent event;
    if (!decodeNoteMessage(message, event)) return;
    if (!shouldAcceptChannel(channelFilter.load(), event.channel)) return;

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        pending.push_back(event);
    }
    triggerAsyncUpdate();
}

void MidiInputHandler::setEventCallback(EventCallback cb) {
    std::lock_guard<std::mutex> lock(queueMutex);
    eventCallback = std::move(cb);
}

void MidiInputHandler::clearEventCallback() {
    std::lock_guard<std::mutex> lock(queueMutex);
    eventCallback = nullptr;
}

void MidiInputHandler::handleAsyncUpdate() {
    // Message thread: safe to call into mapping / simulation / UI state.
    std::vector<MidiNoteEvent> batch;
    EventCallback callback;
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        batch.swap(pending);
        callback = eventCallback;
    }
    if (!callback) return;
    for (const auto& event : batch) {
        callback(event);
    }
}

} // namespace Core
