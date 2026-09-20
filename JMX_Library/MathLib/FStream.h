/**
 * ============================================================================
 * Joymax MathLib - File Stream Implementation
 * Original Source: D:\WORK2005\Source\JMX_Library\MathLib\FStream.h
 *
 * Implements Joymax binary file stream:
 *   - CFStream::CFStream @ 0x00976000
 *   - CFStream::~CFStream @ 0x00976060
 *   - CFStream::Close @ 0x00976070
 *   - CFStream::OpenFile @ 0x009762B0
 *   - CFStream::ReadBytes @ 0x009761C0
 *   - CFStream::Seek @ 0x00976390
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_MATHLIB_FSTREAM_H_
#define _JMX_LIBRARY_MATHLIB_FSTREAM_H_

#include <cstdint>
#include <cstdio>
#include <cstring>

class CFStream {
public:
	// Native @ 0x00976000
	CFStream();

	// Native @ 0x00976060
	~CFStream();

	// Native @ 0x009762B0
	bool OpenFile(const char* szFilePath);

	// Native @ 0x009761C0
	bool ReadBytes(uint32_t dwSize, void* pBuffer);

	// Native @ 0x00976390
	bool Seek(int32_t nOffset, int32_t nOrigin = SEEK_SET);

	// Native @ 0x00976070
	void Close();

	bool IsOpen() const { return m_fp != nullptr; }
	uint32_t GetFileSize() const { return m_dwFileSize; }
	const char* GetFileName() const { return m_szFileName; }

private:
	FILE*    m_fp;              // +0x04
	char     m_szFileName[260]; // +0x08 - +0x10B
	uint32_t m_dwStartOffset;   // +0x10C
	uint32_t m_dwEndOffset;     // +0x110
	uint32_t m_dwFileSize;      // +0x114
};

#endif // _JMX_LIBRARY_MATHLIB_FSTREAM_H_
