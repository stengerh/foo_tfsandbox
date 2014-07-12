#include "stdafx.h"
#include "TitleformatSandboxDialog.h"
#include "string_from_resources.h"
#include "mainmenu_commands_impl_single.h"

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
