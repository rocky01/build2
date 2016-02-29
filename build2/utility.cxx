// file      : build2/utility.cxx -*- C++ -*-
// copyright : Copyright (c) 2014-2016 Code Synthesis Ltd
// license   : MIT; see accompanying LICENSE file

#include <build2/utility>

#include <cstdlib> // strtol()

#include <build2/context>
#include <build2/variable>
#include <build2/diagnostics>

using namespace std;

namespace build2
{
  //
  // <build2/types>
  //

  ostream&
  operator<< (ostream& os, const path& p)
  {
    return os << (stream_verb (os) < 2 ? diag_relative (p) : p.string ());
  }

  ostream&
  operator<< (ostream& os, const dir_path& d)
  {
    if (stream_verb (os) < 2)
      os << diag_relative (d); // Adds trailing '/'.
    else
    {
      const string& s (d.string ());

      // Print the directory with trailing '/'.
      //
      if (!s.empty ())
        os << s << (dir_path::traits::is_separator (s.back ()) ? "" : "/");
    }

    return os;
  }

  //
  // <build2/utility>
  //

  const string empty_string;
  const path empty_path;
  const dir_path empty_dir_path;

  void
  append_options (cstrings& args, const lookup<const value>& l)
  {
    if (l)
      append_options (args, as<strings> (*l));
  }

  void
  hash_options (sha256& csum, const lookup<const value>& l)
  {
    if (l)
      hash_options (csum, as<strings> (*l));
  }

  void
  append_options (cstrings& args, const const_strings_value& sv)
  {
    if (!sv.empty ())
    {
      args.reserve (args.size () + sv.size ());

      for (const string& s: sv)
        args.push_back (s.c_str ());
    }
  }

  void
  hash_options (sha256& csum, const const_strings_value& sv)
  {
    for (const string& s: sv)
      csum.append (s);
  }

  bool
  find_option (const char* option, const lookup<const value>& l)
  {
    if (l)
    {
      for (const string& s: as<strings> (*l))
      {
        if (s == option)
          return true;
      }
    }

    return false;
  }

  unsigned int
  to_version (const string& s)
  {
    // See tests/version.
    //

    auto parse = [&s] (size_t& p, const char* m, long min = 0, long max = 99)
      -> unsigned int
    {
      if (s[p] == '-' || s[p] == '+') // stoi() allows these.
        throw invalid_argument (m);

      const char* b (s.c_str () + p);
      char* e;
      long r (strtol (b, &e, 10));

      if (b == e || r < min || r > max)
        throw invalid_argument (m);

      p = e - s.c_str ();
      return static_cast<unsigned int> (r);
    };

    auto bail = [] (const char* m) {throw invalid_argument (m);};

    unsigned int ma, mi, bf, ab (0);

    size_t p (0), n (s.size ());
    ma = parse (p, "invalid major version");

    if (p >= n || s[p] != '.')
      bail ("'.' expected after major version");

    mi = parse (++p, "invalid minor version");

    if (p >= n || s[p] != '.')
      bail ("'.' expected after minor version");

    bf = parse (++p, "invalid bugfix version");

    if (p < n)
    {
      if (s[p] != '-')
        bail ("'-' expected after bugfix version");

      char k (s[++p]);

      if (k != '\0')
      {
        if (k != 'a' && k != 'b')
          bail ("'a' or 'b' expected in release component");

        ab = parse (++p, "invalid release component", 1, 49);

        if (p != n)
          bail ("junk after release component");

        if (k == 'b')
          ab += 50;
      }
      else
        ab = 1;
    }

    //                  AABBCCDD
    unsigned int r (ma * 1000000U +
                    mi *   10000U +
                    bf *     100U);

    if (ab != 0)
    {
      if (r == 0)
        bail ("0.0.0 version with release component");

      r -= 100;
      r += ab;
    }

    return r;
  }

  bool exception_unwinding_dtor = false;
}
