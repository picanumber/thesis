#include "bmk_utils.h"

namespace utl
{
	std::string nameof_param(rtree_param param)
	{
		switch (param)
		{
		case utl::rtree_param::run_time:
			return "run_time"; 
		case utl::rtree_param::compile_time:
			return "compile_time"; 
		}
		return{};
	}
	
	std::string nameof_split(rtree_split split)
	{
		switch (split)
		{
		case utl::rtree_split::linear:
			return "linear"; 
		case utl::rtree_split::quadratic:
			return "quadratic"; 
		case utl::rtree_split::rstar:
			return "rstar"; 
		}
		return{};
	}
	
	std::string nameof_load(rtree_load load)
	{
		switch (load)
		{
		case utl::rtree_load::bulk:
			return "bulk"; 
		case utl::rtree_load::iterative:
			return "iterative"; 
		}
		return{}; 
	}

	std::ostream& operator<<(std::ostream& os, rtree_param param)
	{
		os << nameof_param(param); 
		return os; 
	}

	std::ostream& operator<<(std::ostream& os, rtree_split split)
	{
		os << nameof_split(split); 
		return os;
	}

	std::ostream& operator<<(std::ostream& os, rtree_load load)
	{
		os << nameof_load(load); 
		return os;
	}

	std::string get_info_header(rtree_param param, rtree_split split, rtree_load load)
	{
		return nameof_param(param) + " | " + nameof_split(split) + " | " + nameof_load(load);
	}
}
