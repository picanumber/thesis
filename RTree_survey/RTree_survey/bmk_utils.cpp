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
}
