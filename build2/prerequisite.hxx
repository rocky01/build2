// file      : build2/prerequisite.hxx -*- C++ -*-
// copyright : Copyright (c) 2014-2017 Code Synthesis Ltd
// license   : MIT; see accompanying LICENSE file

#ifndef BUILD2_PREREQUISITE_HXX
#define BUILD2_PREREQUISITE_HXX

#include <unordered_set>

#include <build2/types.hxx>
#include <build2/utility.hxx>

#include <build2/target-key.hxx>
#include <build2/diagnostics.hxx>

namespace build2
{
  class scope;
  class target;

  // Light-weight (by being shallow-pointing) prerequisite key, similar
  // to (and based on) target key.
  //
  // Note that unlike prerequisite, the key is not (necessarily) owned by a
  // target. So for the key we instead have the base scope of the target that
  // (would) own it. Note that we assume keys to be ephemeral enough for the
  // base scope to remain unchanged.
  //
  class prerequisite_key
  {
  public:
    typedef build2::scope scope_type;

    const optional<string>& proj;
    target_key tk;                // The .dir and .out members can be relative.
    const scope_type* scope;      // Can be NULL if tk.dir is absolute.

    static const optional<string> nullproj;

    template <typename T>
    bool is_a () const {return tk.is_a<T> ();}
    bool is_a (const target_type& tt) const {return tk.is_a (tt);}
  };

  ostream&
  operator<< (ostream&, const prerequisite_key&);

  // Note that every data member except for the target is immutable (const).
  //
  class prerequisite
  {
  public:
    using scope_type = build2::scope;
    using target_type = build2::target;
    using target_type_type = build2::target_type;

    // Note that unlike targets, for prerequisites an empty out directory
    // means undetermined rather than being definitely in the out tree.
    //
    // It might seem natural to keep the reference to the owner target instead
    // of to the scope. But that's not the semantics that we have, consider:
    //
    // foo/obj{x}: bar/cxx{y}
    //
    // bar/ here is relative to the scope, not to foo/. Plus, bar/ can resolve
    // to either src or out.
    //
    const optional<string>  proj;
    const target_type_type& type;
    const dir_path dir;         // Normalized absolute or relative (to scope).
    const dir_path out;         // Empty, normalized absolute, or relative.
    const string name;
    const optional<string> ext; // Absent if unspecified.
    const scope_type& scope;

    // NULL if not yet resolved. Note that this should always be the "primary
    // target", not a member of a target group.
    //
    // While normally only a matching rule should change this, if the
    // prerequisite comes from the group, then it's possible that several
    // rules will try to update it simultaneously. Thus the atomic.
    //
    mutable atomic<const target_type*> target {nullptr};

  public:
    prerequisite (optional<string> p,
                  const target_type_type& t,
                  dir_path d,
                  dir_path o,
                  string n,
                  optional<string> e,
                  const scope_type& s)
        : proj (move (p)),
          type (t),
          dir (move (d)),
          out (move (o)),
          name (move (n)),
          ext (move (e)),
          scope (s) {}

    // Make a prerequisite from a target.
    //
    explicit
    prerequisite (const target_type&);

    // Note that the returned key "tracks" the prerequisite; that is, any
    // updates to the prerequisite's members will be reflected in the key.
    //
    prerequisite_key
    key () const
    {
      return prerequisite_key {proj, {&type, &dir, &out, &name, ext}, &scope};
    }

    // Return true if this prerequisite instance (physically) belongs to the
    // target's prerequisite list. Note that this test only works if you use
    // references to the container elements and the container hasn't been
    // resized since such a reference was obtained. Normally this function is
    // used when iterating over a combined prerequisites range to detect if
    // the prerequisite came from the group (see group_prerequisites).
    //
    bool
    belongs (const target_type&) const;

    // Prerequisite (target) type.
    //
  public:
    template <typename T>
    bool
    is_a () const {return type.is_a<T> ();}

    bool
    is_a (const target_type_type& tt) const {return type.is_a (tt);}

  public:
    prerequisite (prerequisite&& x)
        : proj (move (x.proj)),
          type (move (x.type)),
          dir (move (x.dir)),
          out (move (x.out)),
          name (move (x.name)),
          ext (move (x.ext)),
          scope (move (x.scope)),
          target (x.target.load (memory_order_relaxed)) {}

    prerequisite (const prerequisite& x, memory_order o = memory_order_consume)
        : proj (x.proj),
          type (x.type),
          dir (x.dir),
          out (x.out),
          name (x.name),
          ext (x.ext),
          scope (x.scope),
          target (x.target.load (o)) {}
  };

  inline ostream&
  operator<< (ostream& os, const prerequisite& p)
  {
    return os << p.key ();
  }

  using prerequisites = vector<prerequisite>;
}

#endif // BUILD2_PREREQUISITE_HXX