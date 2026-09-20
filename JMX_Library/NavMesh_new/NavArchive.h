/**
 * ============================================================================
 * Joymax NavMesh - Resource file access
 *
 * Replaces:
 *   - GFXFileManager.dll GFXDllCreateObject(2) folder file manager, rooted at "<path>data\" by
 *     CMapLoader_Initialize (0x009A984C .. 0x009A988F)
 *   - CJArchiveFm (vftable 0x00B43420): buffered reader over an IFileManager handle; ReadBytes 0x009973E0,
 *     refill 0x0099DD30, Close 0x009974F0
 *
 * The DLL is replaced by direct file access under a data root; the navmesh code reads through CNavArchive exactly
 * as it reads through CJArchiveFm. A read past the end of the file fails the archive (the native refill has no
 * more data to supply) and returns zeroed bytes.
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_NAVMESH_NEW_NAVARCHIVE_H_
#define _JMX_LIBRARY_NAVMESH_NEW_NAVARCHIVE_H_

#include <cstdint>
#include <string>
#include <vector>

namespace NavMesh {

/**
 * Folder file manager (GFXFileManager mode 2). Paths are relative to the data root and use '\' like the native
 * callers ("navmesh\nv_%04x.nvm", "res\bldg\...\x.bsr").
 */
class CNavFileManager {
public:
	CNavFileManager();

	void SetRoot(const std::string& strRoot);
	const std::string& GetRoot() const { return m_strRoot; }

	// Loads a whole file. Returns false when it cannot be opened (IFileManager::Open slot 10 returned 0).
	bool LoadFile(const char* pszPath, std::vector<uint8_t>& vecOut) const;

private:
	std::string m_strRoot;
};

/**
 * CJArchiveFm reader over an in-memory copy of the file.
 */
class CNavArchive {
public:
	CNavArchive();

	// CJArchiveFm::Open (slot 10 of IFileManager + archive init)
	bool Open(const CNavFileManager* pFileManager, const char* pszPath);

	bool IsOpen() const { return m_bOpen; }
	bool IsFailed() const { return m_bFailed; }
	size_t GetSize() const { return m_vecData.size(); }
	size_t GetPosition() const { return m_nPos; }

	// [RECONSTRUCTED - 0x009973E0] ReadBytes
	bool Read(void* pDest, size_t nSize);

	// IFileManager::Seek (slot 38, +0x98) with FILE_BEGIN
	bool Seek(size_t nOffset);

	template <typename T>
	T ReadValue() {
		T value{};
		Read(&value, sizeof(T));
		return value;
	}

	// [RECONSTRUCTED - 0x0099EDD0] u32 length followed by that many characters
	bool ReadString(std::string& strOut);

	// Whole file as text (the ifo/txt parsers tokenize the buffer, 0x009A80EB .. 0x009A8126)
	const std::vector<uint8_t>& GetData() const { return m_vecData; }

private:
	std::vector<uint8_t> m_vecData;
	size_t               m_nPos;
	bool                 m_bOpen;
	bool                 m_bFailed;
};

} // namespace NavMesh

#endif // _JMX_LIBRARY_NAVMESH_NEW_NAVARCHIVE_H_
