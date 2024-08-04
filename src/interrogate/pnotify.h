/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file pnotify.h
 * @author drose
 * @date 2000-02-28
 */

#ifndef NOTIFY_H
#define NOTIFY_H

#include "dtoolbase.h"

#include <assert.h>

// This defines the symbol nout in the same way that cerr and cout are
// defined, for compactness of C++ code that uses Notify in its simplest form.
// Maybe it's a good idea to define this symbol and maybe it's not, but it
// does seem that "nout" isn't likely to collide with any other name.

#define nout (std::cerr)

// Here are a couple of assert-type functions.  These are designed to avoid
// simply dumping core, since that's quite troublesome when the programmer is
// working in a higher-level environment that is calling into the C++ layer.

// nassertr() is intended to be used in functions that have return values; it
// returns the indicated value if the assertion fails.

// nassertv() is intended to be used in functions that do not have return
// values; it simply returns if the assertion fails.

// nassertd() does not return from the function, but instead executes the
// following block of code (like an if statement) if the assertion fails.

// nassertr_always() and nassertv_always() are like nassertr() and nassertv(),
// except that they will not get completely compiled out if NDEBUG is set.
// Instead, they will quietly return from the function.  These macros are
// appropriate, for instance, for sanity checking user input parameters, where
// optimal performance is not paramount.

// nassert_static() is a compile-time assertion.  It should only be used with
// constant expressions and compilation will fail if the assertion is not
// true.

#ifdef __GNUC__
// Tell the optimizer to optimize for the case where the condition is true.
#define _nassert_check(condition) (__builtin_expect(!(condition), 0))
#else
#define _nassert_check(condition) (!(condition))
#endif

#ifdef NDEBUG

#define nassertr(condition, return_value)
#define nassertv(condition)
#define nassertd(condition) if (false)
// We trust the compiler to optimize the above out.

#define nassertr_always(condition, return_value) \
  { \
    if (_nassert_check(condition)) { \
      return return_value; \
    } \
  }

#define nassertv_always(condition) \
  { \
    if (_nassert_check(condition)) { \
      return; \
    } \
  }

#define nassert_raise(message) do { std::cerr << message << std::endl; } while (0)

#elif defined(__clang_analyzer__)

// We define these so that clang-tidy can generally assume that assertions will
// not occur, since we don't really care about things like minor memory leaks
// on assertion failure.

#define nassertr(condition, return_value) \
  { \
    if (!(condition)) { \
      abort(); \
      return return_value; \
    } \
  }

#define nassertv(condition) \
  { \
    if (!(condition)) { \
      abort(); \
      return; \
    } \
  }

#define nassertd(condition) \
  if (!(condition))

#define nassertr_always(condition, return_value) nassertr(condition, return_value)
#define nassertv_always(condition) nassertv(condition)

#define nassert_raise(message) abort()

#else   // NDEBUG

#define nassertr(condition, return_value) \
  { \
    assert(condition); \
    if (false) return return_value; \
  }

#define nassertv(condition) \
  { \
    assert(condition); \
    if (false) return; \
  }

#define nassertd(condition) \
  assert(condition); \
  if (false)

#define nassertr_always(condition, return_value) nassertr(condition, return_value)
#define nassertv_always(condition) nassertv(condition)

#define nassert_raise(message) do { std::cerr << message << std::endl; abort(); } while (0)

#endif  // NDEBUG

#endif
