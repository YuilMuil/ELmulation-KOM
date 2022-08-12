#pragma once
#include "../Common/KOM.h" // KOM structure + includes needed for File I/O
#include "pugixml.hpp"     // XML parser
#include "../Common/Cryptography/CRC_32.h"
#include <thread>		   // For multi-threading luac

class KOMCreator //ONLY supporting algo0 for now since I only need algo0 for my purposes!!!
{
public:
	static std::string compress_string(const std::string& str, int compressionlevel = Z_DEFAULT_COMPRESSION);

	static void ProcessDirectory(std::string FullFilePath, std::string FilePath, std::string Filename);
	static bool CreateXML(std::vector<KOMv3>& FileVec, std::ofstream& FileBuffer);
	static bool ParseKOMObj(std::vector<KOMv3>& FileVec, std::string& FullFilePath);
	static bool WriteHeader(std::ofstream& FileBuffer, KOMv3Header& KOMHeaderBuffer );
	static void WriteBinaryToKOM(std::vector<KOMv3>& FileVec, std::ofstream& FileBuffer);
	static bool LuaCompile(std::string& FullFilePath);
};


/*
* Make a KOMv3 object for each child node in a parent directory.
* Make iterator loop for KOM(parent node(FilesList) -> Child node(Files) -> Grandchild node(KOMv3 entire structure))
* 



*/