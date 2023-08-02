#include "KOMExtractor.h"
#include <future>

/*
Things that can be done.
Changing substr to boost::string_ref, but I didn't feel like adding Boost
just for very very minor performance gains.
Not recieving this many parameters in ProcessKOM, but this was alot easier
to implement with wxWidgets native library.

Other than that, I'm not sure what else can be done. Some might ask why I 
made an utility class instead of yknow.. using namespace or classes. 

tl;dr
It was easier to deal when using the standard library's asynchronous function, so if you
want to fix it, just fork it and change it yourself. Not like it'll make
any performance differences anyways ¯\_(-.-)_/¯

update:
the above no longer applies since I'm using openmp now, but I'll just leave it there
as a mark to show how many times I rewrote utility class(hint: its about 3 times).
Unfortunately VS doesn't support OpenMP 5.0, where range based for loops(or for_each)
can be parallel'd by #pragma magic, so please do understand..
*/

// Credits to Timo Bingmann(https://panthema.net/2007/0328-ZLibString.html) for this code.
// I made my own implementation at first but this was much easier to read so I changed it -w-
std::string KOMExtractor::ZlibDecompress(const std::string& str) {
	z_stream zs;                        // z_stream is zlib's control structure
	memset(&zs, 0, sizeof(zs));

	if (inflateInit(&zs) != Z_OK)
		throw(std::runtime_error("inflateInit failed while decompressing."));

	zs.next_in = (Bytef*)str.data();
	zs.avail_in = (unsigned int)str.size();

	int ret;
	char outbuffer[32768];
	std::string outstring;

	// get the decompressed bytes blockwise using repeated calls to inflate
	do {
		zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
		zs.avail_out = sizeof(outbuffer);

		ret = inflate(&zs, 0);

		if (outstring.size() < zs.total_out) {
			outstring.append(outbuffer,
				zs.total_out - outstring.size());
		}

	} while (ret == Z_OK);

	inflateEnd(&zs);

	if (ret != Z_STREAM_END) {          // an error occurred that was not EOF
		std::ostringstream oss;
		oss << "Exception during zlib decompression: (" << ret << ") "
			<< zs.msg;
		throw(std::runtime_error(oss.str()));
	}

	return outstring;
}

void KOMExtractor::LuaDecompile(std::string& FullFilePath)
{
	if (!std::filesystem::exists("./unluac.jar"))
		return;

	std::vector <std::wstring> fileNames;
	for (auto& file : std::filesystem::directory_iterator(FullFilePath))
	{
		auto _file = file.path().generic_wstring();
		auto _type = _file.substr(_file.length() - 3, _file.length());

		if (_type.compare(L"lua") == 0 || _type.compare(L"Lua") == 0 || _type.compare(L"LUA") == 0)
		{
			fileNames.push_back(_file);
		}
	}

	std::future<void>* t = new std::future<void>[omp_get_num_threads()];
	int fileIter = 0;
	//continue doing this over and over again until all files are done
	while (!(fileIter >= fileNames.size()))
	{

		int threadUsedCnt = 0;
		//creating new threads from the said number of threads u want it to be
		for (int i = 0; i < omp_get_num_threads(); i++)
		{
			t[i] = std::async(std::launch::async, [](std::wstring _FilePath)
				{

					STARTUPINFO si;
					PROCESS_INFORMATION pi;
					ZeroMemory(&si, sizeof(si));
					si.cb = sizeof(si);
					ZeroMemory(&pi, sizeof(pi));

					SECURITY_ATTRIBUTES sa;
					sa.nLength = sizeof(sa);
					sa.lpSecurityDescriptor = NULL;
					sa.bInheritHandle = TRUE;

					std::wstring FILESOMETHING = _FilePath + L"out";

					HANDLE h = CreateFile(FILESOMETHING.data(),
						GENERIC_WRITE,
						0,
						&sa,
						CREATE_NEW,
						FILE_ATTRIBUTE_NORMAL,
						NULL);

					si.hStdOutput = h;
					si.dwFlags |= STARTF_USESTDHANDLES;

					//std::wstring command = (L"java -jar unluac.jar \"" + _FilePath + L"\" >> \"" + _FilePath + L"out" + L"\"");
					std::wstring command = (L"java -Xmx2048m -jar unluac.jar \"" + _FilePath + L"\"");
					//std::wstring command = (L"java -jar -Xmx2048m unluac.jar -o \"" + _FilePath + L"\" \"" + _FilePath + L"\"");
					if (!CreateProcess(
						NULL,   // lpApplicationName
						command.data(), // lpCommandLine
						NULL,   // lpProcessAttributes
						NULL,   // lpThreadAttributes
						TRUE,  // bInheritHandles
						NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW,      // dwCreationFlags
						NULL,   // lpEnvironment
						NULL,   // lpCurrentDirectory
						&si,    // lpStartupInfo
						&pi     // lpProcessInformation
					))
					{

					}

					WaitForSingleObject(pi.hProcess, INFINITE);
					CloseHandle(si.hStdOutput);
					CloseHandle(si.hStdInput);
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);

					DeleteFile(_FilePath.data());

				}, fileNames.at(fileIter));
			threadUsedCnt++;
			fileIter++;

			//If files are less than the thread count obviously dont do new jobs to them
			if (fileIter >= fileNames.size())
				break;
		}

	}


	return;
}

void KOMExtractor::ProcessKOM(std::string FullFileDir, std::string FileDir, std::string DirName)
{
	std::ifstream FileStream(FullFileDir, std::ios::binary); //Let the destructor do its job and dont call close() lol

	KOMv3Header HeaderInfo;
	HeaderInfo.MAGIC_HEADER.resize(KOMv3::MAGIC_HEADER.size());
	
	FileStream.read(HeaderInfo.MAGIC_HEADER.data(), KOMv3::MAGIC_HEADER.size());

	if(HeaderInfo.MAGIC_HEADER != KOMv3::MAGIC_HEADER)
		return;		// Should probably.. add an notification, but for now just make it so that the function stops right there :3

	FileStream.ignore(41); // ignore padding and some other things that aren't used

	std::vector<KOMv3> ObjectList; // Load XML data into vectors of KOMv3 object

	if (!ProcessXML(FileStream, ObjectList)) // If XML can't be mapped to object, then we cannot parse anything.
		return;		// Should probably.. add an notification, but for now just make it so that the function stops right there :3

	std::string DecompDirectory = FileDir + "\\" + DirName;

	#pragma omp parallel for num_threads(omp_get_num_procs())
	for (int iter = 0; iter < ObjectList.size(); iter++)
	{
		if (ProcessBuffer(ObjectList[iter]))		// Process buffer(Decrypt->Decompress(Check if lua then also decompile))
			ExportFile(DecompDirectory, ObjectList[iter]);	// Export the file to the folder
	}
		
	////For now disable the lua decompilation
	//LuaDecompile(DecompDirectory);

	//for (auto& file : std::filesystem::directory_iterator(DecompDirectory))
	//{
	//	std::filesystem::path p = file;
	//	if (file.path().extension() == ".luaout")
	//	{
	//		p.replace_extension(L"lua");
	//	}
	//}

}

bool KOMExtractor::ProcessXML(std::ifstream& FileBuffer, std::vector<KOMv3>& RefObj)
{ 
	std::string TempXMLSizeBuffer{};
	TempXMLSizeBuffer.resize(4);

	FileBuffer.read(TempXMLSizeBuffer.data(), 4); // Get XML Size from FileBuffer(last 4 bytes before XML header) and then delete it

	uint32_t XMLSize{ BufferToInt(TempXMLSizeBuffer) }; // Set XML size

	std::string XMLData{};
	XMLData.resize(XMLSize);
	FileBuffer.read(XMLData.data(), XMLSize); // Load XML header from FileBuffer(also deletes it from main FileBuffer)

	pugi::xml_document doc;

	if (!doc.load_buffer(XMLData.data(), XMLData.length())) // If XML refuses to load, then return false.
		return false; 	

	pugi::xml_node FileList = doc.child("Files"); // Parent node in which we will traverse from. (Files->File1,File2,File3...)

	if (FileList.empty()) // If filelist is empty then there's no need to go further.
		return false;

	for (pugi::xml_node child = FileList.child("File"); child; child = child.next_sibling())
	{
		/*
		* Fun fact of the day!
		* Did you know that making an object every single time a for loop occurs
		* has no difference in both speed/memory usage etc performance?
		* I tried with roughly 100k objects in a small console program
		* with similar object structure/size and there were little to no
		* difference(less than 0.4% of difference). So, for readability I just made an object inside this for loop.
		* Am writing this just in case someone thinks they are being clever by making
		* an object outside of the for loop and just sending an copy of it via push_back
		* and then instead of recreating an object, just changing the public member variables
		* values on it.
		* tl;dr The compiler knows all!(Relevant: https://www.youtube.com/watch?v=Lu5SJcNp0J0)
		*/

		KOMv3 TempFileInformation;						// Let's make an temporary KOMv3 object here then push it into RefObj
												
		TempFileInformation.Filename = child.attribute("Name").as_string();
		TempFileInformation.Algorithm = child.attribute("Algorithm").as_int();
		TempFileInformation.Checksum = child.attribute("Checksum").as_string();	
		TempFileInformation.CompressedSize = child.attribute("CompressedSize").as_int();   //Offset to be used as the iterator end area when referencing the binary buffer
		TempFileInformation.DecompressedSize = child.attribute("Size").as_int();
		TempFileInformation.FileTime = child.attribute("FileTime").as_string();

		TempFileInformation.FileBuffer.resize(TempFileInformation.CompressedSize);
		FileBuffer.read(TempFileInformation.FileBuffer.data(), TempFileInformation.CompressedSize);

		RefObj.push_back(TempFileInformation);			// DO NOT USE EMPLACE_BACK AS THE OBJECT MADE HERE WILL BE DESTROYED UPON LOOP END!!!!
	}

	return true;
}

bool KOMExtractor::ProcessBuffer(KOMv3& FileObj)
{
	//This is indeed thread-safe. - Yuilmuil
	std::string FileExt{ FileObj.Filename.substr(FileObj.Filename.size() - 3, FileObj.Filename.size()) };
	std::wstring tempbuffer;
	std::string tempbuffer2;
	switch(FileObj.Algorithm)
	{
		case Algorithm::Algorithm_0:
			FileObj.FileBuffer = ZlibDecompress(FileObj.FileBuffer);
			if (FileExt == "txt" || FileExt == "lua" || FileExt == "Lua" || FileExt == "LUA")
				FileObj.FileBuffer = __xor(FileObj.FileBuffer.data(), FileObj.FileBuffer.size()); //decrypt function call
			break;
		case Algorithm::Algorithm_2:
			tempbuffer.assign(FileObj.Filename.begin(), FileObj.Filename.end());
			XorAlgo2(FileObj.FileBuffer.data(), FileObj.FileBuffer.data(), FileObj.CompressedSize, tempbuffer.data());
			FileObj.FileBuffer = ZlibDecompress(FileObj.FileBuffer);
			break;
		case Algorithm::Algorithm_3:
			//FileObj.FileBuffer = reinterpret_cast<char*>(decrypt_algorithm(FileObj.Filename, (unsigned char*)(FileObj.FileBuffer.data()), FileObj.CompressedSize));
			FileObj.FileBuffer = ZlibDecompress(FileObj.FileBuffer);
			break;

		default:
			return false;
	}

	return true;
}

void KOMExtractor::ExportFile(std::string& DecompDirectory, KOMv3& FileObject)
{
	if (!std::filesystem::exists(DecompDirectory))
	{
		std::filesystem::create_directory(DecompDirectory);
	}

	std::ofstream output{ DecompDirectory + "\\" + FileObject.Filename, std::ios::binary};
	output.write(&FileObject.FileBuffer[0], FileObject.FileBuffer.size());

	//put unluac call here!
}

uint32_t KOMExtractor::BufferToInt(std::string Buffer)
{
	//Reads buffer byte and converts the 4 bytes to int(little endian).
	//Can do this with an for loop but for now just leave it as it is, since we only need to get the XML header size in this..
	uint32_t result = uint32_t(static_cast<unsigned char>(Buffer[3]) << 24 |
		static_cast<unsigned char>(Buffer[2]) << 16 |
		static_cast<unsigned char>(Buffer[1]) << 8 |
		static_cast<unsigned char>(Buffer[0]));

	return result;
}

