#include "spatial_index_benchmark.hpp"
#ifdef SIBENCH_RTREE_LOAD_BLK
#define BOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL 1
#endif
#include <boost/geometry/index/rtree.hpp>
// enable internal debugging utilities
#include <boost/geometry/index/detail/rtree/utilities/statistics.hpp>
#include <boost/geometry/index/detail/rtree/utilities/print.hpp>
#include <boost/tuple/tuple.hpp>
using namespace std;
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

template <typename T>
void print_statistics(std::ostream& os, std::string const& lib, T const& i)
{
	using bgi::detail::rtree::utilities::statistics;
	auto const s = statistics(i);
	os << sibench::get_banner(lib)
		<< " stats: levels=" << boost::get<0>(s)
		<< ", nodes=" << boost::get<1>(s)
		<< ", leaves=" << boost::get<2>(s)
		<< ", values=" << boost::get<3>(s)
		<< ", values_min=" << boost::get<4>(s)
		<< ", values_max=" << boost::get<5>(s)
		<< std::endl;
}

int main()
{
	try
	{
#ifdef SIBENCH_BGI_RTREE_PARAMS_CT
		std::string const lib("bgi_ct");
#elif SIBENCH_BGI_RTREE_PARAMS_RT
		std::string const lib("bgi");
#else
#error Boost.Geometry rtree parameters variant unknown
#endif

		sibench::print_result_header(std::cout, lib);

		// Generate random objects for indexing
		auto const boxes = sibench::generate_boxes(sibench::max_insertions);

#ifdef SIBENCH_BGI_RTREE_PARAMS_RT
		for (std::size_t next_capacity = 2; next_capacity <= sibench::max_capacities; ++next_capacity)
		{
			double const fill_factor = 0.5;
			std::size_t const min_capacity = next_capacity;
			std::size_t const max_capacity = std::size_t(std::floor(min_capacity / fill_factor));
#elif SIBENCH_BGI_RTREE_PARAMS_CT
			{
				std::size_t const max_capacity = 1024;
				std::size_t const min_capacity = 340;
#endif

				typedef bg::model::point<double, 2, bg::cs::cartesian> point_t;
				typedef bg::model::box<point_t> box_t;
#ifdef SIBENCH_BGI_RTREE_PARAMS_CT
#ifdef SIBENCH_RTREE_SPLIT_LINEAR
				typedef bgi::rtree<box_t, bgi::linear<max_capacity, min_capacity>> rtree_t;
#elif SIBENCH_RTREE_SPLIT_QUADRATIC
				typedef bgi::rtree<box_t, bgi::quadratic<max_capacity, min_capacity>> rtree_t;
#else
				typedef bgi::rtree<box_t, bgi::rstar<max_capacity, min_capacity>> rtree_t;
#endif

				rtree_t rtree;

#elif SIBENCH_BGI_RTREE_PARAMS_RT
#ifdef SIBENCH_RTREE_SPLIT_LINEAR
				typedef bgi::dynamic_linear rtree_parameters_t;
#elif SIBENCH_RTREE_SPLIT_QUADRATIC
				typedef bgi::dynamic_quadratic rtree_parameters_t;
#else
				typedef bgi::dynamic_rstar rtree_parameters_t;
#endif
				typedef bgi::rtree<box_t, rtree_parameters_t> rtree_t;

				rtree_parameters_t rtree_parameters(max_capacity, min_capacity);
				rtree_t rtree(rtree_parameters);

#endif // SIBENCH_BGI_RTREE_PARAMS_RT

				// accumulated results store (load, query)
				sibench::result_info load_r;
				sibench::result_info query_r;

				load_r.min_capacity = query_r.min_capacity = min_capacity;
				load_r.max_capacity = query_r.max_capacity = max_capacity;

				// Benchmark: insert
				{
#ifdef SIBENCH_RTREE_LOAD_BLK
					typedef std::vector<box_t> box_values_t;
					box_values_t vboxes;
					vboxes.reserve(boxes.size());
					for (auto const& box : boxes)
					{
						point_t p1(std::get<0>(box), std::get<1>(box));
						point_t p2(std::get<2>(box), std::get<3>(box));
						vboxes.emplace_back(p1, p2);
					}

					auto const marks = sibench::benchmark("load", 1, vboxes,
#if SIBENCH_BGI_RTREE_PARAMS_CT
						[&rtree](box_values_t const& boxes, std::size_t iterations)
					{
						rtree = rtree_t(boxes.cbegin(), boxes.cend());
#else // !SIBENCH_BGI_RTREE_PARAMS_CT
						[&rtree, &rtree_parameters](box_values_t const& boxes, std::size_t iterations)
					{
						rtree = rtree_t(boxes.cbegin(), boxes.cend(), rtree_parameters);
#endif
						::boost::ignore_unused_variable_warning(iterations);
});
#else // !SIBENCH_RTREE_LOAD_BLK
					auto const marks = sibench::benchmark("load", boxes.size(), boxes,
						[&rtree](sibench::boxes2d_t const& boxes, std::size_t iterations)
					{
						auto const s = iterations < boxes.size() ? iterations : boxes.size();
						for (size_t i = 0; i < s; ++i)
						{
							auto const& box = boxes[i];
							point_t p1(std::get<0>(box), std::get<1>(box));
							point_t p2(std::get<2>(box), std::get<3>(box));
							box_t region(p1, p2);
							rtree.insert(region);
						}
					});
#endif

					load_r.accumulate(marks);

					// debugging
					//sibench::print_result(std::cout, lib, marks);
					//std::cout <<  rtree;
					//print_statistics(std::cout, lib, rtree);
			}

				// Benchmark: query
				{
					std::size_t query_found = 0;

					auto const marks = sibench::benchmark("query", sibench::max_queries, boxes,
						[&rtree, &query_found](sibench::boxes2d_t const& boxes, std::size_t iterations)
					{
						std::vector<box_t> result;
						result.reserve(iterations);

						for (std::size_t i = 0; i < iterations; ++i)
						{
							result.clear();
							auto const& box = boxes[i];
							point_t p1(std::get<0>(box) - 10, std::get<1>(box) - 10);
							point_t p2(std::get<2>(box) + 10, std::get<3>(box) + 10);
							box_t region(p1, p2);
							rtree.query(bgi::intersects(region), std::back_inserter(result));
							query_found += result.size();
						}
					});

					query_r.accumulate(marks);

					// debugging
					//sibench::print_result(std::cout, lib, marks);
					//sibench::print_query_count(std::cout, lib, query_found);
				}

				// single line per run
				sibench::print_result(std::cout, lib, load_r, query_r);

		} // for capacity

			return EXIT_SUCCESS;
	}
	catch (std::exception const& e)
	{
		std::cerr << e.what() << std::endl;
	}
	catch (...)
	{
		std::cerr << "unknown error" << std::endl;
	}
	return EXIT_FAILURE;
}


