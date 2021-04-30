#include "LpcTrack.h"
#include "Lpc.h"
#include <wx/app.h>
#include <cmath>

#include <iostream>
#include "Exception.h"

vc::model::LpcTrack::LpcTrack(Track *pitchTrack, double preemphFrequency, double sampleRate)
    : mPitchTrack(pitchTrack),
      mPreemphFrequency(preemphFrequency),
      mSampleRate(sampleRate),
      mWindowLengths(pitchTrack->num_frames(), -1),
      mWindowSpacings(pitchTrack->num_frames(), -1)
{
    // Pre-generate the mappings (needs two passes).
    for (int i = 0; i < pitchTrack->num_frames(); ++i) {
        (void) calculateWindowLengthAtPitchTrackIndex(i);
        (void) calculateWindowSpacingAtPitchTrackIndex(i);
    }
    for (int i = 0; i < pitchTrack->num_frames(); ++i) {
        (void) calculateWindowLengthAtPitchTrackIndex(i);
        (void) calculateWindowSpacingAtPitchTrackIndex(i);
    }
}

void vc::model::LpcTrack::fillFromAudio(const std::vector<double> &audio)
{
    const int length = static_cast<int>(audio.size());

    int start = 0;
    int winLen = windowLengthAtTime(start);
    int winSpc = windowSpacingAtTime(start);
    
    while (start + winLen < length) {
        std::vector<double> window(winLen);
        for (int j = 0; j < winLen; ++j)
            window[j] = audio[start + j];
        
        pushAudio(window);

        start += winSpc;
        winLen = windowLengthAtTime(start);
        winSpc = windowSpacingAtTime(start);

        wxAppConsole::GetInstance()->Yield();
    }
}

void vc::model::LpcTrack::applyToAudio(std::vector<double>& audio) const
{
    const int length = static_cast<int>(audio.size());

    std::vector<double> output(length, 0.0);
    std::vector<double> weights(length, 0.0);

    int index = 0;
    int start = 0;
    int winLen = windowLengthAtTime(start);
    int winSpc = windowSpacingAtTime(start);
    
    while (start + winLen < length) {
        // Add a pre-ramp to the frame.
        const int rampLen = mTrack[index].size() + 1;
        std::vector<double> frame(rampLen + winLen);
        std::vector<double> window(winLen);
        for (int j = 0; j < winLen; ++j) {
            window[j] = 0.5 - 0.5 * std::cos((2.0 * PI * j) / double(winLen - 1));
            frame[rampLen + j] = window[j] * audio[start + j];
        }
        for (int l = 0; l < rampLen; ++l) {
            frame[l] = frame[rampLen] * (2.0 * double(l) / double(rampLen) - 1.0);
        }
        
        frame = Lpc::applyFilter(mTrack[index], frame);

        for (int j = 0; j < winLen; ++j) {
            output[start + j] += window[j] * frame[rampLen + j];
            weights[start + j] += window[j];
        }
        
        index++;
        start += winSpc;
        winLen = windowLengthAtTime(start);
        winSpc = windowSpacingAtTime(start);
        
        wxAppConsole::GetInstance()->Yield();
    }

    for (int i = 0; i < length; ++i) {
        output[i] /= weights[i];
    }

    std::swap(output, audio);
}

void vc::model::LpcTrack::pushCoefficients(const std::vector<double>& lpca)
{ 
    mTrack.push_back(lpca);
}

void vc::model::LpcTrack::pushAudio(const std::vector<double>& audio)
{
    pushCoefficients(Lpc::analyze(audio, mPreemphFrequency, mSampleRate));
}

void vc::model::LpcTrack::applyFrequencyShift(const double shiftFactor)
{
    for (auto& lpca : mTrack) {
        lpca = vc::model::Lpc::applyFrequencyShift(lpca, shiftFactor);
    }
}

int vc::model::LpcTrack::windowLengthAtTime(int index) const
{
    return windowLengthAtPitchTrackIndex(mPitchTrack->Index(float(index) / float(mSampleRate)));
}

int vc::model::LpcTrack::windowSpacingAtTime(int index) const
{
    // Constant spacing yields the best results.
    return (int) std::round(0.67 / 1000.0 * mSampleRate);
    // return windowSpacingAtPitchTrackIndex(mPitchTrack->Index(float(index) / float(mSampleRate)));
}

int vc::model::LpcTrack::windowLengthAtPitchTrackIndex(int index) const
{
    if (index < 0 || index >= mPitchTrack->num_frames())
        return (int) std::round(20.0 / 1000.0 * mSampleRate);
    return mWindowLengths[index];
}

int vc::model::LpcTrack::windowSpacingAtPitchTrackIndex(int index) const
{
    if (index < 0 || index >= mPitchTrack->num_frames())
        return (int) std::round(5.0 / 1000.0 * mSampleRate);
    return mWindowSpacings[index];
}

int vc::model::LpcTrack::calculateWindowLengthAtPitchTrackIndex(const int index, const int depth)
{
    const int defaultWindowLength = (int) std::round(20.0 / 1000.0 * mSampleRate);
    
    if (mWindowLengths[index] < 0) {
        if (!mPitchTrack->v(index)) {
            mWindowLengths[index] = 0;
        }
        else {
            const double pitchPeriod = 1.0 / mPitchTrack->a(index);
            mWindowLengths[index] = (int) std::round(6.0 * pitchPeriod * mSampleRate);
        }
    }
    else if (mWindowLengths[index] == 0) {
        // Closest valid value to the left.
        int leftInd = index - 1;
        while (leftInd >= 0 && mWindowLengths[leftInd--] == 0);
        const double leftVal = (leftInd < 0) ? defaultWindowLength : mWindowLengths[leftInd];
        
        // Closest valid value to the right.
        int rightInd = index + 1;
        while (rightInd < mPitchTrack->num_frames() && mWindowLengths[rightInd++] == 0);
        const double rightVal = (rightInd >= mPitchTrack->num_frames()) ? defaultWindowLength : mWindowLengths[rightInd];

        mWindowLengths[index] = (int) std::round(0.5 * (leftVal + rightVal));
    }

    return mWindowLengths[index];
}

int vc::model::LpcTrack::calculateWindowSpacingAtPitchTrackIndex(const int index, const int depth)
{
    const int defaultWindowSpacing = (int) std::round(5.0 / 1000.0 * mSampleRate);
    
    if (mWindowSpacings[index] < 0) {
        if (!mPitchTrack->v(index)) {
            mWindowSpacings[index] = 0;
        }
        else {
            const double pitchPeriod = 1.0 / mPitchTrack->a(index);
            mWindowSpacings[index] = (int) std::round(2.0 * pitchPeriod * mSampleRate);
        }
    }
    else if (mWindowSpacings[index] == 0) {
        // Closest valid value to the left.
        int leftInd = index - 1;
        while (leftInd >= 0 && mWindowSpacings[leftInd--] == 0);
        const double leftVal = (leftInd < 0) ? defaultWindowSpacing : mWindowSpacings[leftInd];
        
        // Closest valid value to the right.
        int rightInd = index + 1;
        while (rightInd < mPitchTrack->num_frames() && mWindowSpacings[rightInd++] == 0);
        const double rightVal = (rightInd >= mPitchTrack->num_frames()) ? defaultWindowSpacing : mWindowSpacings[rightInd];

        mWindowSpacings[index] = (int) std::round(0.5 * (leftVal + rightVal));
    }

    return mWindowSpacings[index];
}