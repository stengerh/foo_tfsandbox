#pragma once

#include "titleformat_syntax.h"
#include "titleformat_analysis.h"
#include "titleformat_debug.h"

namespace ast
{
	class node_filter_impl_unknown_function : public node_filter
	{
	public:
		node_filter_impl_unknown_function(titleformat_debugger &p_debugger) : m_debugger(p_debugger) {}

		virtual bool test(block_expression *b) {return true;}
		virtual bool test(comment *n) {return true;}
		virtual bool test(string_constant *n) {return true;}
		virtual bool test(field_reference *n) {return true;}
		virtual bool test(condition_expression *n) {return true;}

		virtual bool test(call_expression *n)
		{
			return !m_debugger.is_function_unknown(n);
		}

	private:
		titleformat_debugger &m_debugger;
	};
 
}
