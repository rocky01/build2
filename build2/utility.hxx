// file      : build2/utility.hxx -*- C++ -*-
// copyright : Copyright (c) 2014-2017 Code Synthesis Ltd
// license   : MIT; see accompanying LICENSE file

#ifndef BUILD2_UTILITY_HXX
#define BUILD2_UTILITY_HXX

#include <tuple>      // make_tuple()
#include <memory>     // make_shared()
#include <string>     // to_string()
#include <utility>    // move(), forward(), declval(), make_pair()
#include <cassert>    // assert()
#include <iterator>   // make_move_iterator()
#include <algorithm>  // *
#include <functional> // ref(), cref()

#include <libbutl/ft/lang.hxx>

#include <libbutl/utility.hxx> // combine_hash(), reverse_iterate(), case*(),
                               // etc

#include <unordered_set>

#include <build2/types.hxx>
#include <build2/b-options.hxx>
#include <build2/version.hxx>

namespace build2
{
  using std::move;
  using std::forward;
  using std::declval;

  using std::ref;
  using std::cref;

  using std::make_pair;
  using std::make_tuple;
  using std::make_shared;
  using std::make_move_iterator;
  using std::to_string;
  using std::stoul;
  using std::stoull;

  // <libbutl/utility.hxx>
  //
  using butl::reverse_iterate;
  using butl::compare_c_string;
  using butl::compare_pointer_target;
  //using butl::hash_pointer_target;
  using butl::combine_hash;
  using butl::casecmp;
  using butl::case_compare_string;
  using butl::case_compare_c_string;
  using butl::lcase;
  using butl::alpha;
  using butl::alnum;
  using butl::digit;

  using butl::exception_guard;
  using butl::make_exception_guard;

  using butl::throw_generic_error;

  // Basic string utilities.
  //

  // Trim leading/trailing whitespacec, including '\r'.
  //
  string&
  trim (string&);

  // Find the beginning and end poistions of the next word. Return the size
  // of the word or 0 and set b = e = n if there are no more words. For
  // example:
  //
  // for (size_t b (0), e (0); next_word (s, b, e); )
  // {
  //   string w (s, b, e - b);
  // }
  //
  // Or:
  //
  // for (size_t b (0), e (0), n; n = next_word (s, b, e, ' ', ','); )
  // {
  //   string w (s, b, n);
  // }
  //
  // The second version examines up to the n'th character in the string.
  //
  size_t
  next_word (const string&, size_t& b, size_t& e,
             char d1 = ' ', char d2 = '\0');

  size_t
  next_word (const string&, size_t n, size_t& b, size_t& e,
             char d1 = ' ', char d2 = '\0');

  // Command line options.
  //
  extern options ops;

  // Build system driver process path (argv0.initial is argv[0]).
  //
  extern process_path argv0;

  // Build system driver version and check.
  //
  extern const standard_version build_version;

  class location;

  void
  check_build_version (const standard_version_constraint&, const location&);

  // Work/home directories (must be initialized in main()) and relative path
  // calculation.
  //
  extern dir_path work;
  extern dir_path home;

  // By default this points to work. Setting this to something else should
  // only be done in tightly controlled, non-concurrent situations (e.g.,
  // state dump). If it is empty, then relative() below returns the original
  // path.
  //
  extern const dir_path* relative_base;

  // If possible and beneficial, translate an absolute, normalized path into
  // relative to the relative_base directory, which is normally work. Note
  // that if the passed path is the same as relative_base, then this function
  // returns empty path.
  //
  template <typename K>
  basic_path<char, K>
  relative (const basic_path<char, K>&);

  class path_target;

  path
  relative (const path_target&);

  // In addition to calling relative(), this function also uses shorter
  // notations such as '~/'. For directories the result includes the trailing
  // slash. If the path is the same as base, returns "./" if current is true
  // and empty string otherwise.
  //
  string
  diag_relative (const path&, bool current = true);

  // Basic process utilities.
  //

  // Start a process with the specified arguments printing the command at
  // verbosity level 3 and higher. Redirect STDOUT to a pipe. If error is
  // false, then redirecting STDERR to STDOUT (this can be used to suppress
  // diagnostics from the child process). Issue diagnostics and throw failed
  // in case of an error.
  //
  process_path
  run_search (const char*& args0);

  process_path
  run_search (const path&, bool init, const dir_path& fallback = dir_path ());

  process
  run_start (const process_path&, const char* args[], bool error);

  inline process
  run_start (const char* args[], bool error)
  {
    return run_start (run_search (args[0]), args, error);
  }

  bool
  run_finish (const char* args[], bool error, process&, const string&);

  // Start the process as above and then call the specified function on each
  // trimmed line of the output until it returns a non-empty object T (tested
  // with T::empty()) which is then returned to the caller.
  //
  // The predicate can move the value out of the passed string but, if error
  // is false, only in case of a "content match" (so that any diagnostics
  // lines are left intact).
  //
  // If ignore_exit is true, then the program's exist status is ignored (if it
  // is false and the program exits with the non-zero status, then an empty T
  // instance is returned).
  //
  // If checksum is not NULL, then feed it the content of each tripped line
  // (including those that come after the callback returns non-empty object).
  //
  template <typename T, typename F>
  T
  run (const process_path&,
       const char* args[],
       F&&,
       bool error = true,
       bool ignore_exit = false,
       sha256* checksum = nullptr);

  template <typename T, typename F>
  inline T
  run (const char* args[],
       F&& f,
       bool error = true,
       bool ignore_exit = false,
       sha256* checksum = nullptr)
  {
    return run<T> (
      run_search (
        args[0]), args, forward<F> (f), error, ignore_exit, checksum);
  }

  // run <prog>
  //
  template <typename T, typename F>
  inline T
  run (const path& prog,
       F&& f,
       bool error = true,
       bool ignore_exit = false,
       sha256* checksum = nullptr)
  {
    const char* args[] = {prog.string ().c_str (), nullptr};
    return run<T> (args, forward<F> (f), error, ignore_exit, checksum);
  }

  template <typename T, typename F>
  inline T
  run (const process_path& pp,
       F&& f,
       bool error = true,
       bool ignore_exit = false,
       sha256* checksum = nullptr)
  {
    const char* args[] = {pp.recall_string (), nullptr};
    return run<T> (pp, args, forward<F> (f), error, ignore_exit, checksum);
  }

  // run <prog> <arg>
  //
  template <typename T, typename F>
  inline T
  run (const path& prog,
       const char* arg,
       F&& f,
       bool error = true,
       bool ignore_exit = false,
       sha256* checksum = nullptr)
  {
    const char* args[] = {prog.string ().c_str (), arg, nullptr};
    return run<T> (args, forward<F> (f), error, ignore_exit, checksum);
  }

  template <typename T, typename F>
  inline T
  run (const process_path& pp,
       const char* arg,
       F&& f,
       bool error = true,
       bool ignore_exit = false,
       sha256* checksum = nullptr)
  {
    const char* args[] = {pp.recall_string (), arg, nullptr};
    return run<T> (pp, args, forward<F> (f), error, ignore_exit, checksum);
  }

  // Empty string and path.
  //
  extern const std::string empty_string;
  extern const path empty_path;
  extern const dir_path empty_dir_path;

  // Append all the values from a variable to the C-string list. T is either
  // target or scope. The variable is expected to be of type strings.
  //
  struct variable;

  template <typename T>
  void
  append_options (cstrings&, T&, const variable&);

  template <typename T>
  void
  append_options (cstrings&, T&, const char*);

  template <typename T>
  void
  append_options (strings&, T&, const variable&);

  template <typename T>
  void
  append_options (strings&, T&, const char*);

  template <typename T>
  void
  hash_options (sha256&, T&, const variable&);

  template <typename T>
  void
  hash_options (sha256&, T&, const char*);

  // As above but from the strings value directly.
  //
  class value;
  struct lookup;

  void
  append_options (cstrings&, const lookup&);

  void
  append_options (strings&, const lookup&);

  void
  hash_options (sha256&, const lookup&);

  void
  append_options (cstrings&, const strings&);

  void
  append_options (strings&, const strings&);

  void
  hash_options (sha256&, const strings&);

  void
  append_options (cstrings&, const strings&, size_t);

  void
  append_options (strings&, const strings&, size_t);

  void
  hash_options (sha256&, const strings&, size_t);

  // Check if a specified option is present in the variable or value. T is
  // either target or scope.
  //
  template <typename T>
  bool
  find_option (const char* option,
               T&,
               const variable&,
               bool ignore_case = false);

  template <typename T>
  bool
  find_option (const char* option,
               T&,
               const char* variable,
               bool ignore_case = false);

  bool
  find_option (const char* option, const lookup&, bool ignore_case = false);

  bool
  find_option (const char* option, const strings&, bool ignore_case = false);

  bool
  find_option (const char* option, const cstrings&, bool ignore_case = false);

  // As above but look for several options.
  //
  template <typename T>
  bool
  find_options (initializer_list<const char*>,
                T&,
                const variable&,
                bool = false);

  template <typename T>
  bool
  find_options (initializer_list<const char*>, T&, const char*, bool = false);

  bool
  find_options (initializer_list<const char*>, const lookup&, bool = false);

  bool
  find_options (initializer_list<const char*>, const strings&, bool = false);

  bool
  find_options (initializer_list<const char*>, const cstrings&, bool = false);

  // As above but look for an option that has the specified prefix.
  //
  template <typename T>
  bool
  find_option_prefix (const char* prefix, T&, const variable&, bool = false);

  template <typename T>
  bool
  find_option_prefix (const char* prefix, T&, const char*, bool = false);

  bool
  find_option_prefix (const char* prefix, const lookup&, bool = false);

  bool
  find_option_prefix (const char* prefix, const strings&, bool = false);

  bool
  find_option_prefix (const char* prefix, const cstrings&, bool = false);

  // As above but look for several option prefixes.
  //
  template <typename T>
  bool
  find_option_prefixes (initializer_list<const char*>,
                        T&,
                        const variable&,
                        bool = false);

  template <typename T>
  bool
  find_option_prefixes (initializer_list<const char*>,
                        T&,
                        const char*,
                        bool = false);

  bool
  find_option_prefixes (initializer_list<const char*>,
                        const lookup&, bool = false);

  bool
  find_option_prefixes (initializer_list<const char*>,
                        const strings&,
                        bool = false);

  bool
  find_option_prefixes (initializer_list<const char*>,
                        const cstrings&,
                        bool = false);

  // Apply the specified substitution (stem) to a '*'-pattern. If pattern
  // is NULL, then return the stem itself. Assume the pattern is valid,
  // i.e., contains a single '*' character.
  //
  string
  apply_pattern (const char* stem, const string* pattern);

  // Initialize build2 global state (verbosity, home/work directories, etc).
  // Should be called early in main() once.
  //
  void
  init (const char* argv0, uint16_t verbosity);
}

#include <build2/utility.ixx>
#include <build2/utility.txx>

#endif // BUILD2_UTILITY_HXX
