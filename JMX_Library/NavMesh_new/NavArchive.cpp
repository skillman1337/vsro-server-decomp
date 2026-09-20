/**
 * ============================================================================
 * Joymax NavMesh - Resource file access
 * Replaces: GFXFileManager.dll (IFileManager) and CJArchiveFm (RTTI .?AVCJArchiveFm@@)
 *
 * Implements:
 *   - CNavFileManager (GFXFileManager folder mode replacement)
 *   - CNavArchive::Read       @ 0x009973E0 (CJArchiveFm::ReadBytes)
 *   - CNavArchive::ReadString @ 0x0099EDD0
 * ============================================================================
 */

#include "NavArchive.h"

#include <cstdio>
#include <cstring>

namespace NavMesh {

CNavFileManager::CNavFileManager() {
}

void CNavFileManager::SetRoot(const std::string& strRoot) {
	m_strRoot = strRoot;
	if (!m_strRoot.empty()) {
		const char chLast = m_strRoot[m_strRoot.size() - 1];
		if (chLast != '\\' && chLast != '/') {
			m_strRoot.push_back('\\');
		}
	}
}

bool CNavFileManager::LoadFile(const char* pszPath, std::vector<uint8_t>& vecOut) const {
	vecOut.clear();
	if (pszPath == nullptr) {
		return false;
	}

	const std::string strFull = m_strRoot + pszPath;
	FILE* pFile = std::fopen(strFull.c_str(), "rb");
	if (pFile == nullptr) {
		return false;
	}

	if (std::fseek(pFile, 0, SEEK_END) != 0) {
		std::fclose(pFile);
		return false;
	}
	const long nSize = std::ftell(pFile);
	if (nSize < 0 || std::fseek(pFile, 0, SEEK_SET) != 0) {
		std::fclose(pFile);
		return false;
	}

	vecOut.resize(static_cast<size_t>(nSize));
	const size_t nRead = (nSize > 0) ? std::fread(vecOut.data(), 1, vecOut.size(), pFile) : 0;
	std::fclose(pFile);
	if (nRead != vecOut.size()) {
		vecOut.clear();
		return false;
	}
	return true;
}

CNavArchive::CNavArchive()
	: m_nPos(0)
	, m_bOpen(false)
	, m_bFailed(false) {
}

bool CNavArchive::Open(const CNavFileManager* pFileManager, const char* pszPath) {
	m_nPos = 0;
	m_bFailed = false;
	m_bOpen = (pFileManager != nullptr) && pFileManager->LoadFile(pszPath, m_vecData);
	return m_bOpen;
}

/*
================
CNavArchive::Read
[RECONSTRUCTED - 0x009973E0]
================
*/
bool CNavArchive::Read(void* pDest, size_t nSize) {
	if (nSize == 0) {
		return !m_bFailed;
	}
	if (!m_bOpen || m_bFailed || m_nPos + nSize > m_vecData.size()) {
		m_bFailed = true;
		std::memset(pDest, 0, nSize);
		return false;
	}
	std::memcpy(pDest, m_vecData.data() + m_nPos, nSize);
	m_nPos += nSize;
	return true;
}

bool CNavArchive::Seek(size_t nOffset) {
	if (!m_bOpen || nOffset > m_vecData.size()) {
		m_bFailed = true;
		return false;
	}
	m_nPos = nOffset;
	return true;
}

/*
================
CNavArchive::ReadString
[RECONSTRUCTED - 0x0099EDD0]
================
*/
bool CNavArchive::ReadString(std::string& strOut) {
	const uint32_t dwLength = ReadValue<uint32_t>();
	strOut.clear();
	if (m_bFailed || dwLength > m_vecData.size() - m_nPos) {
		m_bFailed = true;
		return false;
	}
	strOut.assign(reinterpret_cast<const char*>(m_vecData.data() + m_nPos), dwLength);
	m_nPos += dwLength;
	return true;
}

} // namespace NavMesh
