#ifndef REAPER_H
#define REAPER_H

#include "core/track.h"

inline const char *reaperErr;

namespace REAPER {

bool Analyze(const std::vector<double>& audio, int sampleRate, Track **p_f0);

}

#endif // REAPER_H