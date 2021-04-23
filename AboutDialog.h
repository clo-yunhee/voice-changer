#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

namespace vc::gui {

class AboutDialog : public wxDialog {
public:
    AboutDialog(wxWindow *parent);
};

}

#endif // ABOUTDIALOG_H