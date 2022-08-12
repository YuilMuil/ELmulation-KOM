#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>	   // Needed this for filesystem stuff..
#include <omp.h>		   // OpenMP instead of jthreads
#include <zlib.h>		   // zlib include

#include "pugixml.hpp"	   // XML parser
#include "../Common/Cryptography/DecryptFunctions.h" // Decryption functions(algo0,algo2,algo3);

enum Algorithm
{
	//Komv3 algorithms
	Algorithm_0 = 0,
	Algorithm_2 = 2,
	Algorithm_3 = 3,

	//Komv4 algorithms
	Algorithmv2 = 10
};

class KOMv3Header
{
	//  27 bytes = magic header
	//	25 bytes of padding(00)
	//	4 bytes of file object entry count(so if theres 200 files, then it will represent 200)
	//	4 bytes(basically is the kom compressed, which it will always be true, so 1)
	//	4 bytes of filetime(basically CRC of first 60 bytes)
	//	4 bytes of CRC(xml header)
	//	4 bytes of XML header size(integer)
public:
	std::string MAGIC_HEADER{};
	int FileEntryCount{};
	int HeaderCRC{};
	int XMLSize{};
	std::stringstream XML{};

};

class KOMv3
{
public:
	int Algorithm = Algorithm_0;	   // Default is set to Algo0(in clients before 2014).
	std::string Filename = "";		   // Name of the file.
	std::string Checksum = "";		   // CRC32 checksum.
	int CompressedSize = 0;				       // Compressed file size. Offset value to keep track of where the binary files are located at when appending.
	int DecompressedSize = 0;		   // Original file size.
	std::string FileBuffer = "";	       // File buffer.
	std::string FileTime = "00000000"; // No clue what this is, but doesn't seem to matter..?(seems in newer versions, they just put the same value as checksum lol)

	//Things that will not change in value.
	inline static const std::string MAGIC_HEADER = "KOG GC TEAM MASSFILE V.0.3."; // Magic header

};

//KOMv4 structure. Will not be implementing this until I need it for my emulator.
#ifdef KOMv4_IMPLEMENT
class KOMv4 : KOMv3
{
public:
	std::string MappedID;			   // Unique key for each file.

	//Things that will not change in value.
	inline static const std::string MAGIC_HEADER = "KOG GC TEAM MASSFILE V.0.4."; // Magic header

private:
	inline static const unsigned char MappedIDList[256] = {0}; // MappedID list.

};
#endif //KOMv4_IMPLEMENT

