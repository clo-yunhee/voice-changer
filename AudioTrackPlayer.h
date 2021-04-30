#ifndef AUDIOTRACKPLAYER_H
#define AUDIOTRACKPLAYER_H

#include <atomic>
#include <vector>
#include <portaudio.h>
#include "AudioTrack.h"

#define PI 3.1415926535897932384626433832795028841971693993751058209749445923078164062

namespace vc::model {

class AudioTrackPlayer {
public:
    AudioTrackPlayer();
    ~AudioTrackPlayer();

    void releaseResources();
    void resetForTrack(const AudioTrack *track);
    void pause(bool paused = true);
    bool isPlaying() const;
    void seek(double time);
    double position() const;
    double duration() const;

private:
    static int streamCallback(const void *input,
                                void *output,
                                unsigned long frameCount,
                                const PaStreamCallbackTimeInfo *timeInfo,
                                PaStreamCallbackFlags statusFlags,
                                void *userData);

    PaStream *mStream;
    bool mIsPlaying;
    std::atomic<unsigned long> mCursor;

    std::vector<double> mResampledAudio;
    int mSampleRate;
};

}

#endif // AUDIOTRACKPLAYER_H