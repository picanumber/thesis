#pragma once

#include <tuple>
#include <random>
#include <vector>
#include <locale>
#include <iomanip>
#include <iostream>
#include <type_traits>
#include <boost/geometry/index/rtree.hpp>

namespace utl
{
	namespace bg = boost::geometry;
	namespace bgi = bg::index;

	// --------------------------------------------------------------------------------------------
	// --------------------------------------------------------------- constants & related printing
	enum class rtree_param : int { run_time, compile_time };
	enum class rtree_split : int { linear, quadratic, rstar, bulk };
	enum class input_maker : int { from_file, rand_gen }; 

	std::string nameof_param(rtree_param param);
	std::string nameof_split(rtree_split split);

	std::ostream& operator<<(std::ostream& os, rtree_param param);
	std::ostream& operator<<(std::ostream& os, rtree_split split);

	std::string get_info_header(rtree_param param, rtree_split split);

	class split_every_three : public std::numpunct<char>
	{
	protected:
		virtual char do_thousands_sep() const 
		{ 
			return '\''; 
		}
		virtual std::string do_grouping() const 
		{ 
			return "\03"; 
		}
	};

	template <class Num>
	std::string to_short_string(Num num, std::streamsize sz = 4)
	{
		std::stringstream ss; 
		
		if (num >= 1'000'000)
		{
			ss << std::setprecision(sz) << (num / 1'000'000.) << 'M'; 
		}
		else if (num >= 1'000)
		{
			ss << std::setprecision(sz) << (num / 1'000.) << 'K';
		}
		else
		{
			ss << std::setprecision(sz) << num; 
		}

		return ss.str(); 
	}

	// --------------------------------------------------------------------------------------------
	// ----------------------------------------------------------------------------- type utilities
	template <class Box> struct inner_pt;

	template <class coord_t, std::size_t D>
	struct inner_pt<bg::model::box<bg::model::point<coord_t, D, bg::cs::cartesian>>>
	{
		using type = bg::model::point<coord_t, D, bg::cs::cartesian>;

		constexpr static std::size_t dim = D; 
		using coord_type = coord_t; 
	};

	template <class B>
	using inner_pt_t = typename inner_pt<B>::type;

	template <std::size_t D, class coord_t>
	using point_type = bg::model::point<coord_t, D, bg::cs::cartesian>;

	template <std::size_t D, class coord_t>
	using box_type = bg::model::box<point_type<D, coord_t>>;

	namespace detail
	{
		template <class split_t>
		struct has_dynamic_params
		{
			enum {
				value =
				std::is_same<split_t, bgi::dynamic_linear>::value ||
				std::is_same<split_t, bgi::dynamic_quadratic>::value ||
				std::is_same<split_t, bgi::dynamic_rstar>::value
			};
		};
	}

	template <class>
	struct has_dynamic_params; 

	template <
		template <class...> class rtree_t, 
		class box_t, 
		class split_t, 
		class... Rest
	>
	struct has_dynamic_params<rtree_t<box_t, split_t, Rest...>> 
		: detail::has_dynamic_params<split_t>
	{
		using box_type = box_t; 
		using split_type = split_t; 
	};

	template <class rtree_t>
	using rtree_split_t = typename has_dynamic_params<rtree_t>::split_type; 

	template <class rtree_t>
	using rtree_box_t = typename has_dynamic_params<rtree_t>::box_type;

	// --------------------------------------------------------------------------------------------
	// ------------------------------------------------------------------------ geometric utilities
	template <unsigned K, class Geometry>
	auto knn(Geometry const& g) -> decltype(bgi::nearest(g, K))
	{
		return bgi::nearest(g, K); 
	}

	// --------------------------------------------------------------------------------------------
	// ------------------------------------------------------------------------------------- makers
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

	// --------------------------------------------------------------------------------------------
	// -------------------------------------------------------------------------- generic utilities
	namespace detail
	{
		template <std::size_t I, class Out, class Tuple, class S1, class...Sn>
		void impl_cartesian_product(Out& range, Tuple& val, S1&& s1, Sn&&... seqs)
		{
			for (auto&& elem : s1)
			{
				std::get<I>(val) = elem;
				impl_cartesian_product<I + 1>(range, val, std::forward<Sn>(seqs)...);
			}
		}

		template <std::size_t I, class Out, class Tuple>
		void impl_cartesian_product(Out& range, Tuple& val)
		{
			range.push_back(val);
		}
	}

	template <class...S>
	auto cartesian_product(S&&... sequences)
	{
		using product_t = std::tuple<typename std::decay_t<S>::value_type...>;

		product_t vessel;
		std::vector<product_t> vt;

		detail::impl_cartesian_product<0>(vt, vessel, std::forward<S>(sequences)...);

		return vt;
	}

	namespace detail
	{
		template <class T, std::size_t... Is>
		void print_tuple_impl(std::ostream& stream, T&& t, std::index_sequence<Is...>&&)
		{
			using swallow = int[];
			(void)swallow {
				0, (void(
					stream << (Is == 0 ? "" : ", ") << std::get<Is>(std::forward<T>(t))), 0)...
			};
		}
	}

	template <class...Ts>
	void print_tuple(std::ostream& stream, std::tuple<Ts...>& tup)
	{
		stream << "{ ";
		detail::print_tuple_impl(stream, tup, std::index_sequence_for<Ts...>{});
		stream << " }\n";
	}

	// --------------------------------------------------------------------------------------------
	// --------------------------------------------------------------------- random data generation
	namespace detail
	{
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
	} // ~ detail

	template <std::size_t D, class coord_t, class Gen>
	std::enable_if_t<2 == D, point_type<D, coord_t>> 
		random_point(Gen& gntr)
	{
		return{ gntr(), gntr() };
	}

	template <std::size_t D, class coord_t, class GenC, class GenW>
	std::enable_if_t<2 == D, box_type<D, coord_t>> 
		random_box(GenC& gnc, GenW& gnw)
	{
		auto const swd = gnw();
		auto const x = gnc();
		auto const y = gnc();

		return{ {x - swd, y - swd}, {x + swd, y + swd} };
	}

	template <std::size_t D, class coord_t, class Factor> 
	std::enable_if_t<2 == D, box_type<D, coord_t>>
		bloat_box(box_type<D, coord_t> const& b, Factor fa)
	{
		return{
			{ bg::get<bg::min_corner, 0>(b) - fa, bg::get<bg::min_corner, 1>(b) - fa },
			{ bg::get<bg::max_corner, 0>(b) + fa, bg::get<bg::max_corner, 1>(b) + fa } };
	}

	template <std::size_t D, class coord_t, class Gen> 
	std::enable_if_t<3 == D, point_type<D, coord_t>>
		random_point(Gen& gntr)
	{
		return{ gntr(), gntr(), gntr() };
	}

	template <std::size_t D, class coord_t, class GenC, class GenW>
	std::enable_if_t<3 == D, box_type<D, coord_t>> 
		random_box(GenC& gnc, GenW& gnw)
	{
		auto const swd = gnw();
		auto const x   = gnc();
		auto const y   = gnc();
		auto const z   = gnc();

		return{ {x - swd, y - swd, z - swd}, {x + swd, y + swd, z + swd} };
	}

	template <std::size_t D, class coord_t, class Factor>
	std::enable_if_t<3 == D, box_type<D, coord_t>> 
		bloat_box(box_type<D, coord_t> const& b, Factor fa)
	{
		return{{ 
				bg::get<bg::min_corner, 0>(b) - fa, 
				bg::get<bg::min_corner, 1>(b) - fa, 
				bg::get<bg::min_corner, 2>(b) - fa 
			}, 
			{ 
				bg::get<bg::max_corner, 0>(b) + fa, 
				bg::get<bg::max_corner, 1>(b) + fa, 
				bg::get<bg::max_corner, 2>(b) + fa 
			}};
	}

	template <std::size_t D, class coord_t>
	std::vector<box_type<D, coord_t>>
		generate_boxes(std::size_t num, std::size_t sparsity = 10)
	{
		using point_t = bg::model::point<coord_t, D, bg::cs::cartesian>;
		using box_t = bg::model::box<point_t>;
		using boxes_t = std::vector<box_t>;

		boxes_t ret;
		ret.reserve(num);

		coord_t minbox_sz = static_cast<coord_t>(0.2 * sparsity);
		coord_t maxbox_sz = static_cast<coord_t>(2 * sparsity);
		coord_t space_width = static_cast<coord_t>(num * sparsity);

		detail::random_generator<coord_t> box_widths{ minbox_sz, maxbox_sz, true };
		detail::random_generator<coord_t> box_centers{ space_width, true };
		for (size_t i = 0; i < num; i++)
		{
			ret.emplace_back(random_box<D, coord_t>(box_centers, box_widths));
		}

		return ret;
	}

	// --------------------------------------------------------------------------------------------
	// -------------------------------------------------------------------------- shapefile reading
	std::vector<box_type<2, double>> read_from_shapefile(std::string const& filename); 
} // ~utl
