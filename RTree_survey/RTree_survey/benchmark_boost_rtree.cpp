#include <vector>
#include <iostream>
#include <type_traits>

#include <boost/geometry/index/rtree.hpp>
#include <CODEine/benchmark.h>
#include "bmk_utils.h"

using namespace utl; 

// TOOD: how about using 2D AND 3D points ???
namespace
{
	template <class Box> struct inner_pt;

	template <class coord_t, std::size_t D>
	struct inner_pt <
		bg::model::box<bg::model::point<coord_t, D, bg::cs::cartesian>>
	>
	{
		using type = bg::model::point<coord_t, D, bg::cs::cartesian>;
	};

	template <class B>
	using inner_pt_t = typename inner_pt<B>::type;
} // ~ type utilities

namespace 
{

	template <
		enum class rtree_param param, 
		template <std::size_t...> class split_t, 
		typename box_t
	>
	std::enable_if_t<param == rtree_param::compile_time, int> 
		bmk_impl(rtree_split split, rtree_load load, std::vector<box_t> const& boxes)
	{
		std::cout << "----------------------- compile time params version\n";

		std::string const lib("bgi_ct");

		// Generate random objects for indexing
		//auto const boxes = sibench::generate_boxes(sibench::max_insertions);

		std::size_t const max_capacity = 1024;
		std::size_t const min_capacity = 340;

		using point_t = inner_pt_t<box_t>; 
		using rtree_t = bgi::rtree<box_t, split_t<max_capacity, min_capacity>>;

		rtree_t rtree; 

		return 0;
	}

	template <
		enum class rtree_param param, 
		template <std::size_t...> class split_t,
		typename box_t
	>
	std::enable_if_t<param == rtree_param::run_time, int> 
		bmk_impl(rtree_split split, rtree_load load, std::vector<box_t> const& boxes) 
	{
		std::cout << "--------------------------- run time params version\n";

		return 0;
	}
}

template <class box_t>
int do_rtree_bmk(
	rtree_param param, rtree_split split, rtree_load load, std::vector<box_t> const& boxes)
{
	switch (param)
	{
	case rtree_param::run_time:
		switch (split)
		{
		case rtree_split::linear: 
			return bmk_impl<rtree_param::run_time, bgi::linear>(split, load, boxes);
		case rtree_split::quadratic:
			return bmk_impl<rtree_param::run_time, bgi::quadratic>(split, load, boxes);
		case rtree_split::rstar:
			return bmk_impl<rtree_param::run_time, bgi::rstar>(split, load, boxes);
		default:
			return 1; 
		}
	case utl::rtree_param::compile_time:
		switch (split)
		{
		case rtree_split::linear:
			return bmk_impl<rtree_param::compile_time, bgi::linear>(split, load, boxes);
		case rtree_split::quadratic:
			return bmk_impl<rtree_param::compile_time, bgi::quadratic>(split, load, boxes);
		case rtree_split::rstar:
			return bmk_impl<rtree_param::compile_time, bgi::rstar>(split, load, boxes);
		default:
			return 1;
		}
	default:
		return 1; 
	}
}

int benchmark_boost_rtree()
{
	std::cout <<
		"BEGIN=====================================================\n"
		"==========================================================\n\n";

	typedef bg::model::point<double, 2, bg::cs::cartesian> point_t;
	typedef bg::model::box<point_t> box_t;
	
	std::vector<box_t> boxes = utl::generate_boxes<2, double>(10'000, 10); 

	int ret = do_rtree_bmk(rtree_param::compile_time, rtree_split::linear, rtree_load::bulk, boxes);
	
	std::cout <<
		"\n==========================================================\n"
		"=======================================================END\n";

	return ret; 
}
