#pragma warning (disable: 4267)
#pragma warning (disable: 4244)

#include "reaper.h"
#include "core/track.h"
#include "epoch_tracker/epoch_tracker.h"
#include <dr_wav.h>

static Track* MakeEpochOutput(EpochTracker &et, float unvoiced_pm_interval) {
  std::vector<float> times;
  std::vector<int16_t> voicing;
  et.GetFilledEpochs(unvoiced_pm_interval, &times, &voicing);
  Track* pm_track = new Track;
  pm_track->resize(times.size());
  for (int32_t i = 0; i < times.size(); ++i) {
    pm_track->t(i) = times[i];
    pm_track->set_v(i, voicing[i]);
  }
  return pm_track;
}

static Track* MakeF0Output(EpochTracker &et, float resample_interval, Track** cor) {
  std::vector<float> f0;
  std::vector<float> corr;
  if (!et.ResampleAndReturnResults(resample_interval, &f0, &corr)) {
    return NULL;
  }

  Track* f0_track = new Track;
  Track* cor_track = new Track;
  f0_track->resize(f0.size());
  cor_track->resize(corr.size());
  for (int32_t i = 0; i < f0.size(); ++i) {
    float t = resample_interval * i;
    f0_track->t(i) = t;
    cor_track->t(i) = t;
    f0_track->set_v(i, (f0[i] > 0.0) ? true : false);
    cor_track->set_v(i, (f0[i] > 0.0) ? true : false);
    f0_track->a(i) = (f0[i] > 0.0) ? f0[i] : -1.0;
    cor_track->a(i) = corr[i];
  }
  *cor = cor_track;
  return f0_track;
}

static bool ComputeEpochsAndF0(EpochTracker &et, float unvoiced_pulse_interval,
			float external_frame_interval,
			Track** pm, Track** f0, Track** corr) {
  if (!et.ComputeFeatures()) {
    return false;
  }
  bool tr_result = et.TrackEpochs();
  et.WriteDiagnostics("");  // Try to save them here, even after tracking failure.
  if (!tr_result) {
    fprintf(stderr, "Problems in TrackEpochs");
    return false;
  }

  // create pm and f0 objects, these need to be freed in calling client.
  *pm = MakeEpochOutput(et, unvoiced_pulse_interval);
  *f0 = MakeF0Output(et, external_frame_interval, corr);
  return true;
}

bool REAPER::Analyze(const std::vector<double>& audio, int sampleRate, Track **p_f0)
{
    // Convert f64 audio to s16 audio.
    uint64_t sampleCount = audio.size();
    std::vector<int16_t> convAudio(sampleCount);
    drwav_f64_to_s16(convAudio.data(), audio.data(), sampleCount);

    bool do_hilbert_transform = kDoHilbertTransform;
    bool do_high_pass = kDoHighpass;
    float external_frame_interval = kExternalFrameInterval;
    float max_f0 = 700.0;
    float min_f0 = 40.0;
    float inter_pulse = kUnvoicedPulseInterval;
    float unvoiced_cost = kUnvoicedCost;

    EpochTracker et;
    et.set_unvoiced_cost(unvoiced_cost);
    if (!et.Init(convAudio.data(), sampleCount, sampleRate,
            min_f0, max_f0, do_high_pass, do_hilbert_transform)) {
        reaperErr = "Couldn't init EpochTracker";
        return false;
    }

    Track *f0 = nullptr;
    Track *pm = nullptr;
    Track *corr = nullptr;
    if (!ComputeEpochsAndF0(et, inter_pulse, external_frame_interval, &pm, &f0, &corr)) {
        reaperErr = "Failed to compute epochs";
        return false;
    }

    *p_f0 = f0;
    delete pm;
    delete corr;

    return true;
}