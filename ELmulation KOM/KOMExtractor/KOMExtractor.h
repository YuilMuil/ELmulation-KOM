#pragma once
#include "../Common/KOM.h" // KOM structure
#include <thread>

class KOMExtractor //This is a utility class!
{
public:
	// File process functions
	static void ProcessKOM(std::string FullFileDir, std::string FileDir, std::string DirName);
	static bool ProcessXML(std::ifstream& FileBuffer, std::vector<KOMv3>& RefObj);
	static bool ProcessBuffer(KOMv3& FileObj);

	// Utility functions
	static uint32_t BufferToInt(std::string Buffer);
	static std::string ZlibDecompress(const std::string& str);
	static void ExportFile(std::string& DecompDirectory, KOMv3& FileObject);
	static void LuaDecompile(std::string& FullFilePath);

};


/*					&&&&&&&&&&&& HOW THE PARSING WILL BE DONE! DRAFTING &&&&&&&&&&&&
* 
* It will first get the File path from FileParserHandler and use that to get the file stream into memory(since we will not edit the actual KOM!!)
* We always keep that buffer as an std::string.
* Skip the file buffer by x amount of bytes and save the size of the xml part of the buffer and then skip the rest
* of that(there is a check to whether something is compressed or not along with more things, but it doesn't matter for our purposes).
* We will make an std::vector of KOMv3 objects, and then we will iterate each one of them with a for loop that fills out the information(via pugixml)
* 
* Now we will do another for loop through the std::vector of KOMv3 objects, except this will be done with std::async for each XML <File> child node.
* Process of how we will use std::async while iterating through KOMv3 objects.(explained in line 25)
* First, do a case switch statement that checks what algorithm it is. Then depending on that, call the DecryptFileBuffer or just do a break.
* Afterwards call the Decompress function and use the public member variable of KOMv3.FileBuffer(std::string) and store it there via reference since copying is expensive.
* Finally we will export that file via std::ofstream member functions.

*/