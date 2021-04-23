#ifndef GLOTTALSOURCE_H
#define GLOTTALSOURCE_H

#include "GlottalModel.h"

namespace vc::synth {

class GlottalSource {
public:
    GlottalSource();

    void setSampleRate(int sampleRate);

    void setRd(double Rd, double rubber = 0);
    void setPitch(double pitch, double rubber = 0);
    void setVoicing(bool voicing);

    double generateFrame();

private:
    int mSampleRate;

    double mRdTarget;
    double mRdTargetVelocity;

    double mPitchTarget;
    double mPitchTargetVelocity;

    bool mVoicingTarget;

    // Actual.
    bool mFirst;
    double mRd;
    double mPitch;
    double mVoicingMultiplier;

    vc::model::LFGenerator lfGenerator;
};

}

#endif // GLOTTALSOURCE_H