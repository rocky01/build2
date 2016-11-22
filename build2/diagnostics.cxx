// file      : build2/diagnostics.cxx -*- C++ -*-
// copyright : Copyright (c) 2014-2016 Code Synthesis Ltd
// license   : MIT; see accompanying LICENSE file

#include <build2/diagnostics>

#include <cstring>  // strchr()

using namespace std;

namespace build2
{
  // Stream verbosity.
  //
  const int stream_verb_index = ostream::xalloc ();

  void
  print_process (const char* const* args, size_t n)
  {
    diag_record r (text);
    print_process (r, args, n);
  }

  struct process_args
  {
    const char* const* a;
    size_t n;
  };

  inline static ostream&
  operator<< (ostream& o, const process_args& p)
  {
    process::print (o, p.a, p.n);
    return o;
  }

  void
  print_process (diag_record& r, const char* const* args, size_t n)
  {
    r << process_args {args, n};
  }

  // Diagnostics verbosity level.
  //
  uint16_t verb;

  // Diagnostic facility, project specifics.
  //

  void simple_prologue_base::
  operator() (const diag_record& r) const
  {
    stream_verb (r.os, sverb_);

    if (type_ != nullptr)
      r << type_ << ": ";

    if (mod_ != nullptr)
      r << mod_ << "::";

    if (name_ != nullptr)
      r << name_ << ": ";
  }

  void location_prologue_base::
  operator() (const diag_record& r) const
  {
    stream_verb (r.os, sverb_);

    r << *loc_.file << ':';

    if (!ops.no_line ())
    {
      if (loc_.line != 0)
        r << loc_.line << ':';

      if (!ops.no_column ())
      {
        if (loc_.column != 0)
          r << loc_.column << ':';
      }
    }

    r << ' ';

    if (type_ != nullptr)
      r << type_ << ": ";

    if (mod_ != nullptr)
      r << mod_ << "::";

    if (name_ != nullptr)
      r << name_ << ": ";
  }

  const basic_mark error ("error");
  const basic_mark warn  ("warning");
  const basic_mark info  ("info");
  const basic_mark text  (nullptr);
  const fail_mark  fail  ("error");
  const fail_end   endf;
}
