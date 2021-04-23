#include "VoiceChangerController.h"

vc::controller::Controller::Controller()
{
}

vc::model::AudioTrack *vc::controller::Controller::audioTrack()
{
    return &mAudioTrack;
}

vc::synth::GlottalSource *vc::controller::Controller::glottalSource()
{
    return &mGlottalSource;
}