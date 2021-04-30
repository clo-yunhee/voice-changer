#ifndef VOICECHANGERAPP_H
#define VOICECHANGERAPP_H

#include "wx/event.h"
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/richtext/richtextctrl.h>

#include "VoiceChangerController.h"
#include "AudioTrackPlayer.h"

namespace vc::gui {

class App : public wxApp
{
public:
    bool OnInit() override;
    bool OnExceptionInMainLoop() override;

    vc::controller::Controller *controller();

private:
    vc::controller::Controller mController;
};

class Frame : public wxFrame, public wxTimer
{
public:
    Frame(App *app);

private:
    void OnOpen(wxCommandEvent& event);
    void OnSave(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnResynth(wxCommandEvent& event);
    void OnPlayPause(wxCommandEvent& event);
    void OnSeek(wxCommandEvent& event);
    void OnFormantShiftChange(wxCommandEvent& event);
    void OnPitchShiftChange(wxCommandEvent& event);
    void OnRdChange(wxCommandEvent& event);

    void Notify() override;

    void UpdateTrackDetails();
    void UpdatePlayerControls();

    void Log(const std::string& text, bool bold = false);

    App *mApp;

    wxRichTextCtrl *mTrackDetails;
    wxRichTextCtrl *mLogs;
    wxButton *mResynth;
    wxButton *mPlayPause;
    wxSlider *mSeeker;
    wxStaticText *mSeekerPosition;
    wxSlider *mFormantShift;
    wxStaticText *mFormantShiftLabel;
    wxSlider *mPitchShift;
    wxStaticText *mPitchShiftLabel;
    wxSlider *mRd;
    wxStaticText *mRdLabel;

    vc::model::AudioTrackPlayer mTrackPlayer;
};

enum
{
    ID_TrackDetails = 1,
    ID_Logs,
    ID_Resynth,
    ID_PlayPause,
    ID_Seeker,
    ID_SeekerPosition,
    ID_FormantShift,
    ID_FormantShiftLabel,
    ID_PitchShift,
    ID_PitchShiftLabel,
    ID_Rd,
    ID_RdLabel,
};

}

#endif // VOICECHANGERAPP_H