#pragma once

class CLibraryScope
{
public:
	CLibraryScope();
	explicit CLibraryScope(LPCTSTR pszName);
	~CLibraryScope();

	bool LoadLibrary(LPCTSTR pszName);

	bool IsLoaded() const {return m_hDll != NULL;}

private:
	HMODULE m_hDll;
};

