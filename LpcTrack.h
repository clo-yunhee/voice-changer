#ifndef LPCTRACK_H
#define LPCTRACK_H

#include <vector>
#include "reaper/core/track.h"

namespace vc::model {

class LpcTrack {
public:
    LpcTrack(Track *pitchTrack, double preemphFrequency, double sampleRate);

    void fillFromAudio(const std::vector<double>& audio);
    void applyToAudio(std::vector<double>& audio) const;

    void pushCoefficients(const std::vector<double>& lpca);
    void pushAudio(const std::vector<double>& audio);

    void applyFrequencyShift(double shiftFactor);

private:
    int windowLengthAtTime(int index) const;
    int windowSpacingAtTime(int index) const;

    int windowLengthAtPitchTrackIndex(int index) const;
    int windowSpacingAtPitchTrackIndex(int index) const;

    int calculateWindowLengthAtPitchTrackIndex(int index, int depth = 0);
    int calculateWindowSpacingAtPitchTrackIndex(int index, int depth = 0);

    Track *mPitchTrack;
    double mPreemphFrequency;
    double mSampleRate;

    std::vector<int> mWindowLengths;
    std::vector<int> mWindowSpacings;

    std::vector<std::vector<double>> mTrack;
};

}

#endif // LPCTRACK_H