#ifndef AUDIOTRACK_H
#define AUDIOTRACK_H

#include <vector>
#include <string>
#include "reaper/reaper.h"

namespace vc::model {

class AudioTrack {
public:
    AudioTrack();
    
    bool loadFromFile(const std::string& path);
    bool loadFromWavFile(const std::string& path);
    bool loadFromFlacFile(const std::string& path);
    bool loadFromMp3File(const std::string& path);

    bool saveToWavFile(const std::string& path);

    void replace(const std::vector<double>& audio, int sampleRate);

    void resample(int targetSampleRate);

    AudioTrack clone() const;
    AudioTrack resampled(int targetSampleRate) const;
    Track *toPitchTrack() const;

    int sampleRate() const;
    double duration() const;

private:
    int mSampleRate;
    std::vector<double> mAudio;
};

template<typename In, typename Out>
void mixDownToMono(const In *pIn, uint64_t frameCount, unsigned int channels, Out *pOut)
{
    for (uint64_t i = 0; i < frameCount; ++i) {
        pOut[i] = Out(0.0);
        for (unsigned int c = 0; c < channels; ++c)
            pOut[i] += Out(pIn[i * channels + c]);
        pOut[i] /= Out(channels);
    }
}

}

#endif // AUDIOTRACK_H