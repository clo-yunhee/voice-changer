#include "VoiceChangerApp.h"
#include "AboutDialog.h"
#include <wx/fs_inet.h>
#include <wx/sstream.h>
#include <wx/txtstrm.h>
#include <sstream>
#include <iomanip>
#include <ctime>
#include "Noise.h"

wxIMPLEMENT_APP(vc::gui::App);

bool vc::gui::App::OnInit()
{
    wxFileSystem::AddHandler(new wxInternetFSHandler);
    wxImage::AddHandler(new wxPNGHandler);

    Frame *frame = new Frame(this);
    frame->Show(true);
    return true;
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
    menuFile->Append(wxID_SAVE);
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);

    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT);
    
    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&File");
    menuBar->Append(menuHelp, "&Help");

    SetMenuBar(menuBar);

    Bind(wxEVT_MENU, &Frame::OnOpen, this, wxID_OPEN);
    Bind(wxEVT_MENU, &Frame::OnSave, this, wxID_SAVE);
    Bind(wxEVT_MENU, &Frame::OnAbout, this, wxID_ABOUT);
    Bind(wxEVT_MENU, &Frame::OnExit, this, wxID_EXIT);

    Bind(wxEVT_BUTTON, &Frame::OnResynth, this, ID_Resynth);

    wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topSizer);

    mTrackDetails = new wxRichTextCtrl(this, ID_TrackDetails, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxRE_READONLY | wxRE_MULTILINE | wxTE_CENTRE);
    mTrackDetails->SetBackgroundColour(*wxLIGHT_GREY);
    topSizer->Add(mTrackDetails, wxSizerFlags(1).Expand().Border(wxALL, 10));

    mLogs = new wxRichTextCtrl(this, ID_Logs, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxRE_READONLY | wxRE_MULTILINE);
    mLogs->SetBackgroundColour(*wxLIGHT_GREY);
    topSizer->Add(mLogs, wxSizerFlags(2).Expand().Border(wxALL, 15));

    mResynth = new wxButton(this, ID_Resynth, "Resynthesize");
    topSizer->Add(mResynth, wxSizerFlags(0).Align(wxALIGN_CENTER).Border(wxALL, 10));

    UpdateTrackDetails();
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
    Log("Task started.", true);

    const double lpcPreemphFrequency = 100.0;
    const double postHpFrequency = 350.0;

    const double relativeNoiseGain = 0.2;

    // Extract pitch and formant information.
    Log("Pitch analysis.");
    auto pitchTrack = mApp->controller()->audioTrack()->toPitchTrack();

    Log("LPC analysis.");
    auto lpcTrack = mApp->controller()->audioTrack()->toLpcTrack(pitchTrack, lpcPreemphFrequency);

    const int sampleRate = mApp->controller()->audioTrack()->sampleRate();
    const double duration = mApp->controller()->audioTrack()->duration();
    const int frameCount = std::round(duration * sampleRate);

    const int voicingWindowLength = std::round(200.0 / 1000.0 * sampleRate);
    const int voicingWindowSpacing = std::round(60.0 / 1000.0 * sampleRate);

    mApp->controller()->glottalSource()->setSampleRate(sampleRate);

    const double minRd = 0.4;
    const double minRdPitch = 70;

    const double maxRd = 2.1;
    const double maxRdPitch = 250;

    mApp->controller()->glottalSource()->setRd(0.8);

    std::vector<double> audio(frameCount);

    // Generate glottal source.
    Log("Generating glottal source.");
    for (int i = 0; i < frameCount; ++i) {
        double time = double(i) / double(sampleRate);

        int pitchTrackIndex = pitchTrack->Index(time);
        bool voicing = pitchTrack->v(pitchTrackIndex);

        if (voicing) {
            double pitch = pitchTrack->a(pitchTrackIndex);
            mApp->controller()->glottalSource()->setPitch(pitch, 0.1);

            double Rd;
            if (pitch < minRdPitch)
                Rd = minRd;
            else if (pitch > maxRdPitch)
                Rd = maxRd;
            else
                Rd = minRd + (maxRd - minRd) * (pitch - minRdPitch) / (maxRdPitch - minRdPitch);
            mApp->controller()->glottalSource()->setRd(Rd, 0.2);
        }
        mApp->controller()->glottalSource()->setVoicing(voicing);

        audio[i] = mApp->controller()->glottalSource()->generateFrame();
    }

    // Add noise to the input.
    Log("Adding noise.");
    auto noise = vc::model::Noise::tilted(frameCount, -3.0);

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

    // Apply LPC filters track.
    Log("LPC filtering.");
    lpcTrack.applyToAudio(audio);

    // Apply voicing contour with Hann window.
    Log("Post-processing.");
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

    // Add a first-order high-pass filter.
    const double hpCoef = std::exp(-(2.0 * PI * postHpFrequency) / sampleRate);
    for (int i = frameCount - 1; i >= 1; --i)
        audio[i] -= hpCoef * audio[i - 1];

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

    Log("Task finished.", true);

    mLogs->Newline();

    delete pitchTrack;
}

void vc::gui::Frame::UpdateTrackDetails()
{
    mTrackDetails->Clear();

    mTrackDetails->BeginAlignment(wxTEXT_ALIGNMENT_CENTER);

    mTrackDetails->BeginBold();
    mTrackDetails->WriteText("Duration: ");
    mTrackDetails->EndBold();
    mTrackDetails->WriteText(std::to_string(mApp->controller()->audioTrack()->duration()));
    mTrackDetails->WriteText(" s");
    mTrackDetails->Newline();

    mTrackDetails->BeginBold();
    mTrackDetails->WriteText("Sample rate: ");
    mTrackDetails->EndBold();
    mTrackDetails->WriteText(std::to_string(mApp->controller()->audioTrack()->sampleRate()));
    mTrackDetails->WriteText(" Hz");
    mTrackDetails->Newline();

    mTrackDetails->EndAlignment();
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