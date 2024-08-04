/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file dtoolbase.cxx
 * @author drose
 * @date 2000-09-12
 */

#include "dtoolbase.h"

#if !defined(CPPPARSER) && !defined(LINK_ALL_STATIC) && !defined(BUILDING_DTOOL_DTOOLBASE)
  #error Buildsystem error: BUILDING_DTOOL_DTOOLBASE not defined
#endif

#if defined(USE_TAU) && defined(_WIN32)
// Hack around tau's lack of DLL export declarations for Profiler class.
bool __tau_shutdown = false;
#endif
