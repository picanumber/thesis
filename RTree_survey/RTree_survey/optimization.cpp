#include "optimization.h"
#include <iostream>
#include <cassert>
#include <stdio.h>
#include <vector>
#include <CODEine/benchmark.h>
#include "bmk_utils.h"
#include <boost/geometry/index/rtree.hpp>

using namespace utl; 
using  point_t = bg::model::point<double, 2, bg::cs::cartesian>;
using  box_t   = bg::model::box<point_t>;

namespace 
{
	template <std::size_t D, class coord_t>
	std::vector<utl::box_type<2, double>> make_input(input_maker method)
	{
		switch (method)
		{
		case utl::input_maker::from_file:
			return utl::read_from_shapefile("../shape_eg_data/gadm28.shp");
		case utl::input_maker::rand_gen:
			return utl::generate_boxes<D, coord_t>(1'000'000, 10);
		default:
			return{};
		}
	}

	template <class boxes_t>
	auto CreateSearchSpace(boxes_t const& boxes, std::size_t cardinality)
	{
		boxes_t ret;
		std::size_t il = boxes.size(); 
		
		ret.reserve(cardinality);
		for (size_t i = 0; i < cardinality; i++)
		{
			ret.emplace_back(utl::bloat_box(boxes[i % il], 10));
		}
		
		return ret;
	}
}

int test_optimizer(int num, char const * a, char const * b, char const * c)
{
	std::cout << "what the hell am I doing here\n"; 

	return (int)(num + strlen(a) + strlen(b) + strlen(c)); 
}


template <class rtree_t, class F>
std::chrono::milliseconds run_experiment_impl(
	rtree_split split, 
	std::vector<box_t> const& boxes, 
	std::vector<box_t> const& query_boxes,
	std::size_t tree_sz,
	std::size_t nqueries, std::size_t min_capacity, std::size_t max_capacity, F query)
{
	bmk::timeout<> to; 

	std::vector<rtree_t> trees; 
	rtree_split_t<rtree_t> rtree_parameters(max_capacity, min_capacity); 
	
	switch (split)
	{
	case rtree_split::bulk:
	{
		auto ite = std::cbegin(boxes);
		std::advance(ite, tree_sz);

		to.tic();
		rtree_t local_tree(std::cbegin(boxes), ite, rtree_parameters);
		to.toc();

		trees.emplace_back(std::move(local_tree)); 
	}
	break;
	default:
	{
		to.tic();
		rtree_t local_tree(rtree_parameters);
		for (size_t i = 0; i < tree_sz; i++)
		{
			local_tree.insert(boxes[i]);
		}
		to.toc();

		trees.emplace_back(std::move(local_tree));
	}
	break;
	}

	assert(!trees.empty()); 
	rtree_t &tree = trees.back(); 

	to.tic(); 
	for (auto const& win : query_boxes)
	{
		for (auto it = tree.qbegin(query(win)); it != tree.qend(); ++it)
		{
			bmk::doNotOptimizeAway(it); 
		}
	}
	to.toc(); 



	return to.duration(); 
}

template <class rtree_t>
std::chrono::milliseconds run_experiment(
	rtree_split split, 
	std::vector<box_t> const& boxes,
	std::vector<box_t> const& qboxes,
	std::size_t tree_sz, std::size_t minc, std::size_t maxc, std::size_t nqueries, int qtype)
{
	switch (qtype)
	{
	case Q_WITHIN:
		return run_experiment_impl<rtree_t>(
			split, boxes, qboxes, tree_sz, nqueries, minc, maxc, bgi::within<box_t>);
	case Q_CONTAINS:
		return run_experiment_impl<rtree_t>(
			split, boxes, qboxes, tree_sz, nqueries, minc, maxc, bgi::contains<box_t>);
	case Q_COVERED_BY:
		return run_experiment_impl<rtree_t>(
			split, boxes, qboxes, tree_sz, nqueries, minc, maxc, bgi::covered_by<box_t>);
	case Q_COVERS:
		return run_experiment_impl<rtree_t>(
			split, boxes, qboxes, tree_sz, nqueries, minc, maxc, bgi::covers<box_t>);
	case Q_DISJOINT:
		return run_experiment_impl<rtree_t>(
			split, boxes, qboxes, tree_sz, nqueries, minc, maxc, bgi::disjoint<box_t>);
	case Q_INTERSECTS:
		return run_experiment_impl<rtree_t>(
			split, boxes, qboxes, tree_sz, nqueries, minc, maxc, bgi::intersects<box_t>);
	case Q_KNN:
		return run_experiment_impl<rtree_t>(
			split, boxes, qboxes, tree_sz, nqueries, minc, maxc, utl::knn<10, box_t>);
	case Q_OVERLAPS:
	default: // until I have proper error handling
		return run_experiment_impl<rtree_t>(
			split, boxes, qboxes, tree_sz, nqueries, minc, maxc, bgi::overlaps<box_t>);
	}
}

// experiments use milliseconds
int run_rtree_operations(
	int dataset, int numElems, int numQs, int qType, // parameters that set the problem
	int minNodes, int maxNodes, int splitType)       // parameters to optimize
{

	auto input_method = (DATASET_REAL2D == dataset) ?
		input_maker::from_file : input_maker::rand_gen;

	std::vector<std::chrono::milliseconds> timings; 

	std::vector<box_t> boxes = make_input<2, double>(input_method);
	std::vector<box_t> qboxs = CreateSearchSpace(boxes, (std::size_t)numQs); 

	for (size_t i = 0; i < 10; i++)
	{
		switch (splitType)
		{
		case LOAD_BULK:
			timings.push_back(run_experiment<bgi::rtree<box_t, bgi::dynamic_linear>>(
				rtree_split::bulk, boxes, qboxs, numElems, minNodes, maxNodes, numQs, qType));
		case LOAD_LIN:
			timings.push_back(run_experiment<bgi::rtree<box_t, bgi::dynamic_linear>>(
				rtree_split::linear, boxes, qboxs, numElems, minNodes, maxNodes, numQs, qType));
		case LOAD_QDRT:
			timings.push_back(run_experiment<bgi::rtree<box_t, bgi::dynamic_quadratic>>(
				rtree_split::quadratic, boxes, qboxs, numElems, minNodes, maxNodes, numQs, qType));
		case LOAD_RSTAR:
			timings.push_back(run_experiment<bgi::rtree<box_t, bgi::dynamic_rstar>>(
				rtree_split::rstar, boxes, qboxs, numElems, minNodes, maxNodes, numQs, qType));
		}
	}

	auto it_min = std::min_element(timings.begin(), timings.end()); 

	return it_min->count(); 
}
