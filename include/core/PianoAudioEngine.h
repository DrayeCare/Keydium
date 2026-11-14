#pragma once

#include <string>
#include <memory>

namespace Core {

/**
 * @class PianoAudioEngine
 * @brief Real-time Grand Piano audio engine backed by FluidSynth.
 *
 * Uses Pimpl idiom so no FluidSynth types or headers leak into Keydium headers.
 * All public methods are thread-safe and safe to call from the JUCE message thread.
 * Audio driver renders true mono audio downmixed identically to left and right channels.
 */
class PianoAudioEngine {
public:
    PianoAudioEngine();
    ~PianoAudioEngine();

    // Non-copyable, movable
    PianoAudioEngine(const PianoAudioEngine&) = delete;
    PianoAudioEngine& operator=(const PianoAudioEngine&) = delete;
    PianoAudioEngine(PianoAudioEngine&&) noexcept;
    PianoAudioEngine& operator=(PianoAudioEngine&&) noexcept;

    /**
     * Load the SoundFont and start the mono audio driver.
     * @param soundFontPath  Absolute or relative path to a .sf2 file.
     * @return true if FluidSynth initialised successfully.
     */
    bool initialise(const std::string& soundFontPath);

    /** Stop the audio driver and free all FluidSynth resources. */
    void shutdown();

    /** Send Note-On. Ignored when disabled or not initialised. */
    void noteOn(int midiNote, int velocity = 100);

    /** Send Note-Off. Ignored when not initialised. */
    void noteOff(int midiNote);

    /** Silence all currently playing voices immediately. */
    void allNotesOff();

    /** Enable or disable audio output. While disabled, noteOn is a no-op. */
    void setEnabled(bool shouldBeEnabled);

    bool isEnabled() const;
    bool isInitialised() const;

    /** Human-readable status for the UI. */
    std::string getStatusString() const;

    /**
     * Downmix stereo/multi-channel buffers to mono:
     * mono = 0.5 * (out[0] + out[1]).
     * Mono signal is written identically to both out[0] and out[1].
     */
    static void downmixStereoToMono(float* out[], int nout, int len);

    /**
     * Synthesise audio samples into buffers and apply mono downmix.
     * Useful for automated unit testing and offline verification.
     */
    bool renderAudio(float* left, float* right, int numSamples);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace Core
