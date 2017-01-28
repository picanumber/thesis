#pragma once

#include <type_traits>
#include "bmk_utils.h"
#include "CODEine/benchmark.h"
#include "function_ref.h"

namespace boost_rtree_experiments
{
	using namespace utl; 

	template <class rtree_t, class boxes_t, class time_t, class clock_t>
	struct load_experiment
	{
		using timeout_ptr_t = bmk::timeout_ptr<time_t, clock_t>; 

		boxes_t const& _boxes;
		rtree_split const _split;
		std::size_t const _fixedSz; 
		std::vector<rtree_t> *_usedTrees; 

		load_experiment(
			boxes_t const& boxes, rtree_split split, 
			std::size_t fixedSz = 0, std::vector<rtree_t> *usedTrees = nullptr)
			: _boxes(boxes)
			, _split(split)
			, _fixedSz(fixedSz)
			, _usedTrees(usedTrees)
		{
		}

		template <class Sz>
		std::enable_if_t<!has_dynamic_params<rtree_t>::value && std::is_integral<Sz>::value>
		operator()(Sz numElems)
		{
			assert(!_usedTrees); 

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

		template <class Sz>
		std::enable_if_t<
			has_dynamic_params<rtree_t>::value && std::is_integral<Sz>::value, timeout_ptr_t
		>
			operator()(Sz max_capacity)
		{
			assert(_usedTrees); 

			timeout_ptr_t to = std::make_unique<bmk::timeout<time_t, clock_t>>();
			std::size_t ni = 0;
			rtree_split_t<rtree_t> rtree_parameters(max_capacity, Sz(max_capacity * 0.5)); 

			switch (_split)
			{
			case rtree_split::bulk:
			{
				auto ite = std::cbegin(_boxes);
				std::advance(ite, _fixedSz);

				rtree_t rtree(std::cbegin(_boxes), ite, rtree_parameters);

				ni = rtree.size();
				to->tic(); 
				_usedTrees->emplace_back(std::move(rtree)); 
				to->toc(); 
			}
			break;
			default:
			{
				rtree_t rtree(rtree_parameters);
				for (size_t i = 0; i < _fixedSz; i++)
				{
					rtree.insert(_boxes[i]);
				}
				ni = rtree.size();

				to->tic();
				auto pr = rtree.parameters().get_max_elements(); 
				_usedTrees->emplace_back(std::move(rtree));
				assert(pr == _usedTrees->back().parameters().get_max_elements()); 
				to->toc();
			}
			break;
			}

			assert(_fixedSz == ni); // also prevents from optimizing away the temp trees
			return to; 
		}
	};

	template <class F, class rtree_t, class boxes_t, class time_t, class clock_t>
	struct query_experiment
	{
		using trees_t = std::vector<rtree_t>; 

		rtree_t const*        _rtree;
		boxes_t const&        _boxes;
		std::size_t           _numqs;
		std::size_t           _nhits;
		trees_t              *_va_capcty_trees;
		utl::function_ref<F>  _fun;

		template <class Op>
		query_experiment(Op&& fun, rtree_t const* rtree, boxes_t const& boxes)
			: _rtree(rtree)
			, _boxes(boxes)
			, _numqs(0)
			, _nhits{}
			, _va_capcty_trees(nullptr)
			, _fun(std::forward<Op>(fun))
		{
		}

		template <class Op>
		query_experiment(Op&& fun, boxes_t const& boxes, std::size_t numqs, trees_t *trees)
			: _rtree(nullptr)
			, _boxes(boxes)
			, _numqs(numqs)
			, _nhits{}
			, _va_capcty_trees(trees)
			, _fun(std::forward<Op>(fun))
		{
		}

		// this performs only qlimit queries using _rtree
		// this performs _numqs queries using the argument tree
		// num can either be the qlimit or the capacity
		// TODO: make the default num a distinct mode change signal (e.g. 0)
		bmk::timeout_ptr<time_t, clock_t> operator()(std::size_t num = 1)
		{
			bmk::timeout_ptr<time_t, clock_t> to =
				std::make_unique<bmk::timeout<time_t, clock_t>>();

			to->tic();
			rtree_t const *rt{ nullptr };
			std::size_t queries_num{ 0 }; 
			if (_rtree)
			{
				assert(!_va_capcty_trees);
				rt = _rtree;
				queries_num = (1 == num ? 100 : num); 
			}
			else
			{
				assert(_va_capcty_trees); 
				auto it1(_va_capcty_trees->begin()), ite(_va_capcty_trees->end()); 
				auto it = std::find_if(it1, ite, [&](rtree_t const& val) { 
					return val.parameters().get_max_elements() == num; 
				}); 
				if (it != ite)
				{
					rt = &(*it); 
				}
				queries_num = (rt ? 1 : 100) * _numqs; 
			}
			assert(rt || (1 == num)); 

			auto qs = CreateSearchSpace(queries_num);
			to->toc();

			boxes_t result;
			if (1 == num && _va_capcty_trees)
			{
				assert(!rt); 
				for (auto const& cur_tree : *_va_capcty_trees)
				{
					for (auto const& win : qs)
					{
						cur_tree.query(_fun(win), std::back_inserter(result));
					}
				}
			}
			else
			{
				for (auto const& win : qs)
				{
					rt->query(_fun(win), std::back_inserter(result));
				}
			}
			_nhits += result.size();

			return to;
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
} // ~ boost_rtree_experiments
