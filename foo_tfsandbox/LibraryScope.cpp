#include "stdafx.h"
#include "LibraryScope.h"

CLibraryScope::CLibraryScope() : m_hDll(NULL)
{
}

CLibraryScope::CLibraryScope(LPCTSTR pszName) : m_hDll(NULL)
{
	LoadLibrary(pszName);
}

bool CLibraryScope::LoadLibrary(LPCTSTR pszName)
{
	ATLASSERT(m_hDll == NULL);

	m_hDll = ::LoadLibrary(pszName);

	return m_hDll != NULL;
}

CLibraryScope::~CLibraryScope()
{
	if (m_hDll != NULL)
	{
		::FreeLibrary(m_hDll);
		m_hDll = NULL;
	}
}
