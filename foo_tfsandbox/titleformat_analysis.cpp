#include "stdafx.h"
#include "titleformat_analysis.h"

void titleformat_analysis::reset()
{
	m_unknown_function_map.remove_all();
	m_track = 0;
}

void titleformat_analysis::init(metadb_handle_ptr track, ast::block_expression *root)
{
	reset();
	m_track = track;
}


bool titleformat_analysis::is_function_unknown(pfc::string name, t_size param_count)
{
	bool is_new = false;
	function_info &info = m_unknown_function_map.find_or_add_ex(function_signature(name, param_count), is_new);
	if (is_new)
	{
		pfc::string_formatter format;
		format << name.get_ptr() << "(";
		if (param_count > 0)
		{
			format << " ";
			for (t_size index = 1; index < param_count; ++index) {format << ", ";}
		}
		format << ")";

		titleformat_object::ptr script;
		const bool script_valid = static_api_ptr_t<titleformat_compiler>()->compile(script, format);
		if (script.is_valid() && m_track.is_valid())
		{
			// TODO use proper context for script evaluation
			pfc::string_formatter buffer;
			m_track->format_title(0, buffer, script, 0);
			info.unknown = strcmp(buffer, "[UNKNOWN FUNCTION]") == 0;
		}
		else
		{
			info.unknown = true;
		}
	}
	return info.unknown;
}
