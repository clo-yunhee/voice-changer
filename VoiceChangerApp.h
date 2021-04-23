#ifndef VOICECHANGERAPP_H
#define VOICECHANGERAPP_H

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "VoiceChangerController.h"

namespace vc::gui {

class App : public wxApp
{
public:
    bool OnInit() override;

    vc::controller::Controller *controller();

private:
    vc::controller::Controller mController;
};

class Frame : public wxFrame
{
public:
    Frame(App *app);

private:
    void OnOpen(wxCommandEvent& event);
    void OnSave(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnResynth(wxCommandEvent& event);

    void UpdateTrackDetails();

    App *mApp;

    wxTextCtrl *mTrackDetails;
    wxTextCtrl *mLogs;
    wxButton *mResynth;
};

enum
{
    ID_TrackDetails = 1,
    ID_Logs,
    ID_Resynth,
};

}

#endif // VOICECHANGERAPP_H