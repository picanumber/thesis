#include "bmk_utils.h"

namespace utl
{
	std::string nameof_param(rtree_param param)
	{
		switch (param)
		{
		case utl::rtree_param::run_time:
			return "run_time"; 
		case utl::rtree_param::compile_time:
			return "compile_time"; 
		}
		return{};
	}
	
	std::string nameof_split(rtree_split split)
	{
		switch (split)
		{
		case utl::rtree_split::linear:
			return "linear"; 
		case utl::rtree_split::quadratic:
			return "quadratic"; 
		case utl::rtree_split::rstar:
			return "rstar"; 
		case utl::rtree_split::bulk:
			return "bulk"; 
		}
		return{};
	}

	std::ostream& operator<<(std::ostream& os, rtree_param param)
	{
		os << nameof_param(param); 
		return os; 
	}

	std::ostream& operator<<(std::ostream& os, rtree_split split)
	{
		os << nameof_split(split); 
		return os;
	}

	std::string get_info_header(rtree_param param, rtree_split split)
	{
		return nameof_param(param) + " | " + nameof_split(split); 
	}
}


// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// SHAPELIB example

// Shapelib is a well-known and often used library to read (and write) shapefiles by Frank Warmerdam

// To build and run this example:
// 1) download shapelib from http://shapelib.maptools.org/
// 2) extract and put the source "shpopen.cpp" in project or makefile
// 3) download a shapefile, for example world countries from http://aprsworld.net/gisdata/world
// Alternativelly, install Shapelib using OSGeo4W installer from http://trac.osgeo.org/osgeo4w/
// that provides Windows binary packages

#include "shapefil.h"

#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

using namespace boost::geometry;

template <typename T, typename F>
void read_shapefile(const std::string& filename, std::vector<T>& polygons, F functor)
{
	try
	{
		SHPHandle handle = SHPOpen(filename.c_str(), "rb");
		if (handle <= 0)
		{
			throw std::string("File " + filename + " not found");
		}

		int nShapeType, nEntities;
		double adfMinBound[4], adfMaxBound[4];
		SHPGetInfo(handle, &nEntities, &nShapeType, adfMinBound, adfMaxBound);

		//std::cout << "Number of entities " << nEntities << std::endl;
		for (int i = 0; i < nEntities; i++)
		{
			SHPObject* psShape = SHPReadObject(handle, i);

			// Read only polygons, and only those without holes
			if (psShape->nSHPType == SHPT_POLYGON && psShape->nParts == 1)
			{
				T polygon;
				functor(psShape, polygon);
				polygons.push_back(polygon);
			}
			SHPDestroyObject(psShape);
		}
		SHPClose(handle);
	}
	catch (const std::string& s)
	{
		throw s;
	}
	catch (...)
	{
		throw std::string("Other exception");
	}
}


template <typename T>
void convert(SHPObject* psShape, T& polygon)
{
	double* x = psShape->padfX;
	double* y = psShape->padfY;
	for (int v = 0; v < psShape->nVertices; v++)
	{
		typename point_type<T>::type point;
		assign_values(point, x[v], y[v]);
		append(polygon, point);
	}
}


std::vector<utl::box_type<2, double>> utl::read_from_shapefile(std::string const& filename)
{
	std::vector<utl::box_type<2, double>> boxes; 
	
	typedef model::polygon<model::d2::point_xy<double> > polygon_2d;
	std::vector<polygon_2d> polygons;

	try
	{
		read_shapefile(filename, polygons, convert<polygon_2d>);
	}
	catch (const std::string& s)
	{
		std::cout << s << std::endl;
		return boxes;
	}

	utl::box_type<2, double> cur_box; 
	boxes.reserve(polygons.size()); 
	for (auto const& pgon : polygons)
	{
		bg::envelope(pgon, cur_box); 
		boxes.push_back(cur_box); 
	}

	return boxes;
}

