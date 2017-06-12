// file      : build2/search.hxx -*- C++ -*-
// copyright : Copyright (c) 2014-2017 Code Synthesis Ltd
// license   : MIT; see accompanying LICENSE file

#ifndef BUILD2_SEARCH_HXX
#define BUILD2_SEARCH_HXX

#include <build2/types.hxx>
#include <build2/utility.hxx>

namespace build2
{
  class target;
  class prerequisite_key;

  // Search for an existing target in this prerequisite's scope.
  //
  const target*
  search_existing_target (const prerequisite_key&);

  // Search for an existing file in the scope's src directory. Prerequisite
  // directory should be relative.
  //
  // Originally the plan was to have a target-type specific variable that
  // contains the search paths. But there wasn't any need for this yet.
  //
  const target*
  search_existing_file (const prerequisite_key&);

  // Create a new target in this prerequisite's scope.
  //
  const target&
  create_new_target (const prerequisite_key&);
}

#endif // BUILD2_SEARCH_HXX