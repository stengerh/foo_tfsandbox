#include "stdafx.h"
#include "string_from_resources.h"

string_from_resources::string_from_resources(UINT nID)
{
	load_string(nID);
}

bool string_from_resources::load_string(UINT nID)
{
	return load_string(nID, core_api::get_my_instance());
}

bool string_from_resources::load_string(UINT nID, HINSTANCE hInstance)
{
	pfc::array_hybrid_t<TCHAR, 256> buffer;
	buffer.set_size(256);
	int length;
	do {
		length = ::LoadString(hInstance, nID, &buffer[0], buffer.get_size());
		if (length == 0) return false;
	} while (length == buffer.get_size() - 1);
	m_data.convert(&buffer[0]);
	return true;
}
