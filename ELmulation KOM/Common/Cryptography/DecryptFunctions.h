#pragma once

#include <locale>
#include <stdio.h>
#include <assert.h>
#include "Keys.h"
#include "CRC_32.h"

#define MAX_PATH 260

typedef unsigned short USHORT;
typedef unsigned char BYTE;
typedef unsigned int UINT;
typedef unsigned long ULONG;

/*

							----Credits----
Reverse engineered Algorithm 2/3 together with Raitou[https://github.com/Raitou].
Credits to ESEmu Team as I learned algo0 structure from their KOM Manager.

*/

__forceinline std::string XorAlgo3(const char* pEncryptedBuffer, int iBufferSize, const wchar_t* pwszFilename)
{
	if (pEncryptedBuffer == NULL || iBufferSize <= 0 || pwszFilename == NULL || pwszFilename[0] == NULL)
		return std::string();
	const static int	s_aiXorKeys[5] = { XOR_KEY_NA0, XOR_KEY_NA1, XOR_KEY_NA2, XOR_KEY_NA3, XOR_KEY_NA4 };
	const int iKeySize = 5 * sizeof(int);
	unsigned char    abyXORKey1[iKeySize];
	memcpy(abyXORKey1, s_aiXorKeys, iKeySize);
	/* For testing (Since ida doesnt show this kind of code 'as far as my understanding goes')
	for (int i = 0; i < 5; ++i)
		((int*)&abyXORKey1[0])[i] ^= XOR_KEY49;
	*/
	unsigned long   dwCRC = 0xFFFFFFFFU;
	int iByteIndex = 0;
	{
		int iFileNameLength = wcslen(pwszFilename);
		iFileNameLength = __min(MAX_PATH, iFileNameLength);
		int iPrefixSize = iFileNameLength * 2 + 4;
		BYTE    abyByte[MAX_PATH * 2 + 4];
		std::locale loc;
		for (int i = 0; i < iFileNameLength; i++)
		{
			USHORT usValue = (USHORT)std::tolower(pwszFilename[i], loc);
			abyByte[iByteIndex++] = (BYTE)(usValue & 0xFF);
			usValue >>= 8;
			if (usValue != 0)
			{
				abyByte[iByteIndex++] = (BYTE)(usValue & 0xFF);
			}
		}
		unsigned long   dwFileSize = (unsigned long)iBufferSize;
		for (int i = 0; i < 4 && dwFileSize != 0; ++i)
		{
			abyByte[iByteIndex++] = (BYTE)(dwFileSize & 0xFF);
			dwFileSize >>= 8;
		}
		assert(iPrefixSize >= iByteIndex);
		CRC_32::GetInstance().CalculateAndNotDecrypt(abyByte, (UINT)iByteIndex, abyXORKey1, iKeySize, dwCRC);
	}
	iByteIndex %= iKeySize;
	BYTE abyXORKey[iKeySize];
	if (iByteIndex == 0)
	{
		memcpy(abyXORKey, abyXORKey1, iKeySize);
	}
	else
	{
		memcpy(&abyXORKey[0], abyXORKey1 + iByteIndex, iKeySize - iByteIndex);
		memcpy(&abyXORKey[iKeySize - iByteIndex], abyXORKey1, iByteIndex);
	}

	CRC_32::GetInstance().CalculateAndDecrypt((BYTE*)pEncryptedBuffer, (UINT)iBufferSize, abyXORKey, iKeySize, dwCRC);

	return pEncryptedBuffer;
}

__forceinline void  XorAlgo2(char* pDecryptedBuffer, const char* pEncryptedBuffer, int iBufferSize, const wchar_t* pwszFilename)
{
	if (pDecryptedBuffer == NULL || pEncryptedBuffer == NULL || iBufferSize <= 0 || pwszFilename == NULL || pwszFilename[0] == NULL)
		return;
	const static int	s_aiXorKeys[5] = { XOR_KEY30, XOR_KEY42, XOR_KEY14, XOR_KEY22, XOR_KEY40 };
	const int iKeySize = 5 * sizeof(int);
	unsigned char    abyXORKey1[iKeySize];
	memcpy(abyXORKey1, s_aiXorKeys, iKeySize);
	for (int i = 0; i < 5; ++i)
		((int*)&abyXORKey1[0])[i] ^= XOR_KEY49;

	unsigned long   dwCRC = 0xFFFFFFFFU;
	int iByteIndex = 0;
	{
		int iFileNameLength = wcslen(pwszFilename);
		iFileNameLength = __min(MAX_PATH, iFileNameLength);
		int iPrefixSize = iFileNameLength * 2 + 4;
		BYTE    abyByte[MAX_PATH * 2 + 4];
		std::locale loc;
		for (int i = 0; i < iFileNameLength; i++)
		{
			USHORT usValue = (USHORT)std::tolower(pwszFilename[i], loc);
			abyByte[iByteIndex++] = (BYTE)(usValue & 0xFF);
			usValue >>= 8;
			if (usValue != 0)
			{
				abyByte[iByteIndex++] = (BYTE)(usValue & 0xFF);
			}
		}
		unsigned long   dwFileSize = (unsigned long)iBufferSize;
		for (int i = 0; i < 4 && dwFileSize != 0; ++i)
		{
			abyByte[iByteIndex++] = (BYTE)(dwFileSize & 0xFF);
			dwFileSize >>= 8;
		}
		assert(iPrefixSize >= iByteIndex);
		CRC_32::GetInstance().CalculateWithoutEncrypt(abyByte, (UINT)iByteIndex, abyXORKey1, iKeySize, dwCRC);
	}
	iByteIndex %= iKeySize;
	BYTE abyXORKey[iKeySize];
	if (iByteIndex == 0)
	{
		memcpy(abyXORKey, abyXORKey1, iKeySize);
	}
	else
	{
		memcpy(&abyXORKey[0], abyXORKey1 + iByteIndex, iKeySize - iByteIndex);
		memcpy(&abyXORKey[iKeySize - iByteIndex], abyXORKey1, iByteIndex);
	}
	if (pDecryptedBuffer != pEncryptedBuffer)
		memcpy(pDecryptedBuffer, pEncryptedBuffer, iBufferSize);
	CRC_32::GetInstance().CalculateAndDecrypt((BYTE*)pDecryptedBuffer, (UINT)iBufferSize, abyXORKey, iKeySize, dwCRC);
}

__forceinline std::string XorAlgo0(const char* pEncryptedBuffer, int iBufferSize)
{
	int	aDecryptionKey[5] = { XOR_KEY_KR0, XOR_KEY_KR1, XOR_KEY_KR2, XOR_KEY_KR3, XOR_KEY_KR4 };

	char pBuf[5];

	std::string strCryptBuffer;

	if (pEncryptedBuffer == NULL)
		return std::string();

	int iKeyIndex = 0;
	int iKeySize = sizeof(aDecryptionKey) / sizeof(int);
	int iRemainSize = iBufferSize;
	int iCryptedSize = 0;

	while (iRemainSize > 0)
	{
		memset(pBuf, 0, 5);

		int iBufTemp;
		if (iRemainSize >= 4)
			memcpy(&iBufTemp, pEncryptedBuffer + iCryptedSize, 4);
		else
			memcpy(&iBufTemp, pEncryptedBuffer + iCryptedSize, iRemainSize);

		int iEncryptData = iBufTemp ^ aDecryptionKey[iKeyIndex] ^ 0x38D2DFA4;

		memcpy(pBuf, &iEncryptData, 4);

		if (iRemainSize < 4)
		{
			strCryptBuffer.append(pBuf, iRemainSize);
			break;
		}
		else
		{
			strCryptBuffer.append(pBuf, 4);
		}

		iRemainSize -= 4;

		iCryptedSize += 4;

		++iKeyIndex;

		if (iKeyIndex >= iKeySize)
			iKeyIndex = 0;

	}

	return strCryptBuffer;
}

__forceinline char* __xor(const char*_input, int iBufferSize)
{
	char _key[12] = { 0x02, 0xAA, 0xF8, 0xC6, 0xDC, 0xAB, 0x47, 0x26, 0xEF, 0xBB, 0x00, 0x98 };
	std::string _output;
	_output.resize(iBufferSize);

	for (auto i = 0; i < iBufferSize; i++)
		_output[i] = _input[i] ^ _key[i % 12];

	char* pDecryptedBuffer = new char[_output.size() + 1];
	memset(pDecryptedBuffer, 0, _output.size() + 1);
	memcpy(pDecryptedBuffer, _output.c_str(), _output.size());
	return pDecryptedBuffer;
}