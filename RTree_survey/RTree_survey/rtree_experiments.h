#pragma once

#include <type_traits>
#include "bmk_utils.h"

namespace boost_rtree_experiments
{
	using namespace utl; 

	template <class rtree_t, class boxes_t>
	struct load_experiment
	{
		boxes_t const& _boxes;
		rtree_split const _split;
		std::size_t const _fixedSz; 

		load_experiment(boxes_t const& boxes, rtree_split split, std::size_t fixedSz = 0)
			: _boxes(boxes)
			, _split(split)
			, _fixedSz(fixedSz)
		{
		}

		template <class Sz>
		std::enable_if_t<!has_dynamic_params<rtree_t>::value && std::is_integral<Sz>::value>
		operator()(Sz numElems)
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

		template <class Sz>
		std::enable_if_t<has_dynamic_params<rtree_t>::value && std::is_integral<Sz>::value>
			operator()(Sz max_capacity)
		{
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
			}
			break;
			}

			assert(_fixedSz == ni); // also prevents from optimizing away the temp trees
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

		query_experiment(boxes_t const& boxes, std::size_t numqs)
			: _rtree(nullptr)
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
			assert(!_rtree); 
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

