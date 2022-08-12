#include "FileProcessing.h"

void UIFrame::FileParserHandler(wxDropFilesEvent& Event)
{
	if (Event.GetNumberOfFiles() > 0)
	{
        wxASSERT(Event.GetFiles());

		wxBusyCursor busyCursor;
		wxWindowDisabler disabler;
		wxBusyInfo busyInfo(_("Parsing files right now! Please wait :)"));

        #pragma omp parallel for num_threads(omp_get_num_procs())
        for (int i = 0; i < Event.GetNumberOfFiles(); i++) 
        {
            wxFileName file = Event.GetFiles()[i];  //File name
            
            if (file.GetExt() == "kom") // Checks if its a kom file. then send that kom file directory path to Extractor class
            {
#ifdef _DEBUG
                wxMessageBox(file.GetFullPath() + "\n KOM FILE DETECTED. Will now extract the kom file! File name is: " + file.GetName());
#endif //_DEBUG
                
                KOMExtractor::ProcessKOM(file.GetFullPath().ToStdString(), file.GetPath().ToStdString(), file.GetName().ToStdString());
            }
            else if(wxDirExists(file.GetFullPath()))                        // If its given an directory path, then send the entire path to Creator class.
            {
#ifdef _DEBUG
                wxMessageBox(file.GetFullPath() + "\n Directory detected. Will create KOM based on directory. File name is: " + file.GetName());
#endif //_DEBUG
                KOMCreator::ProcessDirectory(file.GetFullPath().ToStdString(), file.GetPath().ToStdString(), file.GetName().ToStdString());
            }
#ifdef _DEBUG
            else
            {
                wxMessageBox("Filename: " + file.GetName() + "\nThis is not a kom nor a directory, therefore it will not be parsed!"); //Keep this for now.
            }
#endif //_DEBUG
        }

	}
}