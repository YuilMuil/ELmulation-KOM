#include "KOMCreator.h"

/*
PLEASE READ THIS IF YOU ARE TRYING TO MAKE THIS CONCURRENT!!!
I did not do multi-threading for the KOMCreator class since
OpenMP does not support range based for loops(tis why I used
old style for loops in KOMExtractor), and I felt that the
gains were minimal compared to the development costs(aka my mental and time).
If OpenMP in VS ever supports 5.0+ or someone wants to do this
with some other thread-pooling library, then please do not use 
std::vector since it is not thread-safe.
Instead use concurrent_vector, which is thread-safe even if you
push_back to it at any time.

include for it is <concurrent_vector.h>
And making an concurrent_vector object is as simple as Concurrency::concurrent_vector<KOMv3> myObj;
I know there is mutex locking, but this is a much easier approach especially in C++20.
Have fun developing!

-YuilMuil
*/

// Credits to Timo Bingmann(https://panthema.net/2007/0328-ZLibString.html) for this code.
// I made my own implementation at first but this was much easier to read so I changed it -w-
std::string KOMCreator::compress_string(const std::string& str, int compressionlevel)
{
    z_stream zs;                        // z_stream is zlib's control structure
    memset(&zs, 0, sizeof(zs));

    if (deflateInit(&zs, compressionlevel) != Z_OK)
        throw(std::runtime_error("deflateInit failed while compressing."));

    zs.next_in = (Bytef*)str.data();
    zs.avail_in = str.size();           // set the z_stream's input

    int ret;
    char outbuffer[32768];
    std::string outstring;

    // retrieve the compressed bytes blockwise
    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);

        ret = deflate(&zs, Z_FINISH);

        if (outstring.size() < zs.total_out) {
            // append the block to the output string
            outstring.append(outbuffer,
                zs.total_out - outstring.size());
        }
    } while (ret == Z_OK);

    deflateEnd(&zs);

    if (ret != Z_STREAM_END) {          // an error occurred that was not EOF
        std::ostringstream oss;
        oss << "Exception during zlib compression: (" << ret << ") " << zs.msg;
        throw(std::runtime_error(oss.str()));
    }

    return outstring;
}

bool KOMCreator::LuaCompile(std::string& FullFilePath)
{
    if (!std::filesystem::exists("./luac.exe"))
        return false;

    /*
    Some credits to Raitou since I saved some time writing the multi-thread part of this
    by referencing his. although I used jthread instead of thread, since it automatically
    joins on thread destruction.
    */

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

    std::jthread* t = new std::jthread[omp_get_num_threads()];
    int fileIter = 0;
    //continue doing this over and over again until all files are done
    while (!(fileIter >= fileNames.size())) {

        int threadUsedCnt = 0;
        //creating new threads from the said number of threads u want it to be
        for (int i = 0; i < omp_get_num_threads(); i++) {
            t[i] = std::jthread([](std::wstring _FilePath)
                {
                    STARTUPINFO si;
                    PROCESS_INFORMATION pi;
                    ZeroMemory(&si, sizeof(si));
                    si.cb = sizeof(si);
                    ZeroMemory(&pi, sizeof(pi));

                    std::wstring command = (L"luac -o \"" + _FilePath + L"\" \"" + _FilePath + L"\"");

                    if (!CreateProcess(
                        NULL,   // lpApplicationName
                        command.data(), // lpCommandLine
                        NULL,   // lpProcessAttributes
                        NULL,   // lpThreadAttributes
                        FALSE,  // bInheritHandles
                        NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW,      // dwCreationFlags
                        NULL,   // lpEnvironment
                        NULL,   // lpCurrentDirectory
                        &si,    // lpStartupInfo
                        &pi     // lpProcessInformation
                    ))
                    {

                    }

                    WaitForSingleObject(pi.hProcess, INFINITE);
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }, fileNames.at(fileIter));
            threadUsedCnt++;
            fileIter++;

            //If files are less than the thread count obviously dont do new jobs to them
            if (fileIter >= fileNames.size())
                break;
        }
        
    }
        return true;
}

void KOMCreator::ProcessDirectory(std::string FullFilePath, std::string FilePath, std::string Filename)
{
    if (!LuaCompile(FullFilePath)) // Check if lua is compiled
        return;      // If failed then return

    std::ofstream FileBuffer{FullFilePath+".kom", std::ios::binary};

    std::vector<KOMv3> FileList;

    if (!ParseKOMObj(FileList, FullFilePath)) // Fill Filelist with all of the files information in the folder thats been sent
        return;                               // If fails, then stop the function execution to prevent any UB

    if (CreateXML(FileList, FileBuffer)) // If Header XML is successfully created and appended onto FileBuffer
        WriteBinaryToKOM(FileList, FileBuffer); // Then write the rest of the binary to FileBuffer
    else
        return;                          // If header fails then stop function from running. should add window popup later on...

}

bool KOMCreator::WriteHeader(std::ofstream& FileBuffer, KOMv3Header& KOMHeaderBuffer)
{

    //27 bytes = magic header
    //25 bytes of padding(00)
    // 4 bytes of file object entry count(so if theres 200 files, then it will represent 200)
    // 4 bytes(basically is the kom compressed, which it will always be true, so 1)
    // 4 bytes of filetime(basically CRC of first 60 bytes)
    // 4 bytes of CRC(xml header)
    // 4 bytes of XML header size(integer)

    if (FileBuffer.is_open())
    {
        FileBuffer.write(KOMv3::MAGIC_HEADER.data(), KOMv3::MAGIC_HEADER.size()); // Write magic header

        for (int i = 0; i < 25; i++)
            FileBuffer.write("\0", 1); // write 25 bytes of padding

        FileBuffer.write((char*)&KOMHeaderBuffer.FileEntryCount, 4); //write file count size
        FileBuffer.write("\1\0\0\0", 4); // Is it compressed or not? Just write as always true, since there are basically no times when its not true in els..
        FileBuffer.write("\1\3\3\7", 4); // Just hard code it since it doesnt even check it lol
        FileBuffer.write((char*)&KOMHeaderBuffer.HeaderCRC, 4);
        //FileBuffer.write(CRC_32::GetCRCFromBuffer(&KOMHeaderBuffer.XML, (UINT)KOMHeaderBuffer.XMLSize).data(), 4);
        FileBuffer.write((char*)&KOMHeaderBuffer.XMLSize, 4);

        FileBuffer << KOMHeaderBuffer.XML.rdbuf(); // write kom xml on to FileBuffer

        return true;
    }

    return false;
}

bool KOMCreator::CreateXML(std::vector<KOMv3>& FileVec, std::ofstream& FileBuffer)
{
 
    pugi::xml_document doc;

    // Generate XML declaration
    auto declarationNode = doc.append_child(pugi::node_declaration);
    declarationNode.append_attribute("version") = "1.0";

    auto AllFilesNode = doc.append_child("Files");

    for (auto& FileObj : FileVec)
    {
        pugi::xml_node childnode = AllFilesNode.append_child("File");
        childnode.append_attribute("Name").set_value(FileObj.Filename.data());
        childnode.append_attribute("Size").set_value(FileObj.DecompressedSize);
        childnode.append_attribute("CompressedSize").set_value(FileObj.CompressedSize);
        childnode.append_attribute("Checksum").set_value(FileObj.Checksum.data());
        childnode.append_attribute("FileTime").set_value(FileObj.FileTime.data());
        childnode.append_attribute("Algorithm").set_value(FileObj.Algorithm);
    }
    KOMv3Header KOMHeaderBuffer{}; // Temp buffer for getting length of XML size. not good practice since this copies, but its relatively cheap so its fine..
    doc.save(KOMHeaderBuffer.XML);            // save document to string stream(for getting XML size)

    KOMHeaderBuffer.XMLSize = KOMHeaderBuffer.XML.tellp(); // Get XML length
    KOMHeaderBuffer.FileEntryCount = FileVec.size(); // How many files there are in directory
    KOMHeaderBuffer.HeaderCRC = adler32(KOMHeaderBuffer.HeaderCRC,(Bytef*)&KOMHeaderBuffer.XML.str()[0], KOMHeaderBuffer.XMLSize); // Make CRC based on XML

    if (!WriteHeader(FileBuffer, KOMHeaderBuffer)) // Write KOM entry header + XML
        return false;

    return true;
}

bool KOMCreator::ParseKOMObj(std::vector<KOMv3>& FileVec, std::string& FullFilePath)
{
    // TO-DO make this multi-threaded with OpenMP
    for (auto& fileEntry : std::filesystem::recursive_directory_iterator(FullFilePath))
    {
        KOMv3 FileObject;

        std::ifstream FileNode(fileEntry.path(), std::ios::binary);

        FileObject.Algorithm = Algorithm::Algorithm_0;
        FileObject.Filename = fileEntry.path().filename().string();
        FileObject.DecompressedSize = fileEntry.file_size();

        FileObject.FileBuffer.resize(FileObject.DecompressedSize); // Resize file buffer size
        FileNode.read(FileObject.FileBuffer.data(), FileObject.DecompressedSize); // Get file buffer and load it onto File Object

        //Do Compression and encryption here. and then save current file buffer(after compression/encryption) size as compressedsize
        //ENCRYPTION IS ONLY DONE FOR TXT AND LUA in ALGO0
        if (fileEntry.path().extension() == ".lua" || fileEntry.path().extension() == ".Lua" || fileEntry.path().extension() == ".LUA" || fileEntry.path().extension() == ".txt")
            FileObject.FileBuffer = XorAlgo0(FileObject.FileBuffer.data(), FileObject.FileBuffer.size());

        FileObject.FileBuffer = KOMCreator::compress_string(FileObject.FileBuffer, Z_DEFAULT_COMPRESSION); // Compress the string

        FileObject.CompressedSize = FileObject.FileBuffer.size(); // Get compressed buffer size
        FileObject.Checksum = CRC_32::GetCRCFromString(FileObject.FileBuffer); // Get CRC32 value.
        FileObject.FileTime = FileObject.Checksum; // After doing analysis on newer files, this is what they seem to do.


        FileVec.push_back(FileObject); // Push file object back into vector of KOMv3(FileList) 
    }

    return true;
}

void KOMCreator::WriteBinaryToKOM(std::vector<KOMv3>& FileVec, std::ofstream& FileBuffer)
{
    for (auto& FileEntry : FileVec)                                              // Iterate through each file object in FileList vector
        FileBuffer.write(FileEntry.FileBuffer.data(), FileEntry.CompressedSize); // Write to file buffer

    return;                                                                      // Returns after it is finished. no point in having it but w/e
}

//Generate a vector of file lists, then move onto generating the XML header with pugixml
//*** use static std::string GetCRCFromFile(const char* szFilePath); -> do this for each loop cycle
//Probably will have to do performance tests, but I think the above will be faster(not sure)
//Then send the file stream to Encrypt(ALGO0) -> Compress with compress_string
//Finally append all of them together into one stream and then export it out.
//Not sure what to multi-thread here.. Probably not needed but I'll do some tests here and there
//Since this is a good learning experience :)