// foo_tfsandbox.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "TFDbgDialog.h"

DECLARE_COMPONENT_VERSION(
"Title Formatting Sandbox",
"1.0 alpha 2010-01-12",
"Copyright (C) 2008-2010 Holger Stenger");

class string_from_resources
{
public:
	string_from_resources(UINT nID)
	{
		load_string(nID);
	}

	bool load_string(UINT nID)
	{
		return load_string(nID, core_api::get_my_instance());
	}

	bool load_string(UINT nID, HINSTANCE hInstance)
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

	inline operator const char *() const {return m_data;}

private:
	pfc::stringcvt::string_utf8_from_os m_data;
};

class mainmenu_commands_impl_single : public mainmenu_commands
{
	//! Retrieves number of implemented commands. Index parameter of other methods must be in 0....command_count-1 range.
	virtual t_uint32 get_command_count() {return 1;}
	//! Retrieves GUID of specified command.
	virtual GUID get_command(t_uint32 p_index)
	{
		pfc::dynamic_assert(p_index == 0, "Command index out-of-range");
		return get_command();
	}
	//! Retrieves name of item, for list of commands to assign keyboard shortcuts to etc.
	virtual void get_name(t_uint32 p_index,pfc::string_base & p_out)
	{
		pfc::dynamic_assert(p_index == 0, "Command index out-of-range");
		get_name(p_out);
	}

	//! Retrieves item's description for statusbar etc.
	virtual bool get_description(t_uint32 p_index,pfc::string_base & p_out)
	{
		pfc::dynamic_assert(p_index == 0, "Command index out-of-range");
		return get_description(p_out);
	}

	//! Retrieves display string and display flags to use when menu is about to be displayed. If returns false, menu item won't be displayed. You can create keyboard-shortcut-only commands by always returning false from get_display().
	virtual bool get_display(t_uint32 p_index,pfc::string_base & p_text,t_uint32 & p_flags)
	{
		pfc::dynamic_assert(p_index == 0, "Command index out-of-range");
		return get_display(p_text, p_flags);
	}
	//! Executes the command. p_callback parameter is reserved for future use and should be ignored / set to null pointer.
	virtual void execute(t_uint32 p_index,service_ptr_t<service_base> p_callback)
	{
		pfc::dynamic_assert(p_index == 0, "Command index out-of-range");
		execute(p_callback);
	}

protected:
	virtual GUID get_command() = 0;
	virtual void get_name(pfc::string_base & p_out) = 0;
	virtual bool get_description(pfc::string_base & p_out) = 0;
	virtual GUID get_parent() = 0;
	virtual void execute(service_ptr_t<service_base> p_callback) = 0;
	virtual bool get_display(pfc::string_base & p_text,t_uint32 & p_flags) {p_flags = 0;get_name(p_text);return true;}
};

class mainmenu_commands_tfsandbox : public mainmenu_commands_impl_single
{
public:
	virtual GUID get_command()
	{
		// {819CD6A9-A492-45cd-9BCA-DFE3A6CF2BD9}
		static const GUID guid = { 0x819cd6a9, 0xa492, 0x45cd, { 0x9b, 0xca, 0xdf, 0xe3, 0xa6, 0xcf, 0x2b, 0xd9 } };
		return guid;
	}

	virtual void get_name(pfc::string_base & p_out)
	{
		p_out = string_from_resources(IDS_CMD_TFDBG_NAME);
	}

	virtual bool get_description(pfc::string_base & p_out)
	{
		p_out = string_from_resources(IDS_CMD_TFDBG_DESCRIPTION);
		return true;
	}

	virtual GUID get_parent()
	{
		return mainmenu_groups::view;
	}

	virtual void execute(service_ptr_t<service_base> p_callback)
	{
		CTitleFormatSandboxDialog::ActivateDialog();
	}
};

static service_factory_single_t<mainmenu_commands_tfsandbox> g_mainmenu_commands_tfsandbox_factory;
