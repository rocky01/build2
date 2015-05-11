// file      : build/cxx/target.cxx -*- C++ -*-
// copyright : Copyright (c) 2014-2015 Code Synthesis Ltd
// license   : MIT; see accompanying LICENSE file

#include <build/cxx/target>

using namespace std;

namespace build
{
  namespace cxx
  {
    const target_type hxx::static_type
    {
      typeid (hxx),
      "hxx",
      &file::static_type,
      &target_factory<hxx>,
      file::static_type.search
    };

    const target_type ixx::static_type
    {
      typeid (ixx),
      "ixx",
      &file::static_type,
      &target_factory<ixx>,
      file::static_type.search
    };

    const target_type txx::static_type
    {
      typeid (txx),
      "txx",
      &file::static_type,
      &target_factory<txx>,
      file::static_type.search
    };

    const target_type cxx::static_type
    {
      typeid (cxx),
      "cxx",
      &file::static_type,
      &target_factory<cxx>,
      file::static_type.search
    };

    const target_type h::static_type
    {
      typeid (h),
      "h",
      &file::static_type,
      &target_factory<h>,
      file::static_type.search
    };

    const target_type c::static_type
    {
      typeid (c),
      "c",
      &file::static_type,
      &target_factory<c>,
      file::static_type.search
    };
  }
}
