#include <vector>
#include <iostream>
#include <type_traits>

#include <boost/geometry/index/rtree.hpp>
#include <CODEine/benchmark.h>
#include "bmk_utils.h"

#define FULL_SCALE 0

#ifdef _DEBUG
	#define FACTOR 1
#else
	#define FACTOR 10
#endif

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
	rtree_t make_rtree(rtree_split split, boxes_t const& boxes, std::size_t qtree_sz)
	{
		rtree_t subject;
	
		if (rtree_split::bulk == split)
		{
			auto ite = std::cbegin(boxes);
			std::advance(ite, qtree_sz);
			rtree_t tmp(std::cbegin(boxes), ite);
			subject.swap(tmp);
		}
		else
		{
			for (size_t i = 0; i < qtree_sz; i++)
			{
				subject.insert(boxes[i]);
			}
		}

		return subject; 
	}

	template <class rtree_t, class boxes_t>
	struct load_experiment
	{
		boxes_t const& _boxes;
		rtree_split const _split;

		load_experiment(boxes_t const& boxes, rtree_split split)
			: _boxes(boxes)
			, _split(split)
		{
		}
		
		void operator()(std::size_t numElems)
		{
			std::size_t ni = 0; 

			switch (_split)
			{
			case rtree_split::bulk:
			{
				auto ite = std::cbegin(_boxes); 
				std::advance(ite, numElems); 
				
				rtree_t rtree(std::cbegin(_boxes), ite); 
				
				ni = rtree.size(); 
			}
			break;
			default: 
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

	template <class rtree_t, class boxes_t, class time_t, class clock_t>
	struct query_experiment
	{
		rtree_t const* _rtree;
		boxes_t const& _boxes; 
		std::size_t    _numqs;
		std::size_t    _nhits;

		query_experiment(rtree_t const* rtree, boxes_t const& boxes, std::size_t numqs)
			: _rtree(rtree)
			, _boxes(boxes)
			, _numqs(numqs)
			, _nhits{}
		{
		}

		// this performs only qlimit queries using _rtree
		bmk::timeout_ptr<time_t, clock_t> operator()(std::size_t qlimit)
		{
			bmk::timeout_ptr<time_t, clock_t> to = 
				std::make_unique<bmk::timeout<time_t, clock_t>>();

			to->tic(); 
			auto qs = CreateSearchSpace(qlimit); 
			to->toc(); 

			boxes_t result; 
			for (auto const& win : qs)
			{
				_rtree->query(bgi::intersects(win), std::back_inserter(result));
			}
			_nhits += result.size(); 

			return to; 
		}

		// this performs _numqs queries using a new rtree of maxcapacity
		template <class Rtree>
		auto operator()(Rtree *rtree)
		{
		}
	private:
		auto CreateSearchSpace(std::size_t cardinality)
		{
			boxes_t ret; 
			ret.reserve(cardinality); 

			for (size_t i = 0; i < cardinality; i++)
			{
				ret.emplace_back(utl::bloat_box(_boxes[i], 10)); 
			}

			return ret; 
		}
	};

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
			bmk_t& query_ct, 
			bmk_t& load_rt, 
			bmk_t& query_rt)
	{
		std::cout << get_info_header(param, split) << " ============ BEGIN\n"; 

		std::size_t const max_capacity = 1024; // TODO: Fix those
		std::size_t const min_capacity = 340;

		using point_t = inner_pt_t<box_t>;
		using boxes_t = std::vector<box_t>; 
		using rtree_t = bgi::rtree<box_t, split_t<max_capacity, min_capacity>>;

		std::cout << "\tload experiment...\n"; 
		load_ct.run(utl::nameof_split(split), 1, 
			load_experiment<rtree_t, boxes_t>(boxes, split), "number of elements", {
#if !FULL_SCALE
			100 * FACTOR, 200 * FACTOR, 300 * FACTOR, 400 * FACTOR, 500 * FACTOR
#else
			100'000, 200'000, 300'000, 400'000, 500'000, 600'000, 700'000, 800'000, 900'000, 1'000'000 
#endif
		});

		auto subject = make_rtree<rtree_t>(split, boxes, query_tree_sz);
		std::cout << "\tquery experiment...\n";
		query_ct.run(utl::nameof_split(split), 1, query_experiment<rtree_t, boxes_t,
			bmk_t::time_t, bmk_t::clock_t>(&subject, boxes, num_queries), "number of queries", {
#if !FULL_SCALE
				100 * FACTOR, 200 * FACTOR, 300 * FACTOR, 400 * FACTOR, 500 * FACTOR
#else
				10'000, 20'000, 30'000, 40'000, 50'000, 60'000, 70'000, 80'000, 90'000, 100'000
#endif
		});

		std::cout << get_info_header(param, split) << " ============== END\n\n";
		return 0;
	}

	template <
		enum class rtree_param param,
		template <std::size_t...> class split_t,
		typename box_t
	> 
		std::enable_if_t<param == rtree_param::run_time, int> 
		bmk_impl(
			rtree_split split, 
			std::vector<box_t> const& boxes,
			std::size_t query_tree_sz, 
			std::size_t num_queries,
			bmk_t& load_ct, 
			bmk_t& query_ct, 
			bmk_t& load_rt, 
			bmk_t& query_rt)
	{
		std::cout << "--------------------------- run time params version\n";

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
	bmk_t& query_ct, 
	bmk_t& load_rt, 
	bmk_t& query_rt)
{
	switch (param)
	{
	case rtree_param::run_time:
		switch (split)
		{
		case rtree_split::bulk:
		case rtree_split::linear: 
			return bmk_impl<rtree_param::run_time, bgi::linear>(
				split, boxes, qtree_sz, nqueries, load_ct, query_ct, load_rt, query_rt);
		case rtree_split::quadratic:
			return bmk_impl<rtree_param::run_time, bgi::quadratic>(
				split, boxes, qtree_sz, nqueries, load_ct, query_ct, load_rt, query_rt);
		case rtree_split::rstar:
			return bmk_impl<rtree_param::run_time, bgi::rstar>(
				split, boxes, qtree_sz, nqueries, load_ct, query_ct, load_rt, query_rt);
		default:
			return 1; 
		}
	case utl::rtree_param::compile_time:
		switch (split)
		{
		case rtree_split::bulk:
		case rtree_split::linear:
			return bmk_impl<rtree_param::compile_time, bgi::linear>(
				split, boxes, qtree_sz, nqueries, load_ct, query_ct, load_rt, query_rt);
		case rtree_split::quadratic:
			return bmk_impl<rtree_param::compile_time, bgi::quadratic>(
				split, boxes, qtree_sz, nqueries, load_ct, query_ct, load_rt, query_rt);
		case rtree_split::rstar:
			return bmk_impl<rtree_param::compile_time, bgi::rstar>(
				split, boxes, qtree_sz, nqueries, load_ct, query_ct, load_rt, query_rt);
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

	bmk_t load_ct, query_ct, load_rt, query_rt; 

	std::vector<box_t> boxes = utl::generate_boxes<2, double>(1'000'000, 10); 

	bmk::timeout<std::chrono::minutes> to; 
	to.tic(); 
	for (auto&& params : utl::cartesian_product(param_vs, split_vs))
	{
		do_rtree_bmk(std::get<0>(params), std::get<1>(params), 
			boxes, 
#if !FULL_SCALE
			100 * FACTOR, 10 * FACTOR, 
#else
			1'000'000, 100'000, 
#endif
			load_ct, query_ct, load_rt, query_rt);
	}
	to.toc(); 

	std::cout << "testing took " << to.duration().count() << "minutes overall\n"; 

	load_ct.serialize("Loading time: Fixed capacity", "load_ct.txt"); 
	load_rt.serialize("Loading time: Varying capacity", "load_rt.txt"); 
	query_ct.serialize("Query time: Fixed capacity", "query_ct.txt"); 
	query_rt.serialize("Query time: Varying capacity", "query_rt.txt"); 
	
	std::cout << "\n=======================================================END\n";
	return 0; 
}
