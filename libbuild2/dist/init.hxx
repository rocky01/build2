// file      : libbuild2/dist/init.hxx -*- C++ -*-
// copyright : Copyright (c) 2014-2019 Code Synthesis Ltd
// license   : MIT; see accompanying LICENSE file

#ifndef LIBBUILD2_DIST_INIT_HXX
#define LIBBUILD2_DIST_INIT_HXX

#include <libbuild2/types.hxx>
#include <libbuild2/utility.hxx>

#include <libbuild2/module.hxx>

#include <libbuild2/export.hxx>

namespace build2
{
  namespace dist
  {
    bool
    boot (scope&, const location&, unique_ptr<module_base>&);

    bool
    init (scope&,
          scope&,
          const location&,
          unique_ptr<module_base>&,
          bool,
          bool,
          const variable_map&);

    extern "C" LIBBUILD2_SYMEXPORT const module_functions*
    build2_dist_load ();
  }
}

#endif // LIBBUILD2_DIST_INIT_HXX