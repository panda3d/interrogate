/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file dtoolbase_cc.h
 * @author drose
 * @date 2000-09-13
 */

#ifndef DTOOLBASE_CC_H
#define DTOOLBASE_CC_H

// This file should never be included directly; it's intended to be included
// only from dtoolbase.h.  Include that file instead.

#ifdef __cplusplus

#ifdef USE_TAU
// Tau provides this destructive version of stdbool.h that we must mask.
#define __PDT_STDBOOL_H_
#endif

#ifdef CPPPARSER
#include <iostream>
#include <iomanip>
#include <string>
#include <utility>
#include <algorithm>

#define INLINE inline
#define ALWAYS_INLINE inline
#define MOVE(x) x

#define EXPORT_TEMPLATE_CLASS(expcl, exptp, classname)

// We define the macro PUBLISHED to mark C++ methods that are to be published
// via interrogate to scripting languages.  However, if we're not running the
// interrogate pass (CPPPARSER isn't defined), this maps to public.
#define PUBLISHED __published

typedef int ios_openmode;
typedef int ios_fmtflags;
typedef int ios_iostate;
typedef int ios_seekdir;

#else  // CPPPARSER

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <new>
#include <string>
#include <utility>
#include <algorithm>

typedef std::ios::openmode ios_openmode;
typedef std::ios::fmtflags ios_fmtflags;
typedef std::ios::iostate ios_iostate;
typedef std::ios::seekdir ios_seekdir;

#ifdef _MSC_VER
#define ALWAYS_INLINE __forceinline
#elif defined(__GNUC__)
#define ALWAYS_INLINE __attribute__((always_inline)) inline
#else
#define ALWAYS_INLINE inline
#endif

#ifdef FORCE_INLINING
// If FORCE_INLINING is defined, we use the keyword __forceinline, which tells
// MS VC++ to override its internal benefit heuristic and inline the fn if it
// is technically possible to do so.
#define INLINE ALWAYS_INLINE
#else
#define INLINE inline
#endif

// Determine the availability of C++11 features.
#if defined(_MSC_VER) && _MSC_VER < 1900 // Visual Studio 2015
#error Microsoft Visual C++ 2015 or later is required to compile interrogate.
#endif


#ifndef LINK_ALL_STATIC
// This macro must be used to export an instantiated template class from a
// DLL.  If the template class name itself contains commas, it may be
// necessary to first define a macro for the class name, to allow proper macro
// parameter passing.
#define EXPORT_TEMPLATE_CLASS(expcl, exptp, classname) \
  exptp template class expcl classname;
#else
#define EXPORT_TEMPLATE_CLASS(expcl, exptp, classname)
#endif

// We define the macro PUBLISHED to mark C++ methods that are to be published
// via interrogate to scripting languages.  However, if we're not running the
// interrogate pass (CPPPARSER isn't defined), this maps to public.
#define PUBLISHED public

#endif  // CPPPARSER

#define PANDA_MALLOC_SINGLE(size) ::malloc(size)
#define PANDA_FREE_SINGLE(ptr) ::free(ptr)
#define PANDA_MALLOC_ARRAY(size) ::malloc(size)
#define PANDA_REALLOC_ARRAY(ptr, size) ::realloc(ptr, size)
#define PANDA_FREE_ARRAY(ptr) ::free(ptr)

INLINE void thread_yield() {
}
INLINE void thread_consider_yield() {
}

#if defined(USE_TAU) && defined(_WIN32)
// Hack around tau's lack of DLL export declarations for Profiler class.
extern EXPCL_DTOOL_DTOOLBASE bool __tau_shutdown;
class EXPCL_DTOOL_DTOOLBASE TauProfile {
public:
  TauProfile(void *&tautimer, char *name, char *type, int group, char *group_name) {
    Tau_profile_c_timer(&tautimer, name, type, group, group_name);
    _tautimer = tautimer;
    TAU_PROFILE_START(_tautimer);
  }
  ~TauProfile() {
    if (!__tau_shutdown) {
      TAU_PROFILE_STOP(_tautimer);
    }
  }

private:
  void *_tautimer;
};

#undef TAU_PROFILE
#define TAU_PROFILE(name, type, group) \
  static void *__tautimer; \
  TauProfile __taupr(__tautimer, name, type, group, #group)

#undef TAU_PROFILE_EXIT
#define TAU_PROFILE_EXIT(msg) \
  __tau_shutdown = true; \
  Tau_exit(msg);

#endif  // USE_TAU

#endif  //  __cplusplus
#endif
