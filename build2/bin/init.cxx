// file      : build2/bin/init.cxx -*- C++ -*-
// copyright : Copyright (c) 2014-2016 Code Synthesis Ltd
// license   : MIT; see accompanying LICENSE file

#include <build2/bin/init>

#include <butl/triplet>

#include <build2/scope>
#include <build2/variable>
#include <build2/diagnostics>

#include <build2/config/utility>
#include <build2/install/utility>

#include <build2/bin/rule>
#include <build2/bin/guess>
#include <build2/bin/target>

using namespace std;
using namespace butl;

namespace build2
{
  namespace bin
  {
    static obj_rule obj_;
    static lib_rule lib_;

    // Default config.bin.*.lib values.
    //
    static const strings exe_lib {"shared", "static"};
    static const strings liba_lib {"static"};
    static const strings libs_lib {"shared"};

    bool
    config_init (scope& r,
                 scope& b,
                 const location& loc,
                 unique_ptr<module_base>&,
                 bool first,
                 bool,
                 const variable_map& hints)
    {
      tracer trace ("bin::config_init");
      l5 ([&]{trace << "for " << b.out_path ();});

      // Enter variables.
      //
      if (first)
      {
        auto& v (var_pool);

        // Note: some overridable, some not.
        //
        v.insert<string>    ("config.bin.target",   true);
        v.insert<string>    ("config.bin.pattern",  true);

        v.insert<string>    ("config.bin.lib",      true);
        v.insert<strings>   ("config.bin.exe.lib",  true);
        v.insert<strings>   ("config.bin.liba.lib", true);
        v.insert<strings>   ("config.bin.libs.lib", true);
        v.insert<dir_paths> ("config.bin.rpath",    true);

        v.insert<string>    ("config.bin.lib.prefix", true);
        v.insert<string>    ("config.bin.lib.suffix", true);
        v.insert<string>    ("config.bin.exe.prefix", true);
        v.insert<string>    ("config.bin.exe.suffix", true);

        v.insert<string>    ("bin.lib");
        v.insert<strings>   ("bin.exe.lib");
        v.insert<strings>   ("bin.liba.lib");
        v.insert<strings>   ("bin.libs.lib");
        v.insert<dir_paths> ("bin.rpath");

        v.insert<string>    ("bin.lib.prefix");
        v.insert<string>    ("bin.lib.suffix");
        v.insert<string>    ("bin.exe.prefix");
        v.insert<string>    ("bin.exe.suffix");
      }

      // Configure.
      //
      using config::required;
      using config::optional;
      using config::omitted;

      // Adjust module priority (binutils).
      //
      config::save_module (r, "bin", 350);

      // The idea here is as follows: if we already have one of
      // the bin.* variables set, then we assume this is static
      // project configuration and don't bother setting the
      // corresponding config.bin.* variable.
      //
      //@@ Need to validate the values. Would be more efficient
      //   to do it once on assignment than every time on query.
      //   Custom var type?
      //

      // config.bin.lib
      //
      {
        value& v (b.assign ("bin.lib"));
        if (!v)
          v = required (r, "config.bin.lib", "both").first;
      }

      // config.bin.exe.lib
      //
      {
        value& v (b.assign ("bin.exe.lib"));
        if (!v)
          v = required (r, "config.bin.exe.lib", exe_lib).first;
      }

      // config.bin.liba.lib
      //
      {
        value& v (b.assign ("bin.liba.lib"));
        if (!v)
          v = required (r, "config.bin.liba.lib", liba_lib).first;
      }

      // config.bin.libs.lib
      //
      {
        value& v (b.assign ("bin.libs.lib"));
        if (!v)
          v = required (r, "config.bin.libs.lib", libs_lib).first;
      }

      // config.bin.rpath
      //
      // This one is optional and we merge it into bin.rpath, if any.
      // See the cxx module for details on merging.
      //
      b.assign ("bin.rpath") += cast_null<dir_paths> (
        optional (r, "config.bin.rpath"));

      // config.bin.{lib,exe}.{prefix,suffix}
      //
      // These ones are not used very often so we will omit them from the
      // config.build if not specified. We also override any existing value
      // that might have been specified before loading the module.
      //
      if (const value* v = omitted (r, "config.bin.lib.prefix").first)
        b.assign ("bin.lib.prefix") = *v;

      if (const value* v = omitted (r, "config.bin.lib.suffix").first)
        b.assign ("bin.lib.suffix") = *v;

      if (const value* v = omitted (r, "config.bin.exe.prefix").first)
        b.assign ("bin.exe.prefix") = *v;

      if (const value* v = omitted (r, "config.bin.exe.suffix").first)
        b.assign ("bin.exe.suffix") = *v;

      if (first)
      {
        bool new_val (false); // Set any new values?

        // config.bin.target
        //
        {
          const variable& var (var_pool.find ("config.bin.target"));

          // We first see if the value was specified via the configuration
          // mechanism.
          //
          auto p (omitted (r, var));
          const value* v (p.first);

          // Then see if there is a config hint (e.g., from the C++ module).
          //
          bool hint (false);
          if (v == nullptr)
          {
            if (auto l = hints[var])
            {
              v = l.value;
              hint = true;
            }
          }

          if (v == nullptr)
            fail (loc) << "unable to determine binutils target" <<
              info << "consider specifying it with " << var.name <<
              info << "or first load a module that can provide it as a hint, "
                       << "such as c or cxx";

          // Split/canonicalize the target.
          //
          string s (cast<string> (*v));

          // Did the user ask us to use config.sub? If this is a hinted value,
          // then we assume it has already been passed through config.sub.
          //
          if (!hint && ops.config_sub_specified ())
          {
            s = run<string> (ops.config_sub (),
                             s.c_str (),
                             [] (string& l) {return move (l);});
            l5 ([&]{trace << "config.sub target: '" << s << "'";});
          }

          try
          {
            string canon;
            triplet t (s, canon);

            l5 ([&]{trace << "canonical target: '" << canon << "'; "
                          << "class: " << t.class_;});

            assert (!hint || s == canon);

            // Enter as bin.target.{cpu,vendor,system,version,class}.
            //
            r.assign<string> ("bin.target") = move (canon);
            r.assign<string> ("bin.target.cpu") = move (t.cpu);
            r.assign<string> ("bin.target.vendor") = move (t.vendor);
            r.assign<string> ("bin.target.system") = move (t.system);
            r.assign<string> ("bin.target.version") = move (t.version);
            r.assign<string> ("bin.target.class") = move (t.class_);
          }
          catch (const invalid_argument& e)
          {
            // This is where we suggest that the user specifies --config-sub
            // to help us out.
            //
            fail << "unable to parse binutils target '" << s << "': "
                 << e.what () <<
              info << "consider using the --config-sub option";
          }

          new_val = new_val || p.second; // False for a hinted value.
        }

        // config.bin.pattern
        //
        {
          const variable& var (var_pool.find ("config.bin.pattern"));

          // We first see if the value was specified via the configuration
          // mechanism.
          //
          auto p (omitted (r, var));
          const value* v (p.first);

          // Then see if there is a config hint (e.g., from the C++ module).
          //
          if (v == nullptr)
          {
            if (auto l = hints[var])
              v = l.value;
          }

          // For ease of use enter it as bin.pattern (since it can come from
          // different places).
          //
          if (v != nullptr)
          {
            const string& s (cast<string> (*v));

            if (s.find ('*') == string::npos)
              fail << "missing '*' in binutils pattern '" << s << "'";

            r.assign<string> ("bin.pattern") = s;
            new_val = new_val || p.second; // False for a hinted value.
          }
        }

        // If we set any new values (e.g., we are configuring), then print the
        // report at verbosity level 2 and up (-v).
        //
        if (verb >= (new_val ? 2 : 3))
        {
          diag_record dr (text);

          dr << "bin " << project (r) << '@' << r.out_path () << '\n'
             << "  target     " << cast<string> (r["bin.target"]);

          if (auto l = r["bin.pattern"])
            dr << '\n'
               << "  pattern    " << cast<string> (l);
        }
      }

      return true;
    }

    bool
    init (scope& r,
          scope& b,
          const location& loc,
          unique_ptr<module_base>&,
          bool,
          bool,
          const variable_map& hints)
    {
      tracer trace ("bin::init");
      l5 ([&]{trace << "for " << b.out_path ();});

      // Load bin.config.
      //
      if (!cast_false<bool> (b["bin.config.loaded"]))
        load_module ("bin.config", r, b, loc, false, hints);

      // Cache some config values we will be needing below.
      //
      const string& tclass (cast<string> (r["bin.target.class"]));

      // Register target types and configure their default "installability".
      //
      using namespace install;

      {
        auto& t (b.target_types);

        t.insert<obj>  ();
        t.insert<obje> ();
        t.insert<obja> ();
        t.insert<objs> ();

        t.insert<exe>  ();
        install_path<exe> (b, dir_path ("bin")); // Install into install.bin.

        t.insert<lib>  ();
        t.insert<liba> ();
        t.insert<libs> ();

        install_path<liba> (b, dir_path ("lib")); // Install into install.lib.
        install_mode<liba> (b, "644");

        // Should shared libraries have the executable bit? That depends on
        // who you ask. In Debian, for example, it should not unless, it
        // really is executable (i.e., has main()). On the other hand, on
        // some systems, this may be required in order for the dynamic
        // linker to be able to load the library. So, by default, we will
        // keep it executable, especially seeing that this is also the
        // behavior of autotools. At the same time, it is easy to override
        // this, for example:
        //
        // config.install.lib.mode=644
        //
        // And a library that wants to override any such overrides (e.g.,
        // because it does have main()) can do:
        //
        // libs{foo}: install.mode=755
        //
        // Everyone is happy then? On Windows libs{} is the DLL and goes to
        // bin/, not lib/.
        //
        install_path<libs> (b, dir_path (tclass == "windows" ? "bin" : "lib"));

        // Create additional target types for certain targets.
        //
        if (tclass == "windows")
        {
          // Import library.
          //
          t.insert<libi> ();
          install_path<libi> (b, dir_path ("lib"));
          install_mode<libi> (b, "644");
        }
      }

      // Register rules.
      //
      {
        auto& r (b.rules);

        r.insert<obj> (perform_update_id, "bin.obj", obj_);
        r.insert<obj> (perform_clean_id, "bin.obj", obj_);

        r.insert<lib> (perform_update_id, "bin.lib", lib_);
        r.insert<lib> (perform_clean_id, "bin.lib", lib_);

        // Configure member.
        //
        r.insert<lib> (configure_update_id, "bin.lib", lib_);

        //@@ Should we check if the install module was loaded
        //   (by checking if install operation is registered
        //   for this project)? If we do that, then install
        //   will have to be loaded before bin. Perhaps we
        //   should enforce loading of all operation-defining
        //   modules before all others?
        //
        r.insert<lib> (perform_install_id, "bin.lib", lib_);
      }

      return true;
    }

    bool
    ar_config_init (scope& r,
                    scope& b,
                    const location& loc,
                    unique_ptr<module_base>&,
                    bool first,
                    bool,
                    const variable_map& hints)
    {
      tracer trace ("bin::ar_config_init");
      l5 ([&]{trace << "for " << b.out_path ();});

      // Make sure bin.config is loaded.
      //
      if (!cast_false<bool> (b["bin.config.loaded"]))
        load_module ("bin.config", r, b, loc, false, hints);

      // Enter configuration variables.
      //
      if (first)
      {
        auto& v (var_pool);

        v.insert<path> ("config.bin.ar", true);
        v.insert<path> ("config.bin.ranlib", true);
      }

      // Configure.
      //
      if (first)
      {
        // config.bin.ar
        // config.bin.ranlib
        //
        // For config.bin.ar we have the default (plus the pattern) while
        // ranlib should be explicitly specified by the user in order for us
        // to use it (all targets that we currently care to support have the
        // ar -s option but if that changes we can always force the use of
        // ranlib for certain targets).
        //
        // Another idea is to refuse to use default 'ar' (without the pattern)
        // if the host/build targets don't match. On the other hand, a cross-
        // toolchain can be target-unprefixed. Also, without canonicalization,
        // comparing targets will be unreliable.
        //

        // Use the target to decide on the default binutils program names.
        //
        const string& tsys (cast<string> (r["bin.target.system"]));
        const char* ar_d (tsys == "win32-msvc" ? "lib" : "ar");

        // Don't save the default value to config.build so that if the user
        // changes, say, the C++ compiler (which hinted the pattern), then
        // ar will automatically change as well.
        //
        auto ap (
          config::required (
            r,
            "config.bin.ar",
            path (apply_pattern (ar_d, cast_null<string> (r["bin.pattern"]))),
            false,
            config::save_commented));

        auto rp (
          config::required (
            r,
            "config.bin.ranlib",
            nullptr,
            false,
            config::save_commented));

        const path& ar (cast<path> (ap.first));
        const path* ranlib (cast_null<path> (rp.first));

        if (ranlib != nullptr && ranlib->empty ()) // @@ BC LT [null].
          ranlib = nullptr;

        ar_info ari (guess_ar (ar, ranlib));

        // If this is a new value (e.g., we are configuring), then print the
        // report at verbosity level 2 and up (-v).
        //
        if (verb >= (ap.second || rp.second ? 2 : 3))
        {
          diag_record dr (text);

          dr << "bin.ar " << project (r) << '@' << r.out_path () << '\n'
             << "  ar         " << ar << '\n'
             << "  id         " << ari.ar_id << '\n'
             << "  signature  " << ari.ar_signature << '\n'
             << "  checksum   " << ari.ar_checksum;

          if (ranlib != nullptr)
          {
            dr << '\n'
               << "  ranlib     " << *ranlib << '\n'
               << "  id         " << ari.ranlib_id << '\n'
               << "  signature  " << ari.ranlib_signature << '\n'
               << "  checksum   " << ari.ranlib_checksum;
          }
        }

        r.assign<string> ("bin.ar.id") = move (ari.ar_id);
        r.assign<string> ("bin.ar.signature") = move (ari.ar_signature);
        r.assign<string> ("bin.ar.checksum") = move (ari.ar_checksum);

        if (ranlib != nullptr)
        {
          r.assign<string> ("bin.ranlib.id") = move (ari.ranlib_id);
          r.assign<string> ("bin.ranlib.signature") =
            move (ari.ranlib_signature);
          r.assign<string> ("bin.ranlib.checksum") =
            move (ari.ranlib_checksum);
        }
      }

      return true;
    }

    bool
    ar_init (scope& r,
             scope& b,
             const location& loc,
             unique_ptr<module_base>&,
             bool,
             bool,
             const variable_map& hints)
    {
      tracer trace ("bin::ar_init");
      l5 ([&]{trace << "for " << b.out_path ();});

      // Make sure the bin core and ar.config are loaded.
      //
      if (!cast_false<bool> (b["bin.loaded"]))
        load_module ("bin", r, b, loc, false, hints);

      if (!cast_false<bool> (b["bin.ar.config.loaded"]))
        load_module ("bin.ar.config", r, b, loc, false, hints);

      return true;
    }

    bool
    ld_config_init (scope& r,
                    scope& b,
                    const location& loc,
                    unique_ptr<module_base>&,
                    bool first,
                    bool,
                    const variable_map& hints)
    {
      tracer trace ("bin::ld_config_init");
      l5 ([&]{trace << "for " << b.out_path ();});

      // Make sure bin.config is loaded.
      //
      if (!cast_false<bool> (b["bin.config.loaded"]))
        load_module ("bin.config", r, b, loc, false, hints);

      // Enter configuration variables.
      //
      if (first)
      {
        auto& v (var_pool);

        v.insert<path> ("config.bin.ld", true);
      }

      // Configure.
      //
      if (first)
      {
        // config.bin.ld
        //
        // Use the target to decide on the default ld name.
        //
        const string& tsys (cast<string> (r["bin.target.system"]));
        const char* ld_d (tsys == "win32-msvc" ? "link" : "ld");

        auto p (
          config::required (
            r,
            "config.bin.ld",
            path (apply_pattern (ld_d, cast_null<string> (r["bin.pattern"]))),
            false,
            config::save_commented));

        const path& ld (cast<path> (p.first));
        ld_info ldi (guess_ld (ld));

        // If this is a new value (e.g., we are configuring), then print the
        // report at verbosity level 2 and up (-v).
        //
        if (verb >= (p.second ? 2 : 3))
        {
          text << "bin.ld " << project (r) << '@' << r.out_path () << '\n'
               << "  ld         " << ld << '\n'
               << "  id         " << ldi.id << '\n'
               << "  signature  " << ldi.signature << '\n'
               << "  checksum   " << ldi.checksum;
        }

        r.assign<string> ("bin.ld.id") = move (ldi.id);
        r.assign<string> ("bin.ld.signature") = move (ldi.signature);
        r.assign<string> ("bin.ld.checksum") = move (ldi.checksum);
      }

      return true;
    }

    bool
    ld_init (scope& r,
             scope& b,
             const location& loc,
             unique_ptr<module_base>&,
             bool,
             bool,
             const variable_map& hints)
    {
      tracer trace ("bin::ld_init");
      l5 ([&]{trace << "for " << b.out_path ();});

      // Make sure the bin core and ld.config are loaded.
      //
      if (!cast_false<bool> (b["bin.loaded"]))
        load_module ("bin", r, b, loc, false, hints);

      if (!cast_false<bool> (b["bin.ld.config.loaded"]))
        load_module ("bin.ld.config", r, b, loc, false, hints);

      const string& lid (cast<string> (r["bin.ld.id"]));

      // Register the pdb{} target if using the VC toolchain.
      //
      using namespace install;

      if (lid == "msvc")
      {
        const target_type& pdb (b.derive_target_type<file> ("pdb").first);
        install_path (pdb, b, dir_path ("bin")); // Goes to install.bin
        install_mode (pdb, b, "644");            // But not executable.
      }

      return true;
    }

    bool
    rc_config_init (scope& r,
                    scope& b,
                    const location& loc,
                    unique_ptr<module_base>&,
                    bool first,
                    bool,
                    const variable_map& hints)
    {
      tracer trace ("bin::rc_config_init");
      l5 ([&]{trace << "for " << b.out_path ();});

      // Make sure bin.config is loaded.
      //
      if (!cast_false<bool> (b["bin.config.loaded"]))
        load_module ("bin.config", r, b, loc, false, hints);

      // Enter configuration variables.
      //
      if (first)
      {
        auto& v (var_pool);

        v.insert<path> ("config.bin.rc", true);
      }

      // Configure.
      //
      if (first)
      {
        // config.bin.rc
        //
        // Use the target to decide on the default rc name.
        //
        const string& tsys (cast<string> (r["bin.target.system"]));
        const char* rc_d (tsys == "win32-msvc" ? "rc" : "windres");

        auto p (
          config::required (
            r,
            "config.bin.rc",
            path (apply_pattern (rc_d, cast_null<string> (r["bin.pattern"]))),
            false,
            config::save_commented));

        const path& rc (cast<path> (p.first));
        rc_info rci (guess_rc (rc));

        // If this is a new value (e.g., we are configuring), then print the
        // report at verbosity level 2 and up (-v).
        //
        if (verb >= (p.second ? 2 : 3))
        {
          text << "bin.rc " << project (r) << '@' << r.out_path () << '\n'
               << "  rc         " << rc << '\n'
               << "  id         " << rci.id << '\n'
               << "  signature  " << rci.signature << '\n'
               << "  checksum   " << rci.checksum;
        }

        r.assign<string> ("bin.rc.id") = move (rci.id);
        r.assign<string> ("bin.rc.signature") = move (rci.signature);
        r.assign<string> ("bin.rc.checksum") = move (rci.checksum);
      }

      return true;
    }

    bool
    rc_init (scope& r,
             scope& b,
             const location& loc,
             unique_ptr<module_base>&,
             bool,
             bool,
             const variable_map& hints)
    {
      tracer trace ("bin::rc_init");
      l5 ([&]{trace << "for " << b.out_path ();});

      // Make sure the bin core and rc.config are loaded.
      //
      if (!cast_false<bool> (b["bin.loaded"]))
        load_module ("bin", r, b, loc, false, hints);

      if (!cast_false<bool> (b["bin.rc.config.loaded"]))
        load_module ("bin.rc.config", r, b, loc, false, hints);

      return true;
    }
  }
}
