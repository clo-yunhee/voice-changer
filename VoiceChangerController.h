#ifndef VOICECHANGERCONTROLLER_H
#define VOICECHANGERCONTROLLER_H

#include "AudioTrack.h"
#include "GlottalSource.h"

namespace vc::controller {

class Controller {
public:
    Controller();

    vc::model::AudioTrack *audioTrack();
    vc::synth::GlottalSource *glottalSource();

private:
    vc::model::AudioTrack mAudioTrack;
    vc::synth::GlottalSource mGlottalSource;
};

}

#endif // VOICECHANGERCONTROLLER_H