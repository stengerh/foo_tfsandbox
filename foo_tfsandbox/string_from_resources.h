#pragma once

class string_from_resources
{
public:
	string_from_resources(UINT nID);

	bool load_string(UINT nID);
	bool load_string(UINT nID, HINSTANCE hInstance);

	inline operator const char *() const {return m_data;}

private:
	pfc::stringcvt::string_utf8_from_os m_data;
};
