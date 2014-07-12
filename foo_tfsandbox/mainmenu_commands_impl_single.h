#pragma once

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
