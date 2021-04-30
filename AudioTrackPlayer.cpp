#include "AudioTrackPlayer.h"
#include "Exception.h"
#include "portaudio.h"
#include <iostream>

static int prioritized_sample_rates[] = {
    48000,
    44100,
    96000,
    24000,
    0,
};

vc::model::AudioTrackPlayer::AudioTrackPlayer()
    : mStream(nullptr)
{
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        throw vc::Exception("portaudio", Pa_GetErrorText(err));
    }
}

vc::model::AudioTrackPlayer::~AudioTrackPlayer()
{
    releaseResources();
    
    PaError err = Pa_Terminate();
    if (err != paNoError) {
        throw vc::Exception("portaudio", Pa_GetErrorText(err));
    }
}

void vc::model::AudioTrackPlayer::releaseResources()
{
    PaError err;
    if (mStream != nullptr) {
        if (Pa_IsStreamActive(mStream)) {
            err = Pa_AbortStream(mStream);
            if (err != paNoError) {
                throw vc::Exception("portaudio", Pa_GetErrorText(err));
            }
        }
        err = Pa_CloseStream(mStream);
        if (err != paNoError) {
            throw vc::Exception("portaudio", Pa_GetErrorText(err));
        }
        mStream = nullptr;
    }
}

void vc::model::AudioTrackPlayer::resetForTrack(const AudioTrack *track)
{
    releaseResources();

    mResampledAudio = track->mAudio;
    mSampleRate = track->mSampleRate;

    PaError err = Pa_OpenDefaultStream(&mStream,
                                        0,
                                        1,
                                        paFloat32,
                                        mSampleRate,
                                        1024,
                                        &AudioTrackPlayer::streamCallback,
                                        this);

    if (err != paNoError) {
        throw vc::Exception("portaudio", Pa_GetErrorText(err));
    }

    mIsPlaying = false;
    mCursor = 0;
}

void vc::model::AudioTrackPlayer::pause(bool paused)
{
    PaError err;
    if (!paused) {
        err = Pa_StartStream(mStream);
    }
    else {
        err = Pa_StopStream(mStream);
    }
    if (err != paNoError) {
        throw vc::Exception("portaudio", Pa_GetErrorText(err));
    }

    mIsPlaying = !paused;
}

bool vc::model::AudioTrackPlayer::isPlaying() const
{
    return mIsPlaying;
}

void vc::model::AudioTrackPlayer::seek(const double time)
{
    mCursor = (int) std::round(time * mSampleRate);
}

double vc::model::AudioTrackPlayer::position() const
{
    return double(mCursor) / double(mSampleRate);
}

double vc::model::AudioTrackPlayer::duration() const
{
    return double(mResampledAudio.size()) / double(mSampleRate);
}

int vc::model::AudioTrackPlayer::streamCallback(const void *input,
                                                void *output,
                                                unsigned long frameCount,
                                                const PaStreamCallbackTimeInfo *timeInfo,
                                                PaStreamCallbackFlags statusFlags,
                                                void *userData)
{
    auto player = static_cast<AudioTrackPlayer *>(userData);

    float *fOutput = reinterpret_cast<float *>(output);

    unsigned long frame, cursor;
    for (frame = 0, cursor = player->mCursor; frame < frameCount; ++frame, ++cursor) {
        double sample = (cursor >= 0 && cursor < player->mResampledAudio.size())
                ? player->mResampledAudio[cursor]
                : 0.0;
        fOutput[frame] = float(sample);
    }
    player->mCursor = cursor;

    return paContinue;
}
