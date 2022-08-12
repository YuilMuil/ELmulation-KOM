#include "main.h"
#include "Common/KOM.h"

//Start Application(UI)
wxIMPLEMENT_APP(ELmulationKOM);

template<typename ObjectType>
bool UniversalErrorLog(ObjectType T, wxString objName)
{
	if (T == nullptr)
	{
		wxMessageBox("Error Object:" + objName + ", Please forward this to the developer!", "ERROR", wxOK | wxICON_INFORMATION);
		return false;
	}
	else
	{
		return true;
	}
}

bool ELmulationKOM::OnInit()
{
	//Create frame and set animated background for main frame.
	UIFrame* frame = new UIFrame();

	if (!UniversalErrorLog(frame, "frame"))
		return false;

	//Set icon for application
	frame->SetIcon(wxICON("./resource/icon.ico"));
	frame->Show();

	return true;
}

void UIFrame::InitBackground()
{
	this->AnimatedBG = new wxAnimationCtrl(this, wxID_ANY, wxAnimation(wxT("./resource/background.gif"), wxANIMATION_TYPE_ANY));
	
	if (!UniversalErrorLog(AnimatedBG, "AnimatedBG"))
		return;
	else
		AnimatedBG->Play();
}

void UIFrame::InitCreditButton()
{
	this->CreditButton = new wxBitmapButton(this->AnimatedBG, BTN_CREDITS, wxBitmap(wxT("./resource/credits.bmp"), wxBITMAP_TYPE_BMP),
		wxPoint(341,49), wxDefaultSize, wxBORDER_NONE);
}

void UIFrame::InitMenuBar()
{
	this->MenuBar = new wxStaticBitmap(this->AnimatedBG, CUSTOM_SYS_MENU, wxBitmap(wxT("./resource/system_menu.bmp"), wxBITMAP_TYPE_BMP), wxPoint(-20, 0));
}

void UIFrame::InitExitButton()
{
	this->ExitButton = new wxBitmapButton(this->MenuBar, BTN_EXIT_BUTTON, wxBitmap(wxT("./resource/Exit.bmp"), wxBITMAP_TYPE_BMP), wxPoint(485, 5), wxSize(18, 17), wxBORDER_NONE);
}

void UIFrame::InitMinimizeButton()
{
	this->MinimizeButton = new wxBitmapButton(this->MenuBar, BTN_MINIMIZE_BUTTON, wxBitmap(wxT("./resource/Minimize.bmp"), wxBITMAP_TYPE_BMP), wxPoint(460, 5), wxSize(18,17), wxBORDER_NONE);
}

void UIFrame::InitFileParserArea()
{
	this->AnimatedBG->DragAcceptFiles(true);
	this->AnimatedBG->Connect(wxEVT_DROP_FILES, wxDropFilesEventHandler(UIFrame::FileParserHandler), NULL, this);
}

void UIFrame::BindEventHandlers()
{
	this->CreditButton->Bind(wxEVT_BUTTON, [](wxCommandEvent& Event)
		{
			wxMessageBox("Developed by PSNKek. For Project El developers only! Do not redistribute.", "Credits", wxOK | wxICON_INFORMATION);
		});

	this->ExitButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent& Event)
		{
			this->Close();
		});

	this->MinimizeButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent& Event)
		{
			this->Iconize(true);
		});
}

//Constructor for UI
UIFrame::UIFrame() : wxFrame(NULL, wxID_ANY, "Project El - KOM Tool", wxPoint(300,300),
	wxSize(498, 270), wxFRAME_SHAPED | wxCLIP_CHILDREN)
{
	InitBackground();
	InitMenuBar();
	InitExitButton();
	InitMinimizeButton();
	InitCreditButton();
	InitFileParserArea();

	BindEventHandlers();
}