#pragma once

#include <vector>

#include "titleformat_syntax.h"
#include "titleformat_debug.h"

class inactive_range_walker
{
public:
	std::vector<Sci_CharacterRange> inactiveRanges;

	inactive_range_walker(titleformat_debugger &p_dbg) : dbg(p_dbg)
	{
		walk(p_dbg.get_root());
	}

private:
	titleformat_debugger &dbg;

	void walk(ast::node *n)
	{
		if (dbg.test_value(n))
		{
			t_size count = n->get_child_count();
			switch (n->kind())
			{
			case ast::node::kind_block:
				for (t_size index = 0; index < count; ++index)
				{
					walk(n->get_child(index));
				}
				break;

			case ast::node::kind_condition:
				walk(n->get_child(1));
				break;

			case ast::node::kind_call:
				ast::call_expression *call;
				call = static_cast<ast::call_expression *>(n);
				for (t_size index = 0; index < call->get_param_count(); ++index)
				{
					walk(call->get_param(index));
				}
				break;
			}
		}
		else if (n->kind() != ast::node::kind_comment)
		{
			ast::position pos = n->get_position();
			if (pos.start != -1)
			{
				Sci_CharacterRange range;
				range.cpMin = pos.start;
				range.cpMax = pos.end;
				inactiveRanges.push_back(range);
			}
		}
	}
};

