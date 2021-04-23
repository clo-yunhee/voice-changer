#include "AboutDialog.h"
#include "img/Patreon.png.h"
#include <wx/sstream.h>
#include <wx/txtstrm.h>
#include <wx/html/htmlwin.h>
#include <wx/hyperlink.h>

#define PATREON_URL "https://patreon.com/cloyunhee"

vc::gui::AboutDialog::AboutDialog(wxWindow *parent)
    : wxDialog(parent, wxID_ANY, "About VoiceChanger")
{
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(vbox);

    wxBitmap patreonBitmap = wxBITMAP_PNG_FROM_DATA(Patreon);

    wxStringOutputStream o;
    wxTextOutputStream aboutStr(o);
    aboutStr
        << "<html><head></head><body>"
        << "<center>"
        << "<h3>VoiceChanger</h3>"
        << "<p>An experiment with speech synthesis algorithms.</p>"
        << "<p>Made by Clo Yun-Hee Dufour.</p>"
        << "</center>"
        << "</body></html>";
    wxHtmlWindow *htmlWindow = new wxHtmlWindow(this);
    htmlWindow->SetPage(o.GetString());
    vbox->Add(htmlWindow, 1, wxGROW);

    wxStaticBitmap *patreonIcon = new wxStaticBitmap(this, wxID_ANY, patreonBitmap, wxDefaultPosition, wxSize(64, 64));
    patreonIcon->SetCursor(wxCursor(wxCURSOR_HAND));
    patreonIcon->Bind(wxEVT_LEFT_DOWN, [](auto& event) { ::wxLaunchDefaultBrowser(PATREON_URL); });
    vbox->Add(patreonIcon, 0, wxALIGN_CENTER);

    this->SetBackgroundColour(*wxWHITE);
}