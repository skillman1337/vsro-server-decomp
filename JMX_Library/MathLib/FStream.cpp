/**
 * ============================================================================
 * Joymax MathLib - File Stream Implementation
 * Original Source: D:\WORK2005\Source\JMX_Library\MathLib\FStream.cpp
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

#include "FStream.h"

/**
 * [RECONSTRUCTED - 0x00976000]
 * CFStream::CFStream
 */
CFStream::CFStream()
	: m_fp(nullptr)
	, m_dwStartOffset(0)
	, m_dwEndOffset(0)
	, m_dwFileSize(0) {
	std::memset(m_szFileName, 0, sizeof(m_szFileName));
}

/**
 * [RECONSTRUCTED - 0x00976060]
 * CFStream::~CFStream
 */
CFStream::~CFStream() {
	Close();
}

/**
 * [RECONSTRUCTED - 0x00976070]
 * CFStream::Close
 */
void CFStream::Close() {
	if (m_fp != nullptr) {
		std::fclose(m_fp);
		m_fp = nullptr;
	}
	m_dwStartOffset = 0;
	m_dwEndOffset = 0;
	m_dwFileSize = 0;
	std::memset(m_szFileName, 0, sizeof(m_szFileName));
}

/**
 * [RECONSTRUCTED - 0x009762B0]
 * CFStream::OpenFile
 */
bool CFStream::OpenFile(const char* szFilePath) {
	Close();
	if (szFilePath == nullptr || szFilePath[0] == '\0') {
		return false;
	}

	m_fp = std::fopen(szFilePath, "rb");
	if (m_fp == nullptr) {
		return false;
	}

#if defined(_MSC_VER)
	strncpy_s(m_szFileName, sizeof(m_szFileName), szFilePath, _TRUNCATE);
#else
	std::strncpy(m_szFileName, szFilePath, sizeof(m_szFileName) - 1);
	m_szFileName[sizeof(m_szFileName) - 1] = '\0';
#endif

	m_dwStartOffset = 0;
	std::fseek(m_fp, 0, SEEK_END);
	long nEnd = std::ftell(m_fp);
	if (nEnd < 0) {
		Close();
		return false;
	}
	m_dwEndOffset = static_cast<uint32_t>(nEnd);
	m_dwFileSize = m_dwEndOffset - m_dwStartOffset;

	std::fseek(m_fp, 0, SEEK_SET);
	return true;
}

/**
 * [RECONSTRUCTED - 0x009761C0]
 * CFStream::ReadBytes
 */
bool CFStream::ReadBytes(uint32_t dwSize, void* pBuffer) {
	if (m_fp == nullptr || pBuffer == nullptr || dwSize == 0) {
		return false;
	}
	size_t nRead = std::fread(pBuffer, 1, dwSize, m_fp);
	return (nRead == dwSize);
}

/**
 * [RECONSTRUCTED - 0x00976390]
 * CFStream::Seek
 */
bool CFStream::Seek(int32_t nOffset, int32_t nOrigin) {
	if (m_fp == nullptr) {
		return false;
	}
	return (std::fseek(m_fp, nOffset, nOrigin) == 0);
}
