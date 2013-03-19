#pragma once

#include "titleformat_syntax.h"

class titleformat_analysis
{
public:
	void reset();
	void init(metadb_handle_ptr track, ast::block_expression *root);

	bool is_function_unknown(pfc::string function_name, t_size param_count);

private:
	struct function_signature
	{
		pfc::string name;
		t_size param_count;

		function_signature() : param_count(0) {}
		function_signature(pfc::string _name, t_size _param_count) : name(_name), param_count(_param_count) {}
		function_signature(const function_signature &source) : name(source.name), param_count(source.param_count) {}
		function_signature &operator=(const function_signature &source) {name = source.name; param_count = source.param_count;}

		static int g_compare(const function_signature & p_item1, const function_signature & p_item2) {int rv = pfc::stricmp_ascii(p_item1.name.ptr(),p_item2.name.ptr()); if (rv == 0) rv = pfc::compare_t(p_item1.param_count, p_item2.param_count); return rv;}
		bool operator==(const function_signature& p_other) const {return g_compare(*this,p_other) == 0;}
		bool operator!=(const function_signature& p_other) const {return g_compare(*this,p_other) != 0;}
		bool operator<(const function_signature& p_other) const {return g_compare(*this,p_other) < 0;}
		bool operator>(const function_signature& p_other) const {return g_compare(*this,p_other) > 0;}
		bool operator<=(const function_signature& p_other) const {return g_compare(*this,p_other) <= 0;}
		bool operator>=(const function_signature& p_other) const {return g_compare(*this,p_other) >= 0;}
	};

	struct function_info
	{
		bool unknown;

		function_info() : unknown(false) {}
	};

	pfc::map_t<function_signature, function_info> m_unknown_function_map;

	metadb_handle_ptr m_track;
};
