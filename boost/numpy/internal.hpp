// Copyright Jim Bosch 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_NUMPY_INTERNAL_HPP_INCLUDED
#define BOOST_NUMPY_INTERNAL_HPP_INCLUDED

/**
 *  @file boost/numpy/internal.hpp
 *  @brief Internal header file to include the Numpy C-API headers.
 *
 *  This should only be included by source files in the boost.numpy library itself.
 */
#ifndef NPY_NO_DEPRECATED_API
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#endif

#ifndef HAVE_ROUND
#define HAVE_ROUND
#endif

#include <boost/python.hpp>
#ifdef BOOST_NUMPY_INTERNAL
#define NO_IMPORT_ARRAY
#define NO_IMPORT_UFUNC
#else
#ifndef BOOST_NUMPY_INTERNAL_MAIN
ERROR_internal_hpp_is_for_internal_use_only
#endif
#endif
#define PY_ARRAY_UNIQUE_SYMBOL BOOST_NUMPY_ARRAY_API
#define PY_UFUNC_UNIQUE_SYMBOL BOOST_UFUNC_ARRAY_API
#include <numpy/arrayobject.h>
#include <numpy/ufuncobject.h>
#include <boost/numpy.hpp>

#define NUMPY_OBJECT_MANAGER_TRAITS_IMPL(pytype,manager)                \
    PyTypeObject const * object_manager_traits<manager>::get_pytype() { return &pytype; }

#endif // !BOOST_NUMPY_INTERNAL_HPP_INCLUDED
