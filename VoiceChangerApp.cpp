#include "VoiceChangerApp.h"
#include "FilterDesign.h"
#include "AboutDialog.h"
#include "Noise.h"
#include "Exception.h"
#include "wx/sizer.h"
#include <wx/event.h>
#include <wx/fs_inet.h>
#include <wx/sstream.h>
#include <wx/txtstrm.h>
#include <wx/string.h>
#include <sstream>
#include <iomanip>
#include <ctime>

wxIMPLEMENT_APP(vc::gui::App);

bool vc::gui::App::OnInit()
{
    wxFileSystem::AddHandler(new wxInternetFSHandler);
    wxImage::AddHandler(new wxPNGHandler);

    Frame *frame = new Frame(this);
    frame->Show(true);
    return true;
}

bool vc::gui::App::OnExceptionInMainLoop()
{
    try {
        throw;
    }
    catch (const vc::Exception& e) {
        wxMessageBox(e.what(), "Exception caught.", wxOK | wxICON_ERROR);
        return true;
    }
    catch (...) {
        goto unexpectedError;
    }

unexpectedError:
    wxMessageBox("Unexpected error occurred, the program will terminate.", "Unexpected error.", wxOK | wxICON_ERROR);
    return false;
}

vc::controller::Controller *vc::gui::App::controller()
{
    return &mController;
}

vc::gui::Frame::Frame(App *app)
    : wxFrame(nullptr, wxID_ANY, "VoiceChanger"),
      mApp(app)
{
    wxMenu *menuFile = new wxMenu;
    menuFile->Append(wxID_OPEN);
    menuFile->Append(wxID_SAVEAS);
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);

    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT);
    
    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&File");
    menuBar->Append(menuHelp, "&Help");

    SetMenuBar(menuBar);

    wxFrame::Bind(wxEVT_MENU, &Frame::OnOpen, this, wxID_OPEN);
    wxFrame::Bind(wxEVT_MENU, &Frame::OnSave, this, wxID_SAVEAS);
    wxFrame::Bind(wxEVT_MENU, &Frame::OnAbout, this, wxID_ABOUT);
    wxFrame::Bind(wxEVT_MENU, &Frame::OnExit, this, wxID_EXIT);

    wxFrame::Bind(wxEVT_BUTTON, &Frame::OnResynth, this, ID_Resynth);
    wxFrame::Bind(wxEVT_BUTTON, &Frame::OnPlayPause, this, ID_PlayPause);
    wxFrame::Bind(wxEVT_SLIDER, &Frame::OnSeek, this, ID_Seeker);
    wxFrame::Bind(wxEVT_SLIDER, &Frame::OnFormantShiftChange, this, ID_FormantShift);
    wxFrame::Bind(wxEVT_SLIDER, &Frame::OnPitchShiftChange, this, ID_PitchShift);
    wxFrame::Bind(wxEVT_SLIDER, &Frame::OnRdChange, this, ID_Rd);

    wxBoxSizer *topSizer = new wxBoxSizer(wxHORIZONTAL);
    SetSizer(topSizer);

    wxBoxSizer *leftSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(leftSizer, wxSizerFlags(0).Expand().Border(wxALL, 10));

    mTrackDetails = new wxRichTextCtrl(this, ID_TrackDetails, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxRE_READONLY | wxRE_MULTILINE | wxTE_CENTRE);
    mTrackDetails->SetBackgroundColour(*wxLIGHT_GREY);
    leftSizer->Add(mTrackDetails, wxSizerFlags(1).Expand().Border(wxTOP | wxLEFT | wxRIGHT, 10));

    wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);
    leftSizer->Add(hbox, wxSizerFlags(0).Align(wxEXPAND).Border(wxTOP | wxLEFT | wxRIGHT, 10));

    mPlayPause = new wxButton(this, ID_PlayPause, "");
    hbox->Add(mPlayPause, wxSizerFlags(0).Border(wxLEFT | wxRIGHT, 10));

    mSeeker = new wxSlider(this, ID_Seeker, 0, 0, 100);
    hbox->Add(mSeeker, wxSizerFlags(1).Border(wxRIGHT, 10));

    mSeekerPosition = new wxStaticText(this, ID_SeekerPosition, "");
    mSeekerPosition->SetForegroundColour(*wxWHITE);
    hbox->Add(mSeekerPosition, wxSizerFlags(0).Align(wxALIGN_CENTER).Border(wxRIGHT, 10));

    mResynth = new wxButton(this, ID_Resynth, "Resynthesize");
    leftSizer->Add(mResynth, wxSizerFlags(0).Align(wxALIGN_CENTER).Border(wxTOP | wxLEFT | wxRIGHT, 10));

    mLogs = new wxRichTextCtrl(this, ID_Logs, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxRE_READONLY | wxRE_MULTILINE);
    mLogs->SetBackgroundColour(*wxLIGHT_GREY);
    leftSizer->Add(mLogs, wxSizerFlags(2).Expand().Border(wxALL, 15));

    wxBoxSizer *rightSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(rightSizer, wxSizerFlags(1).Expand().Border(wxALL, 10));

    wxBoxSizer *hbox4 = new wxBoxSizer(wxHORIZONTAL);
    rightSizer->Add(hbox4, wxSizerFlags(0).Align(wxEXPAND).Border(wxTOP | wxLEFT | wxRIGHT, 10));

    wxStaticText *formantShiftTitle = new wxStaticText(this, wxID_ANY, "Formant shift: ");
    formantShiftTitle->SetForegroundColour(*wxWHITE);
    hbox4->Add(formantShiftTitle, wxSizerFlags(0).Align(wxALIGN_CENTER).Border(wxRIGHT, 10));

    mFormantShift = new wxSlider(this, ID_FormantShift, 100, 10, 300);
    hbox4->Add(mFormantShift, wxSizerFlags(1).Border(wxRIGHT, 10));

    mFormantShiftLabel = new wxStaticText(this, ID_PitchShiftLabel, "100%");
    mFormantShiftLabel->SetForegroundColour(*wxWHITE);
    hbox4->Add(mFormantShiftLabel, wxSizerFlags(0).Align(wxALIGN_CENTER).Border(wxRIGHT, 10));

    wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
    rightSizer->Add(hbox2, wxSizerFlags(0).Align(wxEXPAND).Border(wxTOP | wxLEFT | wxRIGHT, 10));

    wxStaticText *pitchShiftTitle = new wxStaticText(this, wxID_ANY, "Pitch shift: ");
    pitchShiftTitle->SetForegroundColour(*wxWHITE);
    hbox2->Add(pitchShiftTitle, wxSizerFlags(0).Align(wxALIGN_CENTER).Border(wxRIGHT, 10));

    mPitchShift = new wxSlider(this, ID_PitchShift, 100, 10, 300);
    hbox2->Add(mPitchShift, wxSizerFlags(1).Border(wxRIGHT, 10));

    mPitchShiftLabel = new wxStaticText(this, ID_PitchShiftLabel, "100%");
    mPitchShiftLabel->SetForegroundColour(*wxWHITE);
    hbox2->Add(mPitchShiftLabel, wxSizerFlags(0).Align(wxALIGN_CENTER).Border(wxRIGHT, 10));

    wxBoxSizer *hbox3 = new wxBoxSizer(wxHORIZONTAL);
    rightSizer->Add(hbox3, wxSizerFlags(0).Align(wxEXPAND).Border(wxTOP | wxLEFT | wxRIGHT, 10));

    wxStaticText *rdTitle = new wxStaticText(this, wxID_ANY, "Rd (thickness / spectral tilt): ");
    rdTitle->SetForegroundColour(*wxWHITE);
    hbox3->Add(rdTitle, wxSizerFlags(0).Align(wxALIGN_CENTER).Border(wxRIGHT, 10));

    mRd = new wxSlider(this, ID_Rd, 170, 40, 280);
    hbox3->Add(mRd, wxSizerFlags(1).Border(wxRIGHT, 10));

    mRdLabel = new wxStaticText(this, ID_RdLabel, "1.7");
    mRdLabel->SetForegroundColour(*wxWHITE);
    hbox3->Add(mRdLabel, wxSizerFlags(0).Align(wxALIGN_CENTER).Border(wxRIGHT, 10));

    SetBackgroundColour(wxColour(48, 48, 48, 255));
    UpdateTrackDetails();
    SetSize(640, 360);

    wxTimer::Start(50);
}

void vc::gui::Frame::OnExit(wxCommandEvent& event)
{
    Close(true);
}

void vc::gui::Frame::OnAbout(wxCommandEvent& event)
{
    AboutDialog aboutDialog(this);
    aboutDialog.ShowModal();
}

void vc::gui::Frame::OnOpen(wxCommandEvent& event)
{
    wxFileDialog fileDialog(
        this, "Choose an audio file to open", wxEmptyString, wxEmptyString,
        "Audio files (*.wav, *.mp3, *.flac)|*.wav;*.mp3;*.flac", wxFD_OPEN);

    if (fileDialog.ShowModal() == wxID_OK)
    {
        if (!mApp->controller()->audioTrack()->loadFromFile(fileDialog.GetPath().ToStdString())) {
            wxMessageBox("Could not open audio file", "Error opening audio file", wxOK | wxICON_ERROR, this);
        }
        else {
            UpdateTrackDetails();
        }
    }
}

void vc::gui::Frame::OnSave(wxCommandEvent& event)
{
    wxFileDialog fileDialog(
        this, "Choose where to save the audio file", wxEmptyString, wxEmptyString,
        "WAV file (*.wav)|*.wav", wxFD_SAVE);
        
    if (fileDialog.ShowModal() == wxID_OK)
    {
        if (!mApp->controller()->audioTrack()->saveToWavFile(fileDialog.GetPath().ToStdString())) {
            wxMessageBox("Could not save audio file", "Error saving audio file", wxOK | wxICON_ERROR, this);
        }
    }
}

void vc::gui::Frame::OnResynth(wxCommandEvent& event)
{
    mResynth->Enable(false);

    Log("Task started.", true);

    const double lpcPreemphFrequency = 100.0;

    const double relativeNoiseGain = 0.22;

    // Extract pitch and formant information.
    Log("Pitch analysis.");
    auto pitchTrack = mApp->controller()->audioTrack()->toPitchTrack();
    if (pitchTrack == nullptr) {
        Log("Task failed.", true);
        mLogs->Newline();
        mResynth->Enable(true);
        throw vc::Exception("Pitch analysis failed.");
    }

    Log("LPC analysis.");
    auto lpcTrack = mApp->controller()->audioTrack()->toLpcTrack(pitchTrack, lpcPreemphFrequency);

    // Morph pitch track.
    for (int i = 0; i < pitchTrack->num_frames(); ++i) {
        const double pitch = pitchTrack->a(i);
        pitchTrack->a(i) = (mPitchShift->GetValue() / 100.0) * pitch;      
    }

    // Morph lpc track.
    lpcTrack.applyFrequencyShift(mFormantShift->GetValue() / 100.0);

    const int sampleRate = mApp->controller()->audioTrack()->sampleRate();
    const double duration = mApp->controller()->audioTrack()->duration();
    const int frameCount = std::round(duration * sampleRate);

    const int voicingWindowLength = std::round(200.0 / 1000.0 * sampleRate);
    const int voicingWindowSpacing = std::round(60.0 / 1000.0 * sampleRate);

    mApp->controller()->glottalSource()->setSampleRate(sampleRate);

    mApp->controller()->glottalSource()->setRd(mRd->GetValue() / 100.0);

    std::vector<double> audio(frameCount);

    // Generate glottal source.
    Log("Generating glottal source.");
    for (int i = 0; i < frameCount; ++i) {
        double time = double(i) / double(sampleRate);

        int pitchTrackIndexBelow = pitchTrack->IndexBelow(time);
        int pitchTrackIndexAbove = pitchTrack->IndexAbove(time);
        double timeBelow = pitchTrack->t(pitchTrackIndexBelow);
        double timeAbove = pitchTrack->t(pitchTrackIndexAbove);

        // timeBelow <= time <= timeAbove
        int pitchTrackIndexClosest = ((time - timeBelow) > (timeAbove - time)) ? pitchTrackIndexBelow : pitchTrackIndexAbove;

        bool voicing = pitchTrack->v(pitchTrackIndexClosest);

        if (voicing) {
            double pitch = (time - timeBelow) / (timeAbove - timeBelow) * pitchTrack->a(pitchTrackIndexBelow)
                            + (timeAbove - time) / (timeAbove - timeBelow) * pitchTrack->a(pitchTrackIndexAbove);

            mApp->controller()->glottalSource()->setPitch(pitch, 0.005);
        }
        mApp->controller()->glottalSource()->setVoicing(voicing);

        audio[i] = mApp->controller()->glottalSource()->generateFrame();
    }

    // Low-pass filter for glottal source.
    auto lpsos = vc::model::Butterworth::lowPass(6, sampleRate / 2.0 - 1000.0, sampleRate);
    audio = vc::model::sosfilter(lpsos, audio);

    // High-pass filter for glottal source.
    auto hpsos = vc::model::Butterworth::highPass(4, 400.0, sampleRate);
    audio = vc::model::sosfilter(hpsos, audio);

    // Add noise to the input.
    Log("Adding noise.");
    auto noise = vc::model::Noise::tilted(frameCount, -1.0);
    auto hp2sos = vc::model::Butterworth::highPass(2, 3500.0, sampleRate);
    noise = vc::model::sosfilter(hp2sos, noise);

    // Low pass filter for noise.
    noise = vc::model::sosfilter(lpsos, noise);

    double audioAmplitude = 0.0;
    double noiseAmplitude = 0.0;
    for (int i = 0; i < frameCount; ++i) {
        if (std::abs(audio[i]) > audioAmplitude)
            audioAmplitude = std::abs(audio[i]);
        if (std::abs(noise[i]) > noiseAmplitude)
            noiseAmplitude = std::abs(noise[i]);
    }

    for (int i = 0; i < frameCount; ++i) {
        audio[i] += relativeNoiseGain * audioAmplitude * noise[i] / noiseAmplitude;
    }

    // Apply voicing contour with Hann window.
    Log("Applying voicing contour.");
    std::vector<double> voicingWindow(voicingWindowLength);
    std::vector<double> voicingWindow2(voicingWindowLength);
    for (int j = 0; j < voicingWindowLength; ++j) {
        voicingWindow[j] = 0.5 - 0.5 * std::cos((2.0 * PI * double(j)) / voicingWindowLength);

        const double x = (double(j) - double(voicingWindowLength) / 2) / (double(voicingWindowLength) / 2);
        voicingWindow2[j] = 1.0 - x * x;
    }

    std::vector<double> weights(frameCount);

    for (int iq = 0; iq < frameCount / voicingWindowSpacing; ++iq) {
        const int i = iq * voicingWindowSpacing;

        double sum = 0.0;
        double quot = 0.0;

        for (int j = 0; j < voicingWindowLength; ++j) {
            const int index = i + j - voicingWindowLength / 2;
            const double time = double(index) / double(sampleRate);
            const double pitchTrackIndex = pitchTrack->Index(time);
            const double sample = pitchTrack->v(pitchTrackIndex) ? 1.0 : 0.0;
            sum += voicingWindow[j] * sample;
            quot += voicingWindow[j];
        }

        for (int j = 0; j < voicingWindowLength; ++j) {
            const int index = i + j - voicingWindowLength / 2;
            if (index >= 0 && index < frameCount) {
                weights[index] += voicingWindow2[j] * sum / quot;
            }
        }
    }

    for (int i = 0; i < frameCount; ++i) {
        audio[i] *= weights[i];
    }

    // Apply LPC filters track.
    Log("LPC filtering.");
    lpcTrack.applyToAudio(audio);
    std::replace_if(audio.begin(), audio.end(), [](double x) { return !std::isnormal(x); }, 0.0);

    // Normalize.
    double normAmplitude = 0.0;
    for (int i = 0; i < frameCount; ++i) {
        if (std::abs(audio[i]) > normAmplitude) {
            normAmplitude = std::abs(audio[i]);
        }
    }
    if (normAmplitude > 0.0) {
        for (int i = 0; i < frameCount; ++i) {
            audio[i] /= normAmplitude;
        }
    }
    
    mApp->controller()->audioTrack()->replace(audio, sampleRate);
    mTrackPlayer.resetForTrack(mApp->controller()->audioTrack());

    Log("Task finished.", true);

    mLogs->Newline();

    delete pitchTrack;

    mResynth->Enable(true);
}

void vc::gui::Frame::OnPlayPause(wxCommandEvent& event)
{
    mTrackPlayer.pause(mTrackPlayer.isPlaying());
    UpdatePlayerControls();
}

void vc::gui::Frame::OnSeek(wxCommandEvent& event)
{
    double time = double(event.GetInt()) / 100.0;
    mTrackPlayer.seek(time);
    UpdatePlayerControls();
}

void vc::gui::Frame::OnFormantShiftChange(wxCommandEvent& event)
{
    mFormantShiftLabel->SetLabel(std::to_string(event.GetInt()) + "%");
}

void vc::gui::Frame::OnPitchShiftChange(wxCommandEvent& event)
{
    mPitchShiftLabel->SetLabel(std::to_string(event.GetInt()) + "%");
}

void vc::gui::Frame::OnRdChange(wxCommandEvent& event)
{
    std::string label;
    label += std::to_string(event.GetInt() / 100);
    label += ".";
    label += std::to_string(event.GetInt() % 100);

    mRdLabel->SetLabel(label);
}

void vc::gui::Frame::Notify()
{
    UpdatePlayerControls();
}

void vc::gui::Frame::UpdateTrackDetails()
{
    mTrackDetails->Clear();

    mTrackDetails->BeginAlignment(wxTEXT_ALIGNMENT_CENTER);

    mTrackDetails->BeginBold();
    mTrackDetails->WriteText("Duration: ");
    mTrackDetails->EndBold();

    std::stringstream durationStr;
    durationStr << std::fixed << std::setprecision(2) << mApp->controller()->audioTrack()->duration();

    mTrackDetails->WriteText(durationStr.str());
    mTrackDetails->WriteText(" s");
    mTrackDetails->Newline();

    mTrackDetails->BeginBold();
    mTrackDetails->WriteText("Sample rate: ");
    mTrackDetails->EndBold();
    mTrackDetails->WriteText(std::to_string(mApp->controller()->audioTrack()->sampleRate()));
    mTrackDetails->WriteText(" Hz");
    mTrackDetails->Newline();

    mTrackDetails->EndAlignment();

    mTrackPlayer.resetForTrack(mApp->controller()->audioTrack());
    UpdatePlayerControls();
}

void vc::gui::Frame::UpdatePlayerControls()
{
    if (mTrackPlayer.isPlaying()) {
        mPlayPause->SetLabel(wxString::FromUTF8("⏹"));
    }
    else {
        mPlayPause->SetLabel(wxString::FromUTF8("▶"));
    }

    std::stringstream ss;
    double trackDuration = mTrackPlayer.duration();
    double trackPosition = std::clamp(mTrackPlayer.position(), 0.0, trackDuration);
    int trackDurationRounded = (int) std::ceil(trackDuration);
    int trackPositionRounded = (int) std::ceil(trackPosition);
    if (trackDurationRounded >= 3600) {
        ss << std::setfill('0') << std::setw(2) << (trackPositionRounded / 3600);
        ss << ":";
    }
    ss << std::setfill('0') << std::setw(2) << (trackPositionRounded % 3600) / 60;
    ss << ":";
    ss << std::setfill('0') << std::setw(2) << (trackPositionRounded % 3600) % 60;
    ss << " / ";
    if (trackDurationRounded >= 3600) {
        ss << std::setfill('0') << std::setw(2) << (trackDurationRounded / 3600);
        ss << ":";
    }
    ss << std::setfill('0') << std::setw(2) << (trackDurationRounded % 3600) / 60;
    ss << ":";
    ss << std::setfill('0') << std::setw(2) << (trackDurationRounded % 3600) % 60;
    mSeekerPosition->SetLabel(ss.str());

    mSeeker->SetRange(0, trackDuration * 100);
    mSeeker->SetValue(trackPosition * 100);

    if (mTrackPlayer.position() > mTrackPlayer.duration() && mTrackPlayer.isPlaying()) {
        mTrackPlayer.pause(true);
    }

    mSeekerPosition->Update();
    mSeeker->Update();
}

void vc::gui::Frame::Log(const std::string& text, bool bold)
{
    time_t now = time(nullptr);
    struct tm *ltm = localtime(&now);

    std::stringstream ss;
    ss << "[";
    ss << std::setfill('0') << std::setw(2) << ltm->tm_hour;
    ss << ":";
    ss << std::setfill('0') << std::setw(2) << ltm->tm_min;
    ss << ":";
    ss << std::setfill('0') << std::setw(2) << ltm->tm_sec;
    ss << "] ";

    mLogs->SetInsertionPoint(mLogs->GetLastPosition());
    
    mLogs->BeginItalic();
    mLogs->WriteText(ss.str());
    mLogs->EndItalic();

    if (bold)
        mLogs->BeginBold();
    mLogs->WriteText(text);
    if (bold)
        mLogs->EndBold();

    mLogs->Newline();

    mLogs->ShowPosition(mLogs->GetLastPosition());

    wxAppConsole::GetInstance()->Yield();
}