#include <vector>
#include <iostream>
#include <type_traits>

#include <boost/geometry/index/rtree.hpp>
#include <CODEine/benchmark.h>
#include "bmk_utils.h"
#include "rtree_experiments.h"

#define FULL_SCALE 1

#ifdef _DEBUG
	#define FACTOR 1
#else
	#define FACTOR 10
#endif

#if !FULL_SCALE
#define NUM_REPS 1
#define TREE_SZ  100 * FACTOR
#define QUERY_SZ 10 * FACTOR
#define XVALS    100 * FACTOR, 200 * FACTOR, 300 * FACTOR, 400 * FACTOR, 500 * FACTOR
#define MAXCVALS 8, 16, 32
#define NUM_MAXC 3
#else
#define NUM_REPS 1
#define TREE_SZ  1'000'000
#define QUERY_SZ 100'000
#define XVALS    100'000, 200'000, 300'000, 400'000, 500'000, 600'000, 700'000, 800'000, 900'000, 1'000'000
#define MAXCVALS 8, 16, 32, 48, 64, 96, 128, 160, 192, 224, 256
#define NUM_MAXC 11
#endif

using namespace utl; 
using real_secs_t = std::chrono::duration<double, std::ratio<1>>;
using bmk_t = bmk::benchmark<real_secs_t>;
namespace bre = boost_rtree_experiments; 

constexpr std::size_t max_capacity = 1024; // TODO: Fix those
constexpr std::size_t min_capacity = 340;

namespace 
{
	template <
		enum class rtree_param param,
		template <std::size_t...> class split_t,
		typename box_t
	>
		std::enable_if_t<param == rtree_param::compile_time, int>
		bmk_impl(
			rtree_split split, 
			std::vector<box_t> const& boxes,
			std::size_t query_tree_sz, 
			std::size_t num_queries,
			bmk_t& load_ct,
			bmk_t& load_rt,
			bmk_t& query_ct_contains,
			bmk_t& query_ct_covered_by,
			bmk_t& query_ct_covers,
			bmk_t& query_ct_disjoint,
			bmk_t& query_ct_intersects,
			bmk_t& query_ct_overlaps,
			bmk_t& query_ct_within,
			bmk_t& query_rt_contains,
			bmk_t& query_rt_covered_by,
			bmk_t& query_rt_covers,
			bmk_t& query_rt_disjoint,
			bmk_t& query_rt_intersects,
			bmk_t& query_rt_overlaps,
			bmk_t& query_rt_within)
	{
		using point_t = inner_pt_t<box_t>;
		using boxes_t = std::vector<box_t>; 
		using rtree_t = bgi::rtree<box_t, split_t<max_capacity, min_capacity>>;

		std::cout << get_info_header(param, split) << " ====================== BEGIN\n"; 

		std::cout << "\tload experiment...\n"; 
		load_ct.run(
			utl::nameof_split(split), 
			NUM_REPS,
			bre::load_experiment<rtree_t, boxes_t, bmk_t::time_t, bmk_t::clock_t>(boxes, split),
			"number of elements", 
			{ XVALS }
		);

		auto subject = make_rtree<rtree_t>(split, boxes, query_tree_sz);

		std::cout << "\tbgi::contains query experiment...\n";
		query_ct_contains.run(
			utl::nameof_split(split),
			NUM_REPS,
			bre::query_experiment
			< decltype(bgi::contains<box_t>), rtree_t, boxes_t, bmk_t::time_t, bmk_t::clock_t>(
				bgi::contains<box_t>, &subject, boxes),
			"number of queries",
			{ XVALS }
		);

		std::cout << "\tbgi::covered_by query experiment...\n";
		query_ct_covered_by.run(
			utl::nameof_split(split), 
			NUM_REPS,
			bre::query_experiment
			< decltype(bgi::covered_by<box_t>), rtree_t, boxes_t, bmk_t::time_t, bmk_t::clock_t>(
				bgi::covered_by<box_t>, &subject, boxes),
			"number of queries", 
			{ XVALS }
		);

		std::cout << "\tbgi::covers query experiment...\n";
		query_ct_covers.run(
			utl::nameof_split(split), 
			NUM_REPS,
			bre::query_experiment
			< decltype(bgi::covers<box_t>), rtree_t, boxes_t, bmk_t::time_t, bmk_t::clock_t>(
				bgi::covers<box_t>, &subject, boxes),
			"number of queries", 
			{ XVALS }
		);

		std::cout << "\tbgi::disjoint query experiment...\n";
		query_ct_disjoint.run(
			utl::nameof_split(split), 
			NUM_REPS,
			bre::query_experiment
			< decltype(bgi::disjoint<box_t>), rtree_t, boxes_t, bmk_t::time_t, bmk_t::clock_t>(
				bgi::disjoint<box_t>, &subject, boxes),
			"number of queries", 
			{ XVALS }
		);
		
		std::cout << "\tbgi::intersects query experiment...\n";
		query_ct_intersects.run(
			utl::nameof_split(split), 
			NUM_REPS,
			bre::query_experiment
			< decltype(bgi::intersects<box_t>), rtree_t, boxes_t, bmk_t::time_t, bmk_t::clock_t>(
				bgi::intersects<box_t>, &subject, boxes), 
			"number of queries", 
			{ XVALS }
		);

		std::cout << "\tbgi::overlaps query experiment...\n";
		query_ct_overlaps.run(
			utl::nameof_split(split), 
			NUM_REPS,
			bre::query_experiment
			< decltype(bgi::overlaps<box_t>), rtree_t, boxes_t, bmk_t::time_t, bmk_t::clock_t>(
				bgi::overlaps<box_t>, &subject, boxes),
			"number of queries", 
			{ XVALS }
		);

		std::cout << "\tbgi::within query experiment...\n";
		query_ct_within.run(
			utl::nameof_split(split), 
			NUM_REPS,
			bre::query_experiment
			< decltype(bgi::within<box_t>), rtree_t, boxes_t, bmk_t::time_t, bmk_t::clock_t>(
				bgi::within<box_t>, &subject, boxes),
			"number of queries", 
			{ XVALS }
		);

		std::cout << get_info_header(param, split) << " ======================== END\n\n";

		return 0;
	}

	template <
		enum class rtree_param param,
		class split_t,
		typename box_t
	> 
		std::enable_if_t<param == rtree_param::run_time, int> 
		bmk_impl(
			rtree_split split, 
			std::vector<box_t> const& boxes,
			std::size_t query_tree_sz, 
			std::size_t num_queries,
			bmk_t& load_ct,
			bmk_t& load_rt,
			bmk_t& query_ct_contains,
			bmk_t& query_ct_covered_by,
			bmk_t& query_ct_covers,
			bmk_t& query_ct_disjoint,
			bmk_t& query_ct_intersects,
			bmk_t& query_ct_overlaps,
			bmk_t& query_ct_within,
			bmk_t& query_rt_contains,
			bmk_t& query_rt_covered_by,
			bmk_t& query_rt_covers,
			bmk_t& query_rt_disjoint,
			bmk_t& query_rt_intersects,
			bmk_t& query_rt_overlaps,
			bmk_t& query_rt_within)
	{
		using point_t = inner_pt_t<box_t>;
		using boxes_t = std::vector<box_t>;
		using rtree_t = bgi::rtree<box_t, split_t>;

		std::cout << get_info_header(param, split) << " ====================== BEGIN\n";
		std::vector<rtree_t> used_rtrees; 

		std::cout << "\tload experiment...\n";
		load_rt.run(
			utl::nameof_split(split),
			NUM_REPS,
			bre::load_experiment<rtree_t, boxes_t, bmk_t::time_t, bmk_t::clock_t>(
				boxes, split, query_tree_sz, &used_rtrees),
			"max capacity (min capacity = max * 0.5)",
			{ MAXCVALS }
		);

		std::vector<std::reference_wrapper<rtree_t>> vc(used_rtrees.begin(), used_rtrees.end());

		std::cout << "\tbgi::contains query experiment...\n";
		query_rt_contains.run(
			utl::nameof_split(split),
			NUM_REPS,
			bre::query_experiment
			<decltype(bgi::contains<box_t>), rtree_t, boxes_t, bmk_t::time_t, bmk_t::clock_t>(
				bgi::contains<box_t>, boxes, num_queries, &used_rtrees),
			"max capacity (min capacity = max * 0.5)",
			{ MAXCVALS }
		);

		std::cout << "\tbgi::covered_by query experiment...\n";
		query_rt_covered_by.run(
			utl::nameof_split(split),
			NUM_REPS,
			bre::query_experiment
			<decltype(bgi::covered_by<box_t>), rtree_t, boxes_t, bmk_t::time_t, bmk_t::clock_t>(
				bgi::covered_by<box_t>, boxes, num_queries, &used_rtrees),
			"max capacity (min capacity = max * 0.5)",
			{ MAXCVALS }
		);

		std::cout << "\tbgi::covers query experiment...\n";
		query_rt_covers.run(
			utl::nameof_split(split),
			NUM_REPS,
			bre::query_experiment
			<decltype(bgi::covers<box_t>), rtree_t, boxes_t, bmk_t::time_t, bmk_t::clock_t>(
				bgi::covers<box_t>, boxes, num_queries, &used_rtrees),
			"max capacity (min capacity = max * 0.5)",
			{ MAXCVALS }
		);

		std::cout << "\tbgi::disjoint query experiment...\n";
		query_rt_disjoint.run(
			utl::nameof_split(split),
			NUM_REPS,
			bre::query_experiment
			<decltype(bgi::disjoint<box_t>), rtree_t, boxes_t, bmk_t::time_t, bmk_t::clock_t>(
				bgi::disjoint<box_t>, boxes, num_queries, &used_rtrees),
			"max capacity (min capacity = max * 0.5)",
			{ MAXCVALS }
		);
			
		std::cout << "\tbgi::intersects query experiment...\n";
		query_rt_intersects.run(
			utl::nameof_split(split),
			NUM_REPS,
			bre::query_experiment
			<decltype(bgi::intersects<box_t>), rtree_t, boxes_t, bmk_t::time_t, bmk_t::clock_t>(
				bgi::intersects<box_t>, boxes, num_queries, &used_rtrees),
			"max capacity (min capacity = max * 0.5)",
			{ MAXCVALS }
		);

		std::cout << "\tbgi::overlaps query experiment...\n";
		query_rt_overlaps.run(
			utl::nameof_split(split),
			NUM_REPS,
			bre::query_experiment
			<decltype(bgi::overlaps<box_t>), rtree_t, boxes_t, bmk_t::time_t, bmk_t::clock_t>(
				bgi::overlaps<box_t>, boxes, num_queries, &used_rtrees),
			"max capacity (min capacity = max * 0.5)",
			{ MAXCVALS }
		);

		std::cout << "\tbgi::within query experiment...\n";
		query_rt_within.run(
			utl::nameof_split(split),
			NUM_REPS,
			bre::query_experiment
			<decltype(bgi::within<box_t>), rtree_t, boxes_t, bmk_t::time_t, bmk_t::clock_t>(
				bgi::within<box_t>, boxes, num_queries, &used_rtrees),
			"max capacity (min capacity = max * 0.5)",
			{ MAXCVALS }
		);

		std::cout << get_info_header(param, split) << " ======================== END\n\n";
		
		return 0;
	}
}

template <class box_t>
int do_rtree_bmk(
	rtree_param param, 
	rtree_split split, 
	std::vector<box_t> const& boxes, 
	std::size_t qtree_sz,
	std::size_t nqueries, 
	bmk_t& load_ct, 
	bmk_t& load_rt, 
	bmk_t& query_ct_contains, 
	bmk_t& query_ct_covered_by, 
	bmk_t& query_ct_covers, 
	bmk_t& query_ct_disjoint, 
	bmk_t& query_ct_intersects, 
	bmk_t& query_ct_overlaps, 
	bmk_t& query_ct_within, 
	bmk_t& query_rt_contains, 
	bmk_t& query_rt_covered_by, 
	bmk_t& query_rt_covers, 
	bmk_t& query_rt_disjoint, 
	bmk_t& query_rt_intersects, 
	bmk_t& query_rt_overlaps, 
	bmk_t& query_rt_within)
{
	switch (param)
	{
	case rtree_param::run_time:
		switch (split)
		{
		case rtree_split::bulk:
		case rtree_split::linear: 
			return bmk_impl<rtree_param::run_time, bgi::dynamic_linear>(
				split, boxes, qtree_sz, nqueries, load_ct, load_rt,
				query_ct_contains, query_ct_covered_by, query_ct_covers,
				query_ct_disjoint, query_ct_intersects, query_ct_overlaps, query_ct_within,
				query_rt_contains, query_rt_covered_by, query_rt_covers,
				query_rt_disjoint, query_rt_intersects, query_rt_overlaps, query_rt_within);
		case rtree_split::quadratic:
			return bmk_impl<rtree_param::run_time, bgi::dynamic_quadratic>(
				split, boxes, qtree_sz, nqueries, load_ct, load_rt,
				query_ct_contains, query_ct_covered_by, query_ct_covers,
				query_ct_disjoint, query_ct_intersects, query_ct_overlaps, query_ct_within,
				query_rt_contains, query_rt_covered_by, query_rt_covers,
				query_rt_disjoint, query_rt_intersects, query_rt_overlaps, query_rt_within);
		case rtree_split::rstar:
			return bmk_impl<rtree_param::run_time, bgi::dynamic_rstar>(
				split, boxes, qtree_sz, nqueries, load_ct, load_rt,
				query_ct_contains, query_ct_covered_by, query_ct_covers,
				query_ct_disjoint, query_ct_intersects, query_ct_overlaps, query_ct_within,
				query_rt_contains, query_rt_covered_by, query_rt_covers,
				query_rt_disjoint, query_rt_intersects, query_rt_overlaps, query_rt_within);
		default:
			return 1; 
		}
	case utl::rtree_param::compile_time:
		switch (split)
		{
		case rtree_split::bulk:
		case rtree_split::linear:
			return bmk_impl<rtree_param::compile_time, bgi::linear>(
				split, boxes, qtree_sz, nqueries, load_ct, load_rt,
				query_ct_contains, query_ct_covered_by, query_ct_covers,
				query_ct_disjoint, query_ct_intersects, query_ct_overlaps, query_ct_within,
				query_rt_contains, query_rt_covered_by, query_rt_covers,
				query_rt_disjoint, query_rt_intersects, query_rt_overlaps, query_rt_within);
		case rtree_split::quadratic:
			return bmk_impl<rtree_param::compile_time, bgi::quadratic>(
				split, boxes, qtree_sz, nqueries, load_ct, load_rt,
				query_ct_contains, query_ct_covered_by, query_ct_covers,
				query_ct_disjoint, query_ct_intersects, query_ct_overlaps, query_ct_within,
				query_rt_contains, query_rt_covered_by, query_rt_covers,
				query_rt_disjoint, query_rt_intersects, query_rt_overlaps, query_rt_within);
		case rtree_split::rstar:
			return bmk_impl<rtree_param::compile_time, bgi::rstar>(
				split, boxes, qtree_sz, nqueries, load_ct, load_rt,
				query_ct_contains, query_ct_covered_by, query_ct_covers,
				query_ct_disjoint, query_ct_intersects, query_ct_overlaps, query_ct_within,
				query_rt_contains, query_rt_covered_by, query_rt_covers,
				query_rt_disjoint, query_rt_intersects, query_rt_overlaps, query_rt_within);
		default:
			return 1;
		}
	default:
		return 1; 
	}
}

const std::vector<rtree_param> param_vs{ 
	rtree_param::run_time, 
	rtree_param::compile_time 
}; 
const std::vector<rtree_split> split_vs{ 
	rtree_split::linear, 
	rtree_split::quadratic, 
	rtree_split::rstar, 
	rtree_split::bulk
}; 

int benchmark_boost_rtree()
{
	typedef bg::model::point<double, 2, bg::cs::cartesian> point_t;
	typedef bg::model::box<point_t> box_t;

	std::cout << "BEGIN=====================================================\n\n";

	bmk_t
		load_ct,
		load_rt,
		query_ct_contains,
		query_ct_covered_by,
		query_ct_covers,
		query_ct_disjoint,
		query_ct_intersects,
		query_ct_overlaps,
		query_ct_within,
		query_rt_contains,
		query_rt_covered_by,
		query_rt_covers,
		query_rt_disjoint,
		query_rt_intersects,
		query_rt_overlaps,
		query_rt_within; 

	std::cout << "making input...\n"; 
	std::vector<box_t> boxes = utl::generate_boxes<2, double>(1'000'000, 10); 

	bmk::timeout<std::chrono::minutes> to; 
	to.tic(); 
	for (auto&& params : utl::cartesian_product(param_vs, split_vs))
	{
		do_rtree_bmk(
			std::get<0>(params), std::get<1>(params),
			boxes, TREE_SZ, QUERY_SZ,
			load_ct,
			load_rt,
			query_ct_contains,
			query_ct_covered_by,
			query_ct_covers,
			query_ct_disjoint,
			query_ct_intersects,
			query_ct_overlaps,
			query_ct_within,
			query_rt_contains,
			query_rt_covered_by,
			query_rt_covers,
			query_rt_disjoint,
			query_rt_intersects,
			query_rt_overlaps,
			query_rt_within); 
	}
	to.toc(); 

	std::cout << "testing took " << to.duration().count() << "minutes overall\n"; 

	auto tree_sz  = std::to_string(TREE_SZ); 
	auto query_sz = std::to_string(QUERY_SZ); 
	auto maxCapty = std::to_string(max_capacity);
	auto minCapty = std::to_string(min_capacity);

	auto load_ct_name = 
		"Loading latency: { max capacity = " + maxCapty + ", min capacity = " + minCapty + " }"; 
	load_ct.serialize(load_ct_name.c_str(), "load_ct.txt"); 
	
	auto load_rt_name = "Loading latency: RTree size = " + tree_sz; 
	load_rt.serialize(load_rt_name.c_str(), "load_rt.txt"); 

	auto query_ct_name = 
		"query latency: { RTree size = " + tree_sz + ", max capacity = "
		+ maxCapty + ", min capacity = " + minCapty + " } "; 
	auto query_rt_name = 
		"query latency: { RTree size = " + tree_sz + ", number of queries = " + query_sz + " }";

	query_ct_contains  .serialize(("Contains   " + query_ct_name).c_str(), "query_ct_contains.txt");
	query_ct_covered_by.serialize(("Covered_by " + query_ct_name).c_str(), "query_ct_covered_by.txt");
	query_ct_covers    .serialize(("Covers     " + query_ct_name).c_str(), "query_ct_covers.txt");
	query_ct_disjoint  .serialize(("Disjoint   " + query_ct_name).c_str(), "query_ct_disjoint.txt");
	query_ct_intersects.serialize(("Intersects " + query_ct_name).c_str(), "query_ct_intersects.txt");
	query_ct_overlaps  .serialize(("Overlaps   " + query_ct_name).c_str(), "query_ct_overlaps.txt");
	query_ct_within    .serialize(("Within     " + query_ct_name).c_str(), "query_ct_within.txt");

	query_rt_contains  .serialize(("Contains   " + query_rt_name).c_str(), "query_rt_contains.txt");
	query_rt_covered_by.serialize(("Covered_by " + query_rt_name).c_str(), "query_rt_covered_by.txt");
	query_rt_covers    .serialize(("Covers     " + query_rt_name).c_str(), "query_rt_covers.txt");
	query_rt_disjoint  .serialize(("Disjoint   " + query_rt_name).c_str(), "query_rt_disjoint.txt");
	query_rt_intersects.serialize(("Intersects " + query_rt_name).c_str(), "query_rt_intersects.txt");
	query_rt_overlaps  .serialize(("Overlaps   " + query_rt_name).c_str(), "query_rt_overlaps.txt");
	query_rt_within    .serialize(("Within     " + query_rt_name).c_str(), "query_rt_within.txt");
	
	std::cout << "\n=======================================================END\n";
	return 0; 
}
