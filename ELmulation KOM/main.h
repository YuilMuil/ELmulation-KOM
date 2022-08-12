#pragma once
#include <iostream>
#include <wx/wx.h>
#include <wx/animate.h> //wxAnimationCtrl call
#include <wx/custombgwin.h> // for custom backgrounds

template<typename ObjectType>
bool UniversalErrorLog(ObjectType T, wxString objName);

class ELmulationKOM : public wxApp
{
public:
	virtual bool OnInit(); //Initialize UIFrame and set the window application up.
};

class UIFrame : public wxFrame
{
public:
	//Constructor
	UIFrame();

private:

	//Animated Background.
	wxAnimationCtrl* AnimatedBG;

	//System Menu
	wxStaticBitmap* MenuBar;

	//Buttons
	wxBitmapButton* ExitButton;
	wxBitmapButton* MinimizeButton;
	wxBitmapButton* CreditButton;

	//Initializer functions
	void InitBackground();
	void InitCreditButton();
	void InitMenuBar();
	void InitExitButton();
	void InitMinimizeButton();
	void InitFileParserArea();

	//Bind event handlers to every button.
	void BindEventHandlers();
	
	//Event handler for drag and dropped files.
	void FileParserHandler(wxDropFilesEvent& Event);

	//Button enums
	enum BTN_ID
	{
		BTN_CREDITS = wxID_HIGHEST + 1, //wxButton* CreditButton
		BTN_EXIT_BUTTON,
		BTN_MINIMIZE_BUTTON,
		CUSTOM_SYS_MENU
		//Add button IDs to be used here.
	};

};