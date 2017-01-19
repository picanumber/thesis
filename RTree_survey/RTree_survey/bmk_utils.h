#pragma once

#include <random>
#include <vector>
#include <type_traits>

#include <boost/geometry/index/rtree.hpp>


namespace utl
{
	namespace bg = boost::geometry;
	namespace bgi = bg::index;

	enum class rtree_param : int { run_time, compile_time };
	enum class rtree_split : int { linear, quadratic, rstar };
	enum class rtree_load : int { bulk, iterative };

	std::string nameof_param(rtree_param param); 
	std::string nameof_split(rtree_split split); 
	std::string nameof_load (rtree_load load); 

	template <typename T>
	struct random_generator
	{
		enum { is_signed = std::is_signed<T>::value };
		using distribution_t = std::conditional_t<std::is_integral<T>::value,
			std::uniform_int_distribution<T>, std::uniform_real_distribution<T>>; 

		using result_type = typename distribution_t::result_type;

		std::random_device                rd; 
		std::mt19937                      gen;
		std::uniform_real_distribution<T> dis;

		random_generator(T width, bool steady = true)
			: rd{}
			, gen{ steady ? 1 : rd() }
			, dis{ is_signed ? -(width / 2.) : 0, is_signed ? (width / 2.) : width }
		{
		}

		random_generator(T low, T high, bool steady = true)
			: rd{}
			, gen{ steady ? 1 : rd() }
			, dis{ low, high }
		{
		}

		result_type operator()()
		{
			return dis(gen);
		}

		random_generator(random_generator const&) = delete;
		random_generator(random_generator&&) = delete; 
		random_generator& operator=(random_generator const&) = delete;
	};

	template <std::size_t D, class coord_t>
	using point_type = bg::model::point<coord_t, D, bg::cs::cartesian>; 

	template <std::size_t D, class coord_t>
	using box_type = bg::model::box<point_type<D, coord_t>>; 

	template <std::size_t D, class coord_t, class Gen>
	std::enable_if_t<2 == D, point_type<D, coord_t>> 
		random_point(Gen& gntr)
	{
		return{ gntr(), gntr() }; 
	}

	template <std::size_t D, class coord_t, class GenC, class GenW>
	std::enable_if_t<2 == D, box_type<D, coord_t>> random_box(GenC& gnc, GenW& gnw)
	{
		auto const swd = gnw();
		auto const x   = gnc();
		auto const y   = gnc();

		return{ {x - swd, y - swd}, {x + swd, y + swd} };
	}

	template <std::size_t D, class coord_t, class Gen>
	std::enable_if_t<3 == D, point_type<D, coord_t>> random_point(Gen& gntr)
	{
		return{ gntr(), gntr(), gntr() };
	}

	template <std::size_t D, class coord_t, class GenC, class GenW>
	std::enable_if_t<3 == D, box_type<D, coord_t>> random_box(GenC& gnc, GenW& gnw)
	{
		auto const swd = gnw();
		auto const x = gnc();
		auto const y = gnc();
		auto const z = gnc();

		return{ {x - swd, y - swd, z - swd}, {x + swd, y + swd, z + swd} };
	}

	template <std::size_t D, class coord_t>
	std::vector<box_type<D, coord_t>> 
		generate_boxes(std::size_t num, std::size_t sparsity = 10)
	{
		using point_t = bg::model::point<coord_t, D, bg::cs::cartesian>; 
		using box_t   = bg::model::box<point_t>; 
		using boxes_t = std::vector<box_t>; 

		boxes_t ret; 
		ret.reserve(num); 

		coord_t minbox_sz   = static_cast<coord_t>(0.2 * sparsity);
		coord_t maxbox_sz   = static_cast<coord_t>(2 * sparsity);
		coord_t space_width = static_cast<coord_t>(num * sparsity); 

		random_generator<coord_t> box_widths{ minbox_sz, maxbox_sz, true };
		random_generator<coord_t> box_centers{ space_width, true };
		for (size_t i = 0; i < num; i++)
		{
			ret.emplace_back(random_box<D, coord_t>(box_centers, box_widths)); 
		}

		return ret; 
	}
}
