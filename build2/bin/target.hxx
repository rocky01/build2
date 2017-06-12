// file      : build2/bin/target.hxx -*- C++ -*-
// copyright : Copyright (c) 2014-2017 Code Synthesis Ltd
// license   : MIT; see accompanying LICENSE file

#ifndef BUILD2_BIN_TARGET_HXX
#define BUILD2_BIN_TARGET_HXX

#include <build2/types.hxx>
#include <build2/utility.hxx>

#include <build2/target.hxx>

namespace build2
{
  namespace bin
  {
    // The obj{} target group.
    //
    class obje: public file
    {
    public:
      using file::file;

    public:
      static const target_type static_type;
      virtual const target_type& dynamic_type () const {return static_type;}
    };

    class obja: public file
    {
    public:
      using file::file;

    public:
      static const target_type static_type;
      virtual const target_type& dynamic_type () const {return static_type;}
    };

    class objs: public file
    {
    public:
      using file::file;

    public:
      static const target_type static_type;
      virtual const target_type& dynamic_type () const {return static_type;}
    };

    class obj: public target
    {
    public:
      using target::target;

    public:
      static const target_type static_type;
      virtual const target_type& dynamic_type () const {return static_type;}
    };

    // Binary module interface.
    //
    // While currently there are only C++ modules, if things pan out, chances
    // are we will have C (or Obj-C) modules. And in that case it is plausible
    // we will also have some binutils to examine BMIs, similar to objdump,
    // etc. So that's why this target type is in bin and not cxx.
    //
    // bmi*{} is similar to obj*{} though the semantics is a bit different:
    // the idea is that we should try hard to re-use a single bmiX{} file for
    // an entire "build" but if that's not possible (because the compilation
    // options are too different), then compile a private version for
    // ourselves (the definition of "too different" is, of course, compiler-
    // specific).
    //
    // When we compile a module interface unit, we end up with bmi*{} and
    // obj*{}. How that obj*{} is produced is compiler-dependent. While it
    // makes sense to decouple the production of the two in order to increase
    // parallelism, doing so will further complicate the already hairy
    // organization. So, at least for now, we produce the two at the same time
    // and make obj*{} an ad hoc member of bmi*{}.
    //
    class bmie: public file
    {
    public:
      using file::file;

    public:
      static const target_type static_type;
      virtual const target_type& dynamic_type () const {return static_type;}
    };

    class bmia: public file
    {
    public:
      using file::file;

    public:
      static const target_type static_type;
      virtual const target_type& dynamic_type () const {return static_type;}
    };

    class bmis: public file
    {
    public:
      using file::file;

    public:
      static const target_type static_type;
      virtual const target_type& dynamic_type () const {return static_type;}
    };

    class bmi: public target
    {
    public:
      using target::target;

    public:
      static const target_type static_type;
      virtual const target_type& dynamic_type () const {return static_type;}
    };

    // The lib{} target group.
    //
    class liba: public file
    {
    public:
      using file::file;

    public:
      static const target_type static_type;
      virtual const target_type& dynamic_type () const {return static_type;}
    };

    class libs: public file
    {
    public:
      using file::file;

    public:
      static const target_type static_type;

      virtual const target_type&
      dynamic_type () const override {return static_type;}
    };

    // Standard layout type compatible with group_view's const target*[2].
    //
    struct lib_members
    {
      const liba* a = nullptr;
      const libs* s = nullptr;
    };

    class lib: public target, public lib_members
    {
    public:
      using target::target;

      virtual group_view
      group_members (action_type) const override;

    public:
      static const target_type static_type;

      virtual const target_type&
      dynamic_type () const override {return static_type;}
    };

    // Windows import library.
    //
    class libi: public file
    {
    public:
      using file::file;

    public:
      static const target_type static_type;
      virtual const target_type& dynamic_type () const {return static_type;}
    };
  }
}

#endif // BUILD2_BIN_TARGET_HXX