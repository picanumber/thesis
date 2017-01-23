#include <vector>
#include <iostream>
#include <type_traits>

#include <boost/geometry/index/rtree.hpp>
#include <CODEine/benchmark.h>
#include "bmk_utils.h"

#define FULL_SCALE 0

using namespace utl; 
using real_secs_t = std::chrono::duration<double, std::ratio<1>>; 
using bmk_t = bmk::benchmark<real_secs_t>; 

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
	template <class rtree_t, class boxes_t>
	struct load_experiment
	{
		boxes_t const& _boxes;
		rtree_load const _load;

		load_experiment(boxes_t const& boxes, rtree_load load)
			: _boxes(boxes)
			, _load(load)
		{
		}
		
		void operator()(std::size_t numElems)
		{
			std::size_t ni = 0; 

			switch (_load)
			{
			case rtree_load::bulk:
			{
				auto it{ std::cbegin(_boxes) }, ite{ std::cbegin(_boxes) };
				std::advance(ite, numElems); 
				rtree_t rtree(it, ite); 
				ni = rtree.size(); 
			}
			break;
			case rtree_load::iterative:
			{
				rtree_t rtree;
				for (size_t i = 0; i < numElems; i++)
				{
					rtree.insert(_boxes[i]);
				}
				ni = rtree.size(); 
			}
			break;
			}

			assert(numElems == ni); // also prevents from optimizing away the temp trees
		}
	};

	template <
		enum class rtree_param param,
		template <std::size_t...> class split_t,
		typename box_t
	> 
		std::enable_if_t<param == rtree_param::compile_time, int> bmk_impl(
			rtree_split split, rtree_load load, std::vector<box_t> const& boxes,
			bmk_t& load_ct, bmk_t& query_ct, bmk_t& load_rt, bmk_t& query_rt)
	{
		std::cout << get_info_header(param, split, load) << " ============ BEGIN\n"; 

		std::size_t const max_capacity = 1024; // TODO: Fix those
		std::size_t const min_capacity = 340;

		using point_t = inner_pt_t<box_t>;
		using rtree_t = bgi::rtree<box_t, split_t<max_capacity, min_capacity>>;
		using boxes_t = std::vector<box_t>; 

		load_ct.run(rtree_load::iterative == load ? utl::nameof_split(split) : nameof_load(load), 1, 
			load_experiment<rtree_t, boxes_t>(boxes, load), "number of elements", {
#if !FULL_SCALE
			10'000, 20'000, 30'000, 40'000, 50'000
#else
			100'000, 200'000, 300'000, 400'000, 500'000, 600'000, 700'000, 800'000, 900'000, 1'000'000 
#endif
		});

		std::cout << get_info_header(param, split, load) << " ============== END\n\n";
		return 0;
	}

	template <
		enum class rtree_param param,
		template <std::size_t...> class split_t,
		typename box_t
	> 
		std::enable_if_t<param == rtree_param::run_time, int> bmk_impl(
			rtree_split split, rtree_load load, std::vector<box_t> const& boxes,
			bmk_t& load_ct, bmk_t& query_ct, bmk_t& load_rt, bmk_t& query_rt)
	{
		std::cout << "--------------------------- run time params version\n";

		return 0;
	}
}

template <class box_t>
int do_rtree_bmk(
	rtree_param param, rtree_split split, rtree_load load, std::vector<box_t> const& boxes, 
	bmk_t& load_ct, bmk_t& query_ct, bmk_t& load_rt, bmk_t& query_rt)
{
	switch (param)
	{
	case rtree_param::run_time:
		switch (split)
		{
		case rtree_split::linear: 
			return bmk_impl<rtree_param::run_time, bgi::linear>(
				split, load, boxes, load_ct, query_ct, load_rt, query_rt);
		case rtree_split::quadratic:
			return bmk_impl<rtree_param::run_time, bgi::quadratic>(
				split, load, boxes, load_ct, query_ct, load_rt, query_rt);
		case rtree_split::rstar:
			return bmk_impl<rtree_param::run_time, bgi::rstar>(
				split, load, boxes, load_ct, query_ct, load_rt, query_rt);
		default:
			return 1; 
		}
	case utl::rtree_param::compile_time:
		switch (split)
		{
		case rtree_split::linear:
			return bmk_impl<rtree_param::compile_time, bgi::linear>(
				split, load, boxes, load_ct, query_ct, load_rt, query_rt);
		case rtree_split::quadratic:
			return bmk_impl<rtree_param::compile_time, bgi::quadratic>(
				split, load, boxes, load_ct, query_ct, load_rt, query_rt);
		case rtree_split::rstar:
			return bmk_impl<rtree_param::compile_time, bgi::rstar>(
				split, load, boxes, load_ct, query_ct, load_rt, query_rt);
		default:
			return 1;
		}
	default:
		return 1; 
	}
}

const std::vector<rtree_param> param_vs{ rtree_param::run_time, rtree_param::compile_time }; 
const std::vector<rtree_split> split_vs{ rtree_split::linear, rtree_split::quadratic, rtree_split::rstar }; 
const std::vector<rtree_load>  load_vs{ rtree_load::bulk, rtree_load::iterative }; 

int benchmark_boost_rtree()
{
	std::cout << "BEGIN=====================================================\n\n";

	typedef bg::model::point<double, 2, bg::cs::cartesian> point_t;
	typedef bg::model::box<point_t> box_t;


	auto vt = utl::cartesian_product(param_vs, split_vs, load_vs); 
	for (auto& elem : vt) utl::print_tuple(std::cout, elem); 

	return 0; 
	
	std::vector<box_t> boxes = utl::generate_boxes<2, double>(1'000'000, 10); 

	bmk_t load_ct, query_ct, load_rt, query_rt; 

	// max capacity must be one of the parameters
	do_rtree_bmk(rtree_param::compile_time, rtree_split::linear, rtree_load::iterative, 
		boxes, load_ct, query_ct, load_rt, query_rt);
	do_rtree_bmk(rtree_param::compile_time, rtree_split::quadratic, rtree_load::iterative,
		boxes, load_ct, query_ct, load_rt, query_rt);
	do_rtree_bmk(rtree_param::compile_time, rtree_split::rstar, rtree_load::iterative,
		boxes, load_ct, query_ct, load_rt, query_rt);
	do_rtree_bmk(rtree_param::compile_time, rtree_split::linear, rtree_load::bulk,
		boxes, load_ct, query_ct, load_rt, query_rt);

	load_ct.serialize("Loading time: Fixed capacity", "load_ct.txt"); 
	load_rt.serialize("Loading time: Varying capacity", "load_rt.txt"); 
	query_ct.serialize("Query time: Fixed capacity", "query_ct.txt"); 
	query_rt.serialize("Query time: Varying capacity", "query_rt.txt"); 
	
	std::cout << "\n=======================================================END\n";
	return 0; 
}
