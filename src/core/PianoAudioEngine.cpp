#include "core/PianoAudioEngine.h"

#include <fluidsynth.h>

#include <atomic>
#include <mutex>
#include <cstdio>
#include <cmath>

namespace Core {

void PianoAudioEngine::downmixStereoToMono(float* out[], int nout, int len) {
    if (nout < 1 || !out || len <= 0) return;

    if (nout >= 2 && out[0] && out[1]) {
        for (int i = 0; i < len; ++i) {
            // Balanced mono downmix with 0.5 scaling avoids any digital clipping
            // when left and right are both at peak amplitude.
            float mono = 0.5f * (out[0][i] + out[1][i]);
            out[0][i] = mono;
            out[1][i] = mono;
        }

        // Copy identical mono signal to any surround channels if present
        for (int ch = 2; ch < nout; ++ch) {
            if (out[ch]) {
                for (int i = 0; i < len; ++i) {
                    out[ch][i] = out[0][i];
                }
            }
        }
    }
}

struct PianoAudioEngine::Impl {
    fluid_settings_t*     settings    = nullptr;
    fluid_synth_t*        synth       = nullptr;
    fluid_audio_driver_t* audioDriver = nullptr;

    int sfontId     = -1;
    int midiChannel = 0; // MIDI channel 0 (GM channel 1)

    std::atomic<bool> enabled     { false };
    std::atomic<bool> initialised { false };

    mutable std::mutex engineMutex;

    ~Impl() {
        shutdown();
    }

    static int audioCallback(void* data, int len, int nfx, float* fx[], int nout, float* out[]) {
        auto* self = static_cast<Impl*>(data);
        if (!self || !self->synth) return FLUID_FAILED;

        int ret = 0;
        if (nfx == 0 && nout >= 2) {
            float* fxb[4] = { out[0], out[1], out[0], out[1] };
            ret = fluid_synth_process(self->synth, len, 4, fxb, nout, out);
        } else {
            ret = fluid_synth_process(self->synth, len, nfx, fx, nout, out);
        }

        if (ret != FLUID_OK) return ret;

        // Apply true mono downmix to all channels
        PianoAudioEngine::downmixStereoToMono(out, nout, len);

        return FLUID_OK;
    }

    bool initialise(const std::string& soundFontPath) {
        std::lock_guard<std::mutex> lock(engineMutex);

        if (initialised.load()) return true;

        settings = new_fluid_settings();
        if (!settings) {
            fprintf(stderr, "[PianoAudioEngine] Failed to create FluidSynth settings.\n");
            return false;
        }

        fluid_settings_setstr(settings, "audio.driver", "dsound");
        fluid_settings_setnum(settings, "synth.sample-rate", 44100.0);
        fluid_settings_setint(settings, "synth.polyphony", 64);
        fluid_settings_setnum(settings, "synth.gain", 0.8);

        synth = new_fluid_synth(settings);
        if (!synth) {
            fprintf(stderr, "[PianoAudioEngine] Failed to create FluidSynth synthesiser.\n");
            delete_fluid_settings(settings);
            settings = nullptr;
            return false;
        }

        sfontId = fluid_synth_sfload(synth, soundFontPath.c_str(), 1);
        if (sfontId == FLUID_FAILED) {
            fprintf(stderr, "[PianoAudioEngine] Could not load SoundFont: %s\n", soundFontPath.c_str());
            delete_fluid_synth(synth);
            delete_fluid_settings(settings);
            synth    = nullptr;
            settings = nullptr;
            return false;
        }

        // Select Acoustic Grand Piano (bank 0, program 0)
        fluid_synth_bank_select(synth, midiChannel, 0);
        fluid_synth_program_change(synth, midiChannel, 0);

        // Use custom callback audio driver to downmix all stereo buffers to mono
        audioDriver = new_fluid_audio_driver2(settings, audioCallback, this);
        if (!audioDriver) {
            // B2 fix: FluidSynth has no "winmm" driver. The official Windows
            // build ships dsound, waveout and wasapi; waveout is the safest,
            // most widely supported fallback when dsound fails.
            fluid_settings_setstr(settings, "audio.driver", "waveout");
            audioDriver = new_fluid_audio_driver2(settings, audioCallback, this);
        }

        if (!audioDriver) {
            fprintf(stderr, "[PianoAudioEngine] Failed to create audio driver.\n");
            fluid_synth_sfunload(synth, sfontId, 0);
            delete_fluid_synth(synth);
            delete_fluid_settings(settings);
            synth    = nullptr;
            settings = nullptr;
            sfontId  = -1;
            return false;
        }

        initialised.store(true);
        enabled.store(true);
        fprintf(stdout, "[PianoAudioEngine] Initialised OK (Mono Output). SoundFont id=%d\n", sfontId);
        return true;
    }

    void shutdown() {
        std::lock_guard<std::mutex> lock(engineMutex);

        if (!initialised.load()) return;

        if (synth) {
            fluid_synth_all_notes_off(synth, -1);
        }

        // Delete audio driver first so audio callback stops firing
        if (audioDriver) {
            delete_fluid_audio_driver(audioDriver);
            audioDriver = nullptr;
        }
        if (synth) {
            delete_fluid_synth(synth);
            synth = nullptr;
        }
        if (settings) {
            delete_fluid_settings(settings);
            settings = nullptr;
        }

        sfontId = -1;
        initialised.store(false);
        enabled.store(false);
        fprintf(stdout, "[PianoAudioEngine] Shut down.\n");
    }

    void noteOn(int midiNote, int velocity) {
        if (!enabled.load() || !initialised.load()) return;
        if (midiNote < 0 || midiNote > 127) return;
        if (velocity < 0) velocity = 0;
        if (velocity > 127) velocity = 127;

        std::lock_guard<std::mutex> lock(engineMutex);
        if (synth) {
            fluid_synth_noteon(synth, midiChannel, midiNote, velocity);
        }
    }

    void noteOff(int midiNote) {
        if (!initialised.load()) return;
        if (midiNote < 0 || midiNote > 127) return;

        std::lock_guard<std::mutex> lock(engineMutex);
        if (synth) {
            fluid_synth_noteoff(synth, midiChannel, midiNote);
        }
    }

    void allNotesOff() {
        std::lock_guard<std::mutex> lock(engineMutex);
        if (synth) {
            fluid_synth_all_notes_off(synth, -1);
        }
    }

    void setEnabled(bool shouldBeEnabled) {
        if (!shouldBeEnabled && initialised.load()) {
            std::lock_guard<std::mutex> lock(engineMutex);
            if (synth) {
                fluid_synth_all_notes_off(synth, -1);
            }
        }
        enabled.store(shouldBeEnabled);
    }

    bool renderAudio(float* left, float* right, int numSamples) {
        std::lock_guard<std::mutex> lock(engineMutex);
        if (!synth || !left || !right || numSamples <= 0) return false;

        float* out[2] = { left, right };
        float* fxb[4] = { left, right, left, right };
        int ret = fluid_synth_process(synth, numSamples, 4, fxb, 2, out);
        if (ret != FLUID_OK) return false;

        PianoAudioEngine::downmixStereoToMono(out, 2, numSamples);
        return true;
    }
};

PianoAudioEngine::PianoAudioEngine() : impl(std::make_unique<Impl>()) {}
PianoAudioEngine::~PianoAudioEngine() = default;
PianoAudioEngine::PianoAudioEngine(PianoAudioEngine&&) noexcept = default;
PianoAudioEngine& PianoAudioEngine::operator=(PianoAudioEngine&&) noexcept = default;

bool PianoAudioEngine::initialise(const std::string& soundFontPath) {
    return impl->initialise(soundFontPath);
}

void PianoAudioEngine::shutdown() {
    impl->shutdown();
}

void PianoAudioEngine::noteOn(int midiNote, int velocity) {
    impl->noteOn(midiNote, velocity);
}

void PianoAudioEngine::noteOff(int midiNote) {
    impl->noteOff(midiNote);
}

void PianoAudioEngine::allNotesOff() {
    impl->allNotesOff();
}

void PianoAudioEngine::setEnabled(bool shouldBeEnabled) {
    impl->setEnabled(shouldBeEnabled);
}

bool PianoAudioEngine::isEnabled() const {
    return impl->enabled.load();
}

bool PianoAudioEngine::isInitialised() const {
    return impl->initialised.load();
}

std::string PianoAudioEngine::getStatusString() const {
    if (!impl->initialised.load()) return "Audio: OFF (not initialised)";
    if (!impl->enabled.load())     return "Audio: Muted";
    return "Audio: Grand Piano (FluidSynth Mono)";
}

bool PianoAudioEngine::renderAudio(float* left, float* right, int numSamples) {
    return impl->renderAudio(left, right, numSamples);
}

} // namespace Core
