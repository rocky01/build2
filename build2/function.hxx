// file      : build2/function.hxx -*- C++ -*-
// copyright : Copyright (c) 2014-2017 Code Synthesis Ltd
// license   : MIT; see accompanying LICENSE file

#ifndef BUILD2_FUNCTION_HXX
#define BUILD2_FUNCTION_HXX

#include <utility>       // index_sequence
#include <type_traits>   // aligned_storage
#include <unordered_map>

#include <build2/types.hxx>
#include <build2/utility.hxx>

#include <build2/variable.hxx>
#include <build2/diagnostics.hxx>

namespace build2
{
  // Functions can be overloaded based on types of their arguments but
  // arguments can be untyped and a function can elect to accept an argument
  // of any type.
  //
  // Functions can be qualified (e.g, string.length(), path.directory()) and
  // unqualified (e.g., length(), directory()). Only functions overloaded on
  // static types can be unqualified plus they should also define a qualified
  // alias.
  //
  // Low-level function implementation would be called with a list of values
  // as arguments. There is also higher-level, more convenient support for
  // defining functions as pointers to functions (including capture-less
  // lambdas), pointers to member functions (e.g., string::size()), or
  // pointers to data members (e.g., name::type). In this case the build2
  // function types are automatically matched to C++ function types according
  // to these rules:
  //
  // T           - statically-typed (value_traits<T> must be defined)
  // names       - untyped
  // value       - any type
  // T*          - NULL-able argument (here T can be names)
  // value*      - NULL-able any type (never NULL itself, use value::null)
  // optional<T> - optional argument (here T can be T*, names, value)
  //
  // Optional arguments must be last. In case of a failure the function is
  // expected to issue diagnostics and throw failed. Note that the arguments
  // are conceptually "moved" and can be reused by the implementation.
  //
  // Normally functions come in families that share a common qualification
  // (e.g., string. or path.). The function_family class is a "registrar"
  // that simplifies handling of function families. For example:
  //
  // function_family f ("string");
  //
  // // Register length() and string.length().
  // //
  // f["length"] = &string::size;
  //
  // // Register string.max_size().
  // //
  // f[".max_size"] = []() {return string ().max_size ();};
  //
  // For more examples/ideas, study the existing function families (reside
  // in the functions-*.cxx files).
  //
  struct function_overload;

  using function_impl = value (vector_view<value>, const function_overload&);

  struct function_overload
  {
    const char* name;     // Set to point to key by insert() below.
    const char* alt_name; // Alternative name, NULL if none. This is the
                          // qualified name for unqualified or vice verse.

    // Arguments.
    //
    // A function can have a number of optional arguments. Arguments can also
    // be typed. A non-existent entry in arg_types means a value of any type.
    // A NULL entry means an untyped value.
    //
    // If arg_max equals to arg_variadic, then the function takes an unlimited
    // number of arguments. In this case the semantics of arg_min and
    // arg_types is unchanged.
    //
    static const size_t arg_variadic = size_t (~0);

    using types = vector_view<const optional<const value_type*>>;

    const size_t arg_min;
    const size_t arg_max;
    const types  arg_types;

    // Function implementation.
    //
    function_impl* const impl;

    // Auxiliary data storage. Note that it is assumed to be POD (no
    // destructors, bitwise copy, etc).
    //
    std::aligned_storage<sizeof (void*) * 3>::type data;
    static const size_t data_size = sizeof (decltype (data));

    function_overload () = default;

    function_overload (const char* an,
                       size_t mi, size_t ma, types ts,
                       function_impl* im)
        : alt_name (an),
          arg_min (mi), arg_max (ma), arg_types (move (ts)),
          impl (im) {}

    template <typename D>
    function_overload (const char* an,
                       size_t mi, size_t ma, types ts,
                       function_impl* im,
                       D d)
        : function_overload (an, mi, ma, move (ts), im)
    {
      // std::is_pod appears to be broken in VC15 and also in GCC up to
      // 5 (pointers to members).
      //
#if !((defined(_MSC_VER) && _MSC_VER <= 1911) || \
      (defined(__GNUC__) && !defined(__clang__) && __GNUC__ <= 5))
      static_assert (std::is_pod<D>::value, "type is not POD");
#endif
      static_assert (sizeof (D) <= data_size, "insufficient space");
      new (&data) D (move (d));
    }
  };

  ostream&
  operator<< (ostream&, const function_overload&); // Print signature.

  class function_map
  {
  public:
    using map_type = std::unordered_multimap<string, function_overload>;
    using iterator = map_type::iterator;
    using const_iterator = map_type::const_iterator;

    iterator
    insert (string name, function_overload);

    void
    erase (iterator i) {map_.erase (i);}

    value
    call (const string& name, vector_view<value> args, const location& l) const
    {
      return call (name, args, l, true).first;
    }

    // As above but do not fail if no match was found (but still do if the
    // match is ambiguous). Instead return an indication of whether the call
    // was made. Used to issue custom diagnostics when calling internal
    // functions.
    //
    pair<value, bool>
    try_call (const string& name,
              vector_view<value> args,
              const location& l) const
    {
      return call (name, args, l, false);
    }

    iterator
    begin () {return map_.begin ();}

    iterator
    end () {return map_.end ();}

    const_iterator
    begin () const {return map_.begin ();}

    const_iterator
    end () const {return map_.end ();}

  private:
    pair<value, bool>
    call (const string&, vector_view<value>, const location&, bool fail) const;

    map_type map_;
  };

  extern function_map functions;

  class function_family
  {
  public:
    // The default thunk catches invalid_argument and issues diagnostics
    // by assuming it is related to function arguments and contains useful
    // description.
    //
    // In order to implement a custom thunk (e.g., to catch additional extra
    // exceptions), you would normally call the default implementation.
    //
    static value
    default_thunk (vector_view<value>, const function_overload&);

    // A function family uses a common qualification (though you can pass
    // empty string to supress it). For an unqualified name (doesn't not
    // contain a dot) the qualified version is added automatically. A name
    // containing a leading dot is a shortcut notation for a qualified-only
    // name.
    //
    explicit
    function_family (string qual, function_impl* thunk = &default_thunk)
        : qual_ (qual), thunk_ (thunk) {}

    struct entry;

    entry
    operator[] (string name) const;

  private:
    const string qual_;
    function_impl* thunk_;
  };

  // Implementation details. If you can understand and explain all of this,
  // then you are hired ;-)!
  //

  template <typename T>
  struct function_arg
  {
    static const bool null = false;
    static const bool opt = false;

    static constexpr optional<const value_type*>
    type () {return &value_traits<T>::value_type;}

    static T&&
    cast (value* v)
    {
      if (v->null)
        throw invalid_argument ("null value");

      // Use fast but unchecked cast since the caller matched the types.
      //
      return move (v->as<T> ());
    }
  };

  template <>
  struct function_arg<names> // Untyped.
  {
    static const bool null = false;
    static const bool opt = false;

    static constexpr optional<const value_type*>
    type () {return nullptr;}

    static names&&
    cast (value* v)
    {
      if (v->null)
        throw invalid_argument ("null value");

      return move (v->as<names> ());
    }
  };

  template <>
  struct function_arg<value> // Anytyped.
  {
    static const bool null = false;
    static const bool opt = false;

    static constexpr optional<const value_type*>
    type () {return nullopt;}

    static value&&
    cast (value* v)
    {
      if (v->null)
        throw invalid_argument ("null value");

      return move (*v);
    }
  };

  template <typename T>
  struct function_arg<T*>: function_arg<T>
  {
    static const bool null = true;

    static T*
    cast (value* v)
    {
      if (v->null)
        return nullptr;

      // This looks bizarre but makes sense. The cast() that we are calling
      // returns an r-value reference to (what's inside) v. And it has to
      // return an r-value reference to that the value is moved into by-value
      // arguments.
      //
      T&& r (function_arg<T>::cast (v));
      return &r;
    }
  };

  template <>
  struct function_arg<value*>: function_arg<value>
  {
    static const bool null = true;

    static value*
    cast (value* v) {return v;} // NULL indicator in value::null.
  };

  template <typename T>
  struct function_arg<optional<T>>: function_arg<T>
  {
    static const bool opt = true;

    static optional<T>
    cast (value* v)
    {
      return v != nullptr ? optional<T> (function_arg<T>::cast (v)) : nullopt;
    }
  };

  // Number of optional arguments. Note that we currently don't check that
  // they are all at the end.
  //
  template <typename A0, typename... A>
  struct function_args_opt
  {
    static const size_t count = (function_arg<A0>::opt ? 1 : 0) +
      function_args_opt<A...>::count;
  };

  template <typename A0>
  struct function_args_opt<A0>
  {
    static const size_t count = (function_arg<A0>::opt ? 1 : 0);
  };

  // Argument counts/types.
  //
  template <typename... A>
  struct function_args
  {
    static const size_t max = sizeof...(A);
    static const size_t min = max - function_args_opt<A...>::count;

    // VC15 doesn't realize that a pointer to static object (in our case it is
    // &value_trair<T>::value_type) is constexpr.
    //
#if !defined(_MSC_VER) || _MSC_VER > 1910
    static constexpr const optional<const value_type*> types[max] = {
      function_arg<A>::type ()...};
#else
    static const optional<const value_type*> types[max];
#endif
  };

  template <typename... A>
#if !defined(_MSC_VER) || _MSC_VER > 1910
  constexpr const optional<const value_type*>
  function_args<A...>::types[function_args<A...>::max];
#else
  const optional<const value_type*>
  function_args<A...>::types[function_args<A...>::max] = {
    function_arg<A>::type ()...};
#endif

  // Specialization for no arguments.
  //
  template <>
  struct function_args<>
  {
    static const size_t max = 0;
    static const size_t min = 0;

#if !defined(_MSC_VER) || _MSC_VER > 1910
    static constexpr const optional<const value_type*>* types = nullptr;
#else
    static const optional<const value_type*>* const types;
#endif
  };

  // Cast data/thunk.
  //
  template <typename R, typename... A>
  struct function_cast
  {
    // A pointer to a standard layout struct is a pointer to its first data
    // member, which in our case is the cast thunk.
    //
    struct data
    {
      value (*const thunk) (vector_view<value>, const void*);
      R (*const impl) (A...);
    };

    static value
    thunk (vector_view<value> args, const void* d)
    {
      return thunk (move (args),
                    static_cast<const data*> (d)->impl,
                    std::index_sequence_for<A...> ());
    }

    template <size_t... i>
    static value
    thunk (vector_view<value> args,
           R (*impl) (A...),
           std::index_sequence<i...>)
    {
      return value (
        impl (
          function_arg<A>::cast (
            i < args.size () ? &args[i] : nullptr)...));
    }
  };

  // Specialization for void return type. In this case we return NULL value.
  //
  template <typename... A>
  struct function_cast<void, A...>
  {
    struct data
    {
      value (*const thunk) (vector_view<value>, const void*);
      void (*const impl) (A...);
    };

    static value
    thunk (vector_view<value> args, const void* d)
    {
      thunk (move (args),
             static_cast<const data*> (d)->impl,
             std::index_sequence_for<A...> ());
      return value (nullptr);
    }

    template <size_t... i>
    static void
    thunk (vector_view<value> args,
           void (*impl) (A...),
           std::index_sequence<i...>)
    {
      impl (function_arg<A>::cast (i < args.size () ? &args[i] : nullptr)...);
    }
  };

  // Customization for coerced lambdas (see below).
  //
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 6
  template <typename L, typename R, typename... A>
  struct function_cast_lamb
  {
    struct data
    {
      value (*const thunk) (vector_view<value>, const void*);
      R (L::*const impl) (A...) const;
    };

    static value
    thunk (vector_view<value> args, const void* d)
    {
      return thunk (move (args),
                    static_cast<const data*> (d)->impl,
                    std::index_sequence_for<A...> ());
    }

    template <size_t... i>
    static value
    thunk (vector_view<value> args,
           R (L::*impl) (A...) const,
           std::index_sequence<i...>)
    {
      const L* l (nullptr); // Undefined behavior.

      return value (
        (l->*impl) (
          function_arg<A>::cast (
            i < args.size () ? &args[i] : nullptr)...));
    }
  };

  template <typename L, typename... A>
  struct function_cast_lamb<L, void, A...>
  {
    struct data
    {
      value (*const thunk) (vector_view<value>, const void*);
      void (L::*const impl) (A...) const;
    };

    static value
    thunk (vector_view<value> args, const void* d)
    {
      thunk (move (args),
             static_cast<const data*> (d)->impl,
             std::index_sequence_for<A...> ());
      return value (nullptr);
    }

    template <size_t... i>
    static void
    thunk (vector_view<value> args,
           void (L::*impl) (A...) const,
           std::index_sequence<i...>)
    {
      const L* l (nullptr);
      (l->*impl) (
        function_arg<A>::cast (
          i < args.size () ? &args[i] : nullptr)...);
    }
  };
#endif

  // Customization for member functions.
  //
  template <typename R, typename T>
  struct function_cast_memf
  {
    struct data
    {
      value (*const thunk) (vector_view<value>, const void*);
      R (T::*const impl) () const;
    };

    static value
    thunk (vector_view<value> args, const void* d)
    {
      auto mf (static_cast<const data*> (d)->impl);
      return value ((function_arg<T>::cast (&args[0]).*mf) ());
    }
  };

  template <typename T>
  struct function_cast_memf<void, T>
  {
    struct data
    {
      value (*const thunk) (vector_view<value>, const void*);
      void (T::*const impl) () const;
    };

    static value
    thunk (vector_view<value> args, const void* d)
    {
      auto mf (static_cast<const data*> (d)->impl);
      (function_arg<T>::cast (args[0]).*mf) ();
      return value (nullptr);
    }
  };

  // Customization for data members.
  //
  template <typename R, typename T>
  struct function_cast_memd
  {
    struct data
    {
      value (*const thunk) (vector_view<value>, const void*);
      R T::*const impl;
    };

    static value
    thunk (vector_view<value> args, const void* d)
    {
      auto dm (static_cast<const data*> (d)->impl);
      return value (move (function_arg<T>::cast (&args[0]).*dm));
    }
  };

  struct function_family::entry
  {
    string name;
    const string& qual;
    function_impl* thunk;

    template <typename R, typename... A>
    void
    operator= (R (*impl) (A...)) &&
    {
      using args = function_args<A...>;
      using cast = function_cast<R, A...>;

      insert (move (name),
              function_overload (
                nullptr,
                args::min,
                args::max,
                function_overload::types (args::types, args::max),
                thunk,
                typename cast::data {&cast::thunk, impl}));
    }

    // Support for assigning a (capture-less) lambda.
    //
    // GCC up until version 6 has a bug (#62052) that is triggered by calling
    // a lambda that takes a by-value argument via its "decayed" function
    // pointer. To work around this we are not going to decay it and instead
    // will call its operator() on NULL pointer; yes, undefined behavior, but
    // better than a guaranteed crash.
    //
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 6
    template <typename L>
    void
    operator= (const L&) &&
    {
      move (*this).coerce_lambda (&L::operator());
    }

    template <typename L, typename R, typename... A>
    void
    coerce_lambda (R (L::*op) (A...) const) &&
    {
      using args = function_args<A...>;
      using cast = function_cast_lamb<L, R, A...>;

      insert (move (name),
              function_overload (
                nullptr,
                args::min,
                args::max,
                function_overload::types (args::types, args::max),
                thunk,
                typename cast::data {&cast::thunk, op}));
    }
#else
    template <typename L>
    void
    operator= (const L& l) &&
    {
      move (*this).operator= (decay_lambda (&L::operator(), l));
    }

    template <typename L, typename R, typename... A>
    static auto
    decay_lambda (R (L::*) (A...) const, const L& l) -> R (*) (A...)
    {
      return static_cast<R (*) (A...)> (l);
    }
#endif

    // Support for assigning a pointer to member function (e.g. an accessor).
    //
    // For now we don't support passing additional (to this) arguments though
    // we could probably do that. The issues would be the argument passing
    // semantics (e.g., what if it's const&) and the optional/default argument
    // handling.
    //
    template <typename R, typename T>
    void
    operator= (R (T::*mf) () const) &&
    {
      using args = function_args<T>;
      using cast = function_cast_memf<R, T>;

      insert (move (name),
              function_overload (
                nullptr,
                args::min,
                args::max,
                function_overload::types (args::types, args::max),
                thunk,
                typename cast::data {&cast::thunk, mf}));
    }

    // Support for assigning a pointer to data member.
    //
    template <typename R, typename T>
    void
    operator= (R T::*dm) &&
    {
      using args = function_args<T>;
      using cast = function_cast_memd<R, T>;

      insert (move (name),
              function_overload (
                nullptr,
                args::min,
                args::max,
                function_overload::types (args::types, args::max),
                thunk,
                typename cast::data {&cast::thunk, dm}));
    }

  private:
    void
    insert (string, function_overload) const;
  };

  inline auto function_family::
  operator[] (string name) const -> entry
  {
    return entry {move (name), qual_, thunk_};
  }
}

#endif // BUILD2_FUNCTION_HXX