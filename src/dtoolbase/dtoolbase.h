/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file dtoolbase.h
 * @author drose
 * @date 2000-09-12
 */

/* This file is included at the beginning of every header file and/or
   C or C++ file.  It must be compilable for C as well as C++ files,
   so no C++-specific code or syntax can be put here.  See
   dtoolbase_cc.h for C++-specific stuff. */

#ifndef DTOOLBASE_H
#define DTOOLBASE_H

#ifdef _MSC_VER
/* These warning pragmas must appear before anything else for VC++ to
   respect them.  Sheesh. */

/* C4231: extern before template instantiation */
/* For some reason, this particular warning won't disable. */
#pragma warning (disable : 4231)
/* C4786: 255 char debug symbols */
#pragma warning (disable : 4786)
/* C4251: needs dll interface */
#pragma warning (disable : 4251)
/* C4503: decorated name length exceeded */
#pragma warning (disable : 4503)
/* C4305: truncation from 'const double' to 'float' */
#pragma warning (disable : 4305)
/* C4250: 'myclass' : inherits 'baseclass::member' via dominance */
#pragma warning (disable : 4250)
/* C4355: 'this' : used in base member initializer list */
#pragma warning (disable : 4355)
/* C4244: 'initializing' : conversion from 'double' to 'float', possible loss of data */
#pragma warning (disable : 4244)
/* C4267: 'var' : conversion from 'size_t' to 'type', possible loss of data */
#pragma warning (disable : 4267)
/* C4577: 'noexcept' used with no exception handling mode specified */
#pragma warning (disable : 4577)
#endif  /* _MSC_VER */

/* Windows likes to define min() and max() macros, which will conflict with
   std::min() and std::max() respectively, unless we do this: */
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif // !NOMINMAX
#endif // _WIN32

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif // !__has_builtin

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif // !__has_attribute

// Use NODEFAULT to optimize a switch() stmt to tell MSVC to automatically go
// to the final untested case after it has failed all the other cases (i.e.
// 'assume at least one of the cases is always true')
#ifdef _DEBUG
#define NODEFAULT  default: assert(0); break;
#elif __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5) || __has_builtin(__builtin_unreachable)
#define NODEFAULT  default: __builtin_unreachable();
#elif defined(_MSC_VER)
#define NODEFAULT  default: __assume(0);   // special VC keyword
#else // NODEFAULT
#define NODEFAULT
#endif // NODEFAULT

// Use this to hint the compiler that a memory address is aligned.
#if __has_builtin(__builtin_assume_aligned) || __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7)
#define ASSUME_ALIGNED(x, y) (__builtin_assume_aligned(x, y))
#else // ASSUME_ALIGNED
#define ASSUME_ALIGNED(x, y) (x)
#endif // ASSUME_ALIGNED

#if __has_attribute(assume_aligned) || __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
#define RETURNS_ALIGNED(x) __attribute__((assume_aligned(x)))
#else // RETURNS_ALIGNED
#define RETURNS_ALIGNED(x)
#endif // RETURNS_ALIGNED

#ifdef __GNUC__
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else // LIKELY/UNLIKELY
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif // LIKELY/UNLIKELY

/*
  include win32 defns for everything up to WinServer2003, and assume
  I'm smart enough to use GetProcAddress for backward compat on
  w95/w98 for newer fns
*/
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif // _WIN32_WINNT
#define _WIN32_WINNT 0x0600

#ifdef __cplusplus
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif // !__STDC_LIMIT_MACROS
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif // !__STDC_CONSTANT_MACROS
#endif // __cplusplus

// This is a workaround for a glibc bug that is triggered by clang when
// compiling with -ffast-math.
#if defined(__clang__) && defined(__GLIBC__)
#include <sys/cdefs.h>
#ifndef __extern_always_inline
#define __extern_always_inline extern __always_inline
#endif // !__extern_always_inline
#endif // __clang__ && __GLIBC__

#include "dtoolsymbols.h"

// always include assert.h until drose unbreaks it for opt4
#include <assert.h>

#ifdef __GNUC__
// Large file >2GB support this needs be be before systypes.h and other C
// headers
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE 1
#endif // __GNUC__

#ifdef CPPPARSER
#include <stdtypedefs.h>
#endif // CPPPARSER

#ifdef USE_TAU
/* If we're building with the Tau instrumentor, include the
   appropriate header file to pick up the TAU macros. */
#include <TAU.h>
#include <Profile/Profiler.h>
#else // USE_TAU
/* Otherwise, if we're not building with the Tau instrumentor, turn
   off all the TAU macros.  We could include the Tau header file to do
   this, but it's better not to assume that Tau is installed. */
#define TAU_TYPE_STRING(profileString, str)
#define TAU_PROFILE(name, type, group)
#define TAU_PROFILE_TIMER(var, name, type, group)
#define TAU_PROFILE_START(var)
#define TAU_PROFILE_STOP(var)
#define TAU_PROFILE_STMT(stmt)
#define TAU_PROFILE_EXIT(msg)
#define TAU_PROFILE_INIT(argc, argv)
#define TAU_PROFILE_SET_NODE(node)
#define TAU_PROFILE_SET_CONTEXT(context)
#define TAU_PROFILE_SET_GROUP_NAME(newname)
#define TAU_PROFILE_TIMER_SET_GROUP_NAME(t, newname)
#define TAU_PROFILE_CALLSTACK()
#define TAU_DB_DUMP()
#define TAU_DB_PURGE()

#define TAU_REGISTER_CONTEXT_EVENT(event, name)
#define TAU_CONTEXT_EVENT(event, data)
#define TAU_DISABLE_CONTEXT_EVENT(event)
#define TAU_ENABLE_CONTEXT_EVENT(event)

#define TAU_REGISTER_EVENT(event, name)
#define TAU_EVENT(event, data)
#define TAU_EVENT_DISABLE_MIN(event)
#define TAU_EVENT_DISABLE_MAX(event)
#define TAU_EVENT_DISABLE_MEAN(event)
#define TAU_EVENT_DISABLE_STDDEV(event)
#define TAU_REPORT_STATISTICS()
#define TAU_REPORT_THREAD_STATISTICS()
#define TAU_REGISTER_THREAD()
#define TAU_REGISTER_FORK(id, op)
#define TAU_ENABLE_INSTRUMENTATION()
#define TAU_DISABLE_INSTRUMENTATION()
#define TAU_ENABLE_GROUP(group)
#define TAU_DISABLE_GROUP(group)
#define TAU_ENABLE_GROUP_NAME(group)
#define TAU_DISABLE_GROUP_NAME(group)
#define TAU_ENABLE_ALL_GROUPS()
#define TAU_DISABLE_ALL_GROUPS()
#define TAU_TRACK_MEMORY()
#define TAU_TRACK_MEMORY_HERE()
#define TAU_ENABLE_TRACKING_MEMORY()
#define TAU_DISABLE_TRACKING_MEMORY()
#define TAU_ENABLE_TRACKING_MUSE_EVENTS()
#define TAU_DISABLE_TRACKING_MUSE_EVENTS()
#define TAU_TRACK_MUSE_EVENTS()
#define TAU_SET_INTERRUPT_INTERVAL(value)

#define TAU_TRACE_SENDMSG(type, destination, length)
#define TAU_TRACE_RECVMSG(type, source, length)

#define TAU_MAPPING(stmt, group) stmt
#define TAU_MAPPING_OBJECT(FuncInfoVar)
#define TAU_MAPPING_LINK(FuncInfoVar, Group)
#define TAU_MAPPING_PROFILE(FuncInfoVar)
#define TAU_MAPPING_CREATE(name, type, key, groupname, tid)
#define TAU_MAPPING_PROFILE_TIMER(Timer, FuncInfoVar, tid)
#define TAU_MAPPING_TIMER_CREATE(t, name, type, gr, group_name)
#define TAU_MAPPING_PROFILE_START(Timer, tid)
#define TAU_MAPPING_PROFILE_STOP(tid)
#define TAU_MAPPING_PROFILE_EXIT(msg, tid)
#define TAU_MAPPING_DB_DUMP(tid)
#define TAU_MAPPING_DB_PURGE(tid)
#define TAU_MAPPING_PROFILE_SET_NODE(node, tid)
#define TAU_MAPPING_PROFILE_SET_GROUP_NAME(timer, name)
#define TAU_PROFILE_TIMER_SET_NAME(t, newname)
#define TAU_PROFILE_TIMER_SET_TYPE(t, newname)
#define TAU_PROFILE_TIMER_SET_GROUP(t, id)
#define TAU_MAPPING_PROFILE_SET_NAME(timer, name)
#define TAU_MAPPING_PROFILE_SET_TYPE(timer, name)
#define TAU_MAPPING_PROFILE_SET_GROUP(timer, id)
#define TAU_MAPPING_PROFILE_GET_GROUP_NAME(timer)
#define TAU_MAPPING_PROFILE_GET_GROUP(timer)
#define TAU_MAPPING_PROFILE_GET_NAME(timer)
#define TAU_MAPPING_PROFILE_GET_TYPE(timer)

#define TAU_PHASE(name, type, group)
#define TAU_PHASE_CREATE_STATIC(var, name, type, group)
#define TAU_PHASE_CREATE_DYNAMIC(var, name, type, group)
#define TAU_PHASE_START(var)
#define TAU_PHASE_STOP(var)
#define TAU_GLOBAL_PHASE(timer, name, type, group)
#define TAU_GLOBAL_PHASE_START(timer)
#define TAU_GLOBAL_PHASE_STOP(timer)
#define TAU_GLOBAL_PHASE_EXTERNAL(timer)
#define TAU_GLOBAL_TIMER(timer, name, type, group)
#define TAU_GLOBAL_TIMER_EXTERNAL(timer)
#define TAU_GLOBAL_TIMER_START(timer)
#define TAU_GLOBAL_TIMER_STOP()

#endif  /* USE_TAU */

/*
 We define the macros BEGIN_PUBLISH and END_PUBLISH to bracket
 functions and global variable definitions that are to be published
 via interrogate to scripting languages.  Also, the macro BLOCKING is
 used to flag any function or method that might perform I/O blocking
 and thus needs to release Python threads for its duration.
 */
#ifdef CPPPARSER
#define BEGIN_PUBLISH __begin_publish
#define END_PUBLISH __end_publish
#define BLOCKING __blocking
#define MAKE_PROPERTY(property_name, ...) __make_property(property_name, __VA_ARGS__)
#define MAKE_PROPERTY2(property_name, ...) __make_property2(property_name, __VA_ARGS__)
#define MAKE_SEQ(seq_name, num_name, element_name) __make_seq(seq_name, num_name, element_name)
#define MAKE_SEQ_PROPERTY(property_name, ...) __make_seq_property(property_name, __VA_ARGS__)
#define MAKE_MAP_PROPERTY(property_name, ...) __make_map_property(property_name, __VA_ARGS__)
#define MAKE_MAP_KEYS_SEQ(property_name, ...) __make_map_keys_seq(property_name, __VA_ARGS__)
#define EXTENSION(x) __extension x
#define EXTEND __extension
#else // CPPPARSER
#define BEGIN_PUBLISH
#define END_PUBLISH
#define BLOCKING
#define MAKE_PROPERTY(property_name, ...)
#define MAKE_PROPERTY2(property_name, ...)
#define MAKE_SEQ(seq_name, num_name, element_name)
#define MAKE_SEQ_PROPERTY(property_name, ...)
#define MAKE_MAP_PROPERTY(property_name, ...)
#define MAKE_MAP_KEYS_SEQ(property_name, ...)
#define EXTENSION(x)
#define EXTEND
#define PY_EXTENSION(x)
#define PY_EXTEND(...)
#define PY_MAKE_PROPERTY(property_name, ...)
#define PY_MAKE_SEQ_PROPERTY(property_name, ...)
#endif // CPPPARSER

/* These symbols are used in dtoolsymbols.h and pandasymbols.h. */
#if defined(_WIN32) && !defined(CPPPARSER) && !defined(LINK_ALL_STATIC)
#define EXPORT_CLASS __declspec(dllexport)
#define IMPORT_CLASS __declspec(dllimport)
#elif __GNUC__ >= 4 && !defined(CPPPARSER) && !defined(LINK_ALL_STATIC)
#define EXPORT_CLASS __attribute__((visibility("default")))
#define IMPORT_CLASS
#else // IMPORT/EXPORT
#define EXPORT_CLASS
#define IMPORT_CLASS
#endif // IMPORT/EXPORT

/* "extern template" is now part of the C++11 standard. */
#if defined(CPPPARSER) || defined(LINK_ALL_STATIC)
#define EXPORT_TEMPL
#define IMPORT_TEMPL
#elif defined(_MSC_VER)
/* Nowadays, we'd define both of these as "extern" in all cases, so that
   the header file always marks the symbol as "extern" and the .cxx file
   explicitly instantiates it.  However, MSVC versions before 2013 break
   the spec by explicitly disallowing it, so we have to instantiate the
   class from the header file.  Fortunately, its linker is okay with the
   duplicate template instantiations that this causes. */
#define EXPORT_TEMPL
#define IMPORT_TEMPL extern
#else // IMPORT/EXPORT template
#define EXPORT_TEMPL extern
#define IMPORT_TEMPL extern
#endif // IMPORT/EXPORT template

#ifdef __cplusplus
#include "dtoolbase_cc.h"
#endif // __cplusplus

#endif // !DTOOLBASE_H
