// Copyright Jim Bosch 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_NUMPY_INTERNAL
#include <boost/numpy/internal.hpp>

#define DTYPE_FROM_CODE(code) \
    dtype(python::detail::new_reference(reinterpret_cast<PyObject*>(PyArray_DescrFromType(code))))

#define BUILTIN_INT_DTYPE(bits)                                         \
    template <> struct builtin_int_dtype< bits, false > {               \
        static dtype get() { return DTYPE_FROM_CODE(NPY_INT ## bits); } \
    };                                                                  \
    template <> struct builtin_int_dtype< bits, true > {                \
        static dtype get() { return DTYPE_FROM_CODE(NPY_UINT ## bits); } \
    };                                                                  \
    template dtype get_int_dtype< bits, false >();                      \
    template dtype get_int_dtype< bits, true >()

#define BUILTIN_FLOAT_DTYPE(bits)                                       \
    template <> struct builtin_float_dtype< bits > {                    \
        static dtype get() { return DTYPE_FROM_CODE(NPY_FLOAT ## bits); } \
    };                                                                  \
    template dtype get_float_dtype< bits >()

#define BUILTIN_COMPLEX_DTYPE(bits)                                     \
    template <> struct builtin_complex_dtype< bits > {                  \
        static dtype get() { return DTYPE_FROM_CODE(NPY_COMPLEX ## bits); } \
    };                                                                  \
    template dtype get_complex_dtype< bits >()

namespace boost { namespace python { namespace converter {
NUMPY_OBJECT_MANAGER_TRAITS_IMPL(PyArrayDescr_Type, numpy::dtype)
}}} // namespace boost::python::converter

namespace boost { namespace numpy {

namespace detail {

dtype builtin_dtype<bool,true>::get() { return DTYPE_FROM_CODE(NPY_BOOL); }

template <int bits, bool isUnsigned> struct builtin_int_dtype;
template <int bits> struct builtin_float_dtype;
template <int bits> struct builtin_complex_dtype;

template <int bits, bool isUnsigned> dtype get_int_dtype() {
    return builtin_int_dtype<bits,isUnsigned>::get();
}
template <int bits> dtype get_float_dtype() { return builtin_float_dtype<bits>::get(); }
template <int bits> dtype get_complex_dtype() { return builtin_complex_dtype<bits>::get(); }

BUILTIN_INT_DTYPE(8);
BUILTIN_INT_DTYPE(16);
BUILTIN_INT_DTYPE(32);
BUILTIN_INT_DTYPE(64);
BUILTIN_FLOAT_DTYPE(32);
BUILTIN_FLOAT_DTYPE(64);
BUILTIN_COMPLEX_DTYPE(64);
BUILTIN_COMPLEX_DTYPE(128);
#if NPY_BITSOF_LONGDOUBLE > NPY_BITSOF_DOUBLE
template <> struct builtin_float_dtype< NPY_BITSOF_LONGDOUBLE > {
    static dtype get() { return DTYPE_FROM_CODE(NPY_LONGDOUBLE); }
};
template dtype get_float_dtype< NPY_BITSOF_LONGDOUBLE >();
template <> struct builtin_complex_dtype< 2 * NPY_BITSOF_LONGDOUBLE > {
    static dtype get() { return DTYPE_FROM_CODE(NPY_CLONGDOUBLE); }
};
template dtype get_complex_dtype< 2 * NPY_BITSOF_LONGDOUBLE >();
#endif

} // namespace detail

python::detail::new_reference dtype::convert(python::object const & arg, bool align) {
  PyArray_Descr* obj=NULL;
  if (align) {
    if (PyArray_DescrAlignConverter(arg.ptr(), &obj) < 0)
      python::throw_error_already_set();
  } else {
    if (PyArray_DescrConverter(arg.ptr(), &obj) < 0)
      python::throw_error_already_set();
  }
  return python::detail::new_reference(reinterpret_cast<PyObject*>(obj));
}

int dtype::get_itemsize() const { return reinterpret_cast<PyArray_Descr*>(ptr())->elsize;}

bool equivalent(dtype const & a, dtype const & b) {
    return PyArray_EquivTypes(
        reinterpret_cast<PyArray_Descr*>(a.ptr()),
        reinterpret_cast<PyArray_Descr*>(b.ptr())
    );
}

namespace {

namespace pyconv = boost::python::converter;

template <typename T>
class array_scalar_converter {
public:

  static PyTypeObject const * get_pytype() {
	// This implementation depends on the fact that get_builtin returns pointers to objects
	// NumPy has declared statically, and that the typeobj member also refers to a static
	// object.  That means we don't need to do any reference counting.
	// In fact, I'm somewhat concerned that increasing the reference count of any of these
	// might cause leaks, because I don't think Boost.Python ever decrements it, but it's
	// probably a moot point if everything is actually static.
	return reinterpret_cast<PyArray_Descr*>(dtype::get_builtin<T>().ptr())->typeobj;
  }

  static void * convertible(PyObject * obj) {
	if (obj->ob_type == get_pytype()) {
	  return obj;
    } else if (boost::is_integral<T>::value) {
      // allow int/long or long/longlong conversions, if one of those pairs has the same size
      if (boost::is_unsigned<T>::value) {
        if (sizeof(T) == sizeof(unsigned int) && obj->ob_type
            == array_scalar_converter<unsigned int>::get_pytype()) {
          return obj;
        }
        if (sizeof(T) == sizeof(unsigned long) && obj->ob_type
            == array_scalar_converter<unsigned long>::get_pytype()) {
          return obj;
        }
        if (sizeof(T) == sizeof(unsigned long long) && obj->ob_type 
            == array_scalar_converter<unsigned long long>::get_pytype()) {
          return obj;
        }
        return 0;
      } else {
        if (sizeof(T) == sizeof(int)
            && obj->ob_type == array_scalar_converter<int>::get_pytype()) {
          return obj;
        }
        if (sizeof(T) == sizeof(long)
            && obj->ob_type == array_scalar_converter<long>::get_pytype()) {
          return obj;
        }
        if (sizeof(T) == sizeof(long long)
            && obj->ob_type == array_scalar_converter<long long>::get_pytype()) {
          return obj;
        }
      }
      return 0;
    } else { 
	  return 0;
	}
  }

  static void convert(PyObject * obj, pyconv::rvalue_from_python_stage1_data* data) {
	void * storage = reinterpret_cast<pyconv::rvalue_from_python_storage<T>*>(data)->storage.bytes;
	// We assume std::complex is a "standard layout" here and elsewhere; not guaranteed by
	// C++03 standard, but true in every known implementation (and guaranteed by C++11).
	PyArray_ScalarAsCtype(obj, reinterpret_cast<T*>(storage));
	data->convertible = storage;
  }

  static void declare() {
	pyconv::registry::push_back(
	  &convertible, &convert, python::type_id<T>()
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
	  , &get_pytype
#endif
	);
  }

};

} // anonymous

void dtype::register_scalar_converters() {
  array_scalar_converter<bool>::declare();
  array_scalar_converter<npy_uint8>::declare();
  array_scalar_converter<npy_int8>::declare();
  array_scalar_converter<npy_uint16>::declare();
  array_scalar_converter<npy_int16>::declare();
  array_scalar_converter<npy_uint32>::declare();
  array_scalar_converter<npy_int32>::declare();
  array_scalar_converter<npy_uint64>::declare();
  array_scalar_converter<npy_int64>::declare();
  array_scalar_converter<float>::declare();
  array_scalar_converter<double>::declare();
  array_scalar_converter< std::complex<float> >::declare();
  array_scalar_converter< std::complex<double> >::declare();
#if NPY_BITSOF_LONGDOUBLE > NPY_BITSOF_DOUBLE
  array_scalar_converter<long double>::declare();
  array_scalar_converter< std::complex<long double> >::declare();
#endif
}

} // namespace boost::numpy
} // namespace boost
