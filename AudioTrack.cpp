#include "AudioTrack.h"
#include "Exception.h"
#include <memory>
#include <fstream>

#include <SpeexResampler.h>

#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>

#define DR_FLAC_IMPLEMENTATION
#include <dr_flac.h>

#define DR_MP3_IMPLEMENTATION
#include <dr_mp3.h>

vc::model::AudioTrack::AudioTrack()
    : mSampleRate(48'000),
      mAudio(5 * mSampleRate, 0.0)
{
    // Default is generating 5 seconds of silence at 48'000 Hz.
}

bool vc::model::AudioTrack::loadFromFile(const std::string& path)
{
    // Instead of relying on file name extension we'll just try
    // to decode the file in several different file formats.

    return loadFromWavFile(path)
        || loadFromFlacFile(path)
        || loadFromMp3File(path);
}

bool vc::model::AudioTrack::loadFromWavFile(const std::string& path)
{
    drwav wav;
    if (!drwav_init_file(&wav, path.c_str(), nullptr)) {
        return false;
    }

    auto sampleData = std::make_unique<float[]>(wav.totalPCMFrameCount * wav.channels);
    drwav_read_pcm_frames_f32(&wav, wav.totalPCMFrameCount, sampleData.get());

    mSampleRate = wav.sampleRate;

    // Mix down to mono.
    mAudio.resize(wav.totalPCMFrameCount);
    mixDownToMono(sampleData.get(), wav.totalPCMFrameCount, wav.channels, mAudio.data());

    drwav_uninit(&wav);

    return true;
}

bool vc::model::AudioTrack::loadFromFlacFile(const std::string& path)
{
    drflac *pFlac = drflac_open_file(path.c_str(), nullptr);
    if (pFlac == nullptr) {
        return false;
    }

    auto sampleData = std::make_unique<float[]>(pFlac->totalPCMFrameCount * pFlac->channels);
    drflac_read_pcm_frames_f32(pFlac, pFlac->totalPCMFrameCount, sampleData.get());
    
    mSampleRate = pFlac->sampleRate;

    // Mix down to mono.
    mAudio.resize(pFlac->totalPCMFrameCount);
    mixDownToMono(sampleData.get(), pFlac->totalPCMFrameCount, pFlac->channels, mAudio.data());

    drflac_close(pFlac);

    return true;
}

bool vc::model::AudioTrack::loadFromMp3File(const std::string& path)
{
    drmp3 mp3;
    if (!drmp3_init_file(&mp3, path.c_str(), nullptr)) {
        return false;
    }
    
    drmp3_config config;
    drmp3_uint64 totalPCMFrameCount;
    float *pSampleData = drmp3__full_read_and_close_f32(&mp3, &config, &totalPCMFrameCount);

    mSampleRate = config.sampleRate;

    // Mix down to mono.
    mAudio.resize(totalPCMFrameCount);
    mixDownToMono(pSampleData, totalPCMFrameCount, config.channels, mAudio.data());

    drmp3_free(pSampleData, nullptr);

    return true;
}

bool vc::model::AudioTrack::saveToWavFile(const std::string& path)
{
    {
        // Touch file.
        std::ofstream touchFile(path);
    }

    drwav_data_format format;
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_PCM;
    format.channels = 1;
    format.sampleRate = mSampleRate;
    format.bitsPerSample = 16;

    drwav wav;
    if (!drwav_init_file_write(&wav, path.c_str(), &format, nullptr)) {
        return false;
    }

    auto sampleData = std::make_unique<drwav_int16[]>(mAudio.size());
    drwav_f64_to_s16(sampleData.get(), mAudio.data(), mAudio.size());

    drwav_uint64 totalFramesToWrite = mAudio.size();
    drwav_uint64 totalFramesWritten = 0;

    constexpr drwav_uint64 bufsz = 4096;

    while (totalFramesToWrite > totalFramesWritten) {
        drwav_uint64 framesRemaining = totalFramesToWrite - totalFramesWritten;
        drwav_uint64 framesToWriteNow = bufsz;
        if (framesToWriteNow > framesRemaining) {
            framesToWriteNow = framesRemaining;
        }

        drwav_write_pcm_frames(&wav, framesToWriteNow, sampleData.get() + totalFramesWritten);

        totalFramesWritten += framesToWriteNow;
    }

    drwav_uninit(&wav);

    return true;
}

void vc::model::AudioTrack::replace(const std::vector<double>& audio, const int sampleRate)
{
    mSampleRate = sampleRate;
    mAudio = audio;
}

void vc::model::AudioTrack::resample(const int targetSampleRate)
{
    speexport::SpeexResampler resampler;
    int err;
    
    err = resampler.init(1, mSampleRate, targetSampleRate, 10, nullptr);
    if (err != speexport::RESAMPLER_ERR_SUCCESS) {
        throw vc::Exception("Error initializing resampler", speexport::speex_resampler_strerror(err));
    }

    resampler.skip_zeros();

    std::vector<float> in(mAudio.begin(), mAudio.end());
    std::vector<float> out((mAudio.size() * mSampleRate) / targetSampleRate + 1);
    auto inLen = speexport::spx_uint32_t(in.size());
    auto outLen = speexport::spx_uint32_t(out.size());

    resampler.process(0, in.data(), &inLen, out.data(), &outLen);
    out.resize(outLen);

    mSampleRate = targetSampleRate;
    mAudio.resize(outLen);
    std::copy(out.begin(), out.end(), mAudio.begin());
}

vc::model::AudioTrack vc::model::AudioTrack::clone() const
{
    vc::model::AudioTrack copy;
    copy.mSampleRate = mSampleRate;
    copy.mAudio = mAudio;
    return copy;
}

vc::model::AudioTrack vc::model::AudioTrack::resampled(int targetSampleRate) const
{
    auto copy = this->clone();
    copy.resample(targetSampleRate);
    return copy;
}

Track *vc::model::AudioTrack::toPitchTrack() const
{
    auto trackAt16kHz = resampled(16'000);
    Track *f0 = nullptr;
    REAPER::Analyze(trackAt16kHz.mAudio, trackAt16kHz.mSampleRate, &f0);
    return f0;
}

vc::model::LpcTrack vc::model::AudioTrack::toLpcTrack(Track *pitchTrack, const double preemphFrequency) const
{
    vc::model::LpcTrack lpcTrack(pitchTrack, preemphFrequency, mSampleRate);
    lpcTrack.fillFromAudio(mAudio);
    return lpcTrack;
}

int vc::model::AudioTrack::sampleRate() const
{
    return mSampleRate;
}

double vc::model::AudioTrack::duration() const
{
    return double(mAudio.size()) / double(mSampleRate);
}