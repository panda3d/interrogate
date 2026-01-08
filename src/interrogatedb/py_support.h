/**
 * @file py_support.h
 * @author rdb
 * @date 2025-09-03
 */

#ifndef PY_SUPPORT_H
#define PY_SUPPORT_H

/**
 * This header file is for helper functions that are only used by
 * interrogate-generated code, whereas py_panda.h is for code that may also
 * be used by extensions.
 */

#if defined(HAVE_PYTHON) && !defined(CPPPARSER)

#include "py_panda.h"

#if PY_VERSION_HEX >= 0x03080000
#define METH_FASTCALL_OR_VARARGS METH_FASTCALL
#define FASTCALL_OR_VARARGS_ARGS PyObject *const *fc_args, Py_ssize_t fc_nargs
#define FASTCALL_OR_VARARGS_KEYWORDS_ARGS PyObject *const *fc_args, Py_ssize_t fc_nargs, PyObject *fc_kwnames
#else
#define METH_FASTCALL_OR_VARARGS METH_VARARGS
#define FASTCALL_OR_VARARGS_ARGS PyObject *args
#define FASTCALL_OR_VARARGS_KEYWORDS_ARGS PyObject *args, PyObject *kwds
#endif

using namespace std;

// This is now simply a forward declaration.  The actual definition is created
// by the code generator.
#define Define_Dtool_Class(MODULE_NAME, CLASS_NAME, PUBLIC_NAME) \
  extern Dtool_PyTypedObject Dtool_##CLASS_NAME;

// More Macro(s) to Implement class functions.. Usually used if C++ needs type
// information
#define Define_Dtool_new(CLASS_NAME,CNAME)\
static PyObject *Dtool_new_##CLASS_NAME(PyTypeObject *type, PyObject *args, PyObject *kwds) {\
  (void) args; (void) kwds;\
  PyObject *self = type->tp_alloc(type, 0);\
  ((Dtool_PyInstDef *)self)->_signature = PY_PANDA_SIGNATURE;\
  ((Dtool_PyInstDef *)self)->_My_Type = &Dtool_##CLASS_NAME;\
  return self;\
}

// The following used to be in the above macro, but it doesn't seem to be
// necessary as tp_alloc memsets the object to 0.
//  ((Dtool_PyInstDef *)self)->_ptr_to_object = NULL;
//  ((Dtool_PyInstDef *)self)->_memory_rules = false;
//  ((Dtool_PyInstDef *)self)->_is_const = false;

// Delete functions..
#ifdef NDEBUG
#define Define_Dtool_FreeInstance_Private(CLASS_NAME,CNAME)\
static void Dtool_FreeInstance_##CLASS_NAME(PyObject *self) {\
  Py_TYPE(self)->tp_free(self);\
}
#else // NDEBUG
#define Define_Dtool_FreeInstance_Private(CLASS_NAME,CNAME)\
static void Dtool_FreeInstance_##CLASS_NAME(PyObject *self) {\
  if (DtoolInstance_VOID_PTR(self) != nullptr) {\
    if (((Dtool_PyInstDef *)self)->_memory_rules) {\
      std::cerr << "Detected leak for " << #CLASS_NAME \
           << " which interrogate cannot delete.\n"; \
    }\
  }\
  Py_TYPE(self)->tp_free(self);\
}
#endif  // NDEBUG

#define Define_Dtool_FreeInstance(CLASS_NAME,CNAME)\
static void Dtool_FreeInstance_##CLASS_NAME(PyObject *self) {\
  if (DtoolInstance_VOID_PTR(self) != nullptr) {\
    if (((Dtool_PyInstDef *)self)->_memory_rules) {\
      delete (CNAME *)DtoolInstance_VOID_PTR(self);\
    }\
  }\
  Py_TYPE(self)->tp_free(self);\
}

#define Define_Dtool_FreeInstanceRef(CLASS_NAME,CNAME)\
static void Dtool_FreeInstance_##CLASS_NAME(PyObject *self) {\
  if (DtoolInstance_VOID_PTR(self) != nullptr) {\
    if (((Dtool_PyInstDef *)self)->_memory_rules) {\
      unref_delete((CNAME *)DtoolInstance_VOID_PTR(self));\
    }\
  }\
  Py_TYPE(self)->tp_free(self);\
}

#define Define_Dtool_FreeInstanceRef_Private(CLASS_NAME,CNAME)\
static void Dtool_FreeInstance_##CLASS_NAME(PyObject *self) {\
  if (DtoolInstance_VOID_PTR(self) != nullptr) {\
    if (((Dtool_PyInstDef *)self)->_memory_rules) {\
      unref_delete((ReferenceCount *)(CNAME *)DtoolInstance_VOID_PTR(self));\
    }\
  }\
  Py_TYPE(self)->tp_free(self);\
}

#define Define_Dtool_Simple_FreeInstance(CLASS_NAME, CNAME)\
static void Dtool_FreeInstance_##CLASS_NAME(PyObject *self) {\
  ((Dtool_InstDef_##CLASS_NAME *)self)->_value.~##CLASS_NAME();\
  Py_TYPE(self)->tp_free(self);\

#if PY_VERSION_HEX >= 0x030d0000
class Dtool_TypeMap : public std::map<std::string, Dtool_PyTypedObject *> {
public:
  PyMutex _lock { 0 };
};
#else
typedef std::map<std::string, Dtool_PyTypedObject *> Dtool_TypeMap;
#endif

EXPCL_PYPANDA Dtool_TypeMap *Dtool_GetGlobalTypeMap();


EXPCL_PYPANDA void DtoolProxy_Init(DtoolProxy *proxy, PyObject *self,
                                   Dtool_PyTypedObject &classdef,
                                   TypeRegistry::PythonWrapFunc *wrap_func);

INLINE Py_hash_t DtoolInstance_HashPointer(PyObject *self);
INLINE int DtoolInstance_ComparePointers(PyObject *v1, PyObject *v2);
INLINE PyObject *DtoolInstance_RichComparePointers(PyObject *v1, PyObject *v2, int op);

// Functions related to error reporting.
EXPCL_PYPANDA bool _Dtool_CheckErrorOccurred();

#ifdef NDEBUG
#define Dtool_CheckErrorOccurred() (UNLIKELY(PyErr_Occurred() != nullptr))
#else
#define Dtool_CheckErrorOccurred() (UNLIKELY(_Dtool_CheckErrorOccurred()))
#endif

// These functions are similar to Dtool_WrapValue, except that they also
// contain code for checking assertions and exceptions when compiling with
// NDEBUG mode on.
EXPCL_PYPANDA PyObject *_Dtool_Return_None();
EXPCL_PYPANDA PyObject *Dtool_Return_Bool(bool value);
EXPCL_PYPANDA PyObject *_Dtool_Return(PyObject *value);

#ifdef NDEBUG
#define Dtool_Return_None() (LIKELY(PyErr_Occurred() == nullptr) ? (Py_NewRef(Py_None)) : nullptr)
#define Dtool_Return(value) (LIKELY(PyErr_Occurred() == nullptr) ? value : nullptr)
#else
#define Dtool_Return_None() _Dtool_Return_None()
#define Dtool_Return(value) _Dtool_Return(value)
#endif

ALWAYS_INLINE void Dtool_Assign_PyObject(PyObject *&ptr, PyObject *value);

/**
 * Wrapper around Python 3.4's enum library, which does not have a C API.
 */
EXPCL_PYPANDA PyTypeObject *Dtool_EnumType_Create(const char *name, PyObject *names,
                                                  const char *module = nullptr);
INLINE long Dtool_EnumValue_AsLong(PyObject *value);

// Macro(s) class definition .. Used to allocate storage and init some values
// for a Dtool Py Type object.

// struct Dtool_PyTypedObject Dtool_##CLASS_NAME;

#define Define_Module_Class_Internal(MODULE_NAME,CLASS_NAME,CNAME)\
extern struct Dtool_PyTypedObject Dtool_##CLASS_NAME;\
static int Dtool_Init_##CLASS_NAME(PyObject *self, PyObject *args, PyObject *kwds);\
static PyObject *Dtool_new_##CLASS_NAME(PyTypeObject *type, PyObject *args, PyObject *kwds);

#define Define_Module_Class(MODULE_NAME,CLASS_NAME,CNAME,PUBLIC_NAME)\
Define_Module_Class_Internal(MODULE_NAME,CLASS_NAME,CNAME)\
Define_Dtool_new(CLASS_NAME,CNAME)\
Define_Dtool_FreeInstance(CLASS_NAME,CNAME)\
Define_Dtool_Class(MODULE_NAME,CLASS_NAME,PUBLIC_NAME)

#define Define_Module_Class_Private(MODULE_NAME,CLASS_NAME,CNAME,PUBLIC_NAME)\
Define_Module_Class_Internal(MODULE_NAME,CLASS_NAME,CNAME)\
Define_Dtool_new(CLASS_NAME,CNAME)\
Define_Dtool_FreeInstance_Private(CLASS_NAME,CNAME)\
Define_Dtool_Class(MODULE_NAME,CLASS_NAME,PUBLIC_NAME)

#define Define_Module_ClassRef_Private(MODULE_NAME,CLASS_NAME,CNAME,PUBLIC_NAME)\
Define_Module_Class_Internal(MODULE_NAME,CLASS_NAME,CNAME)\
Define_Dtool_new(CLASS_NAME,CNAME)\
Define_Dtool_FreeInstanceRef_Private(CLASS_NAME,CNAME)\
Define_Dtool_Class(MODULE_NAME,CLASS_NAME,PUBLIC_NAME)

#define Define_Module_ClassRef(MODULE_NAME,CLASS_NAME,CNAME,PUBLIC_NAME)\
Define_Module_Class_Internal(MODULE_NAME,CLASS_NAME,CNAME)\
Define_Dtool_new(CLASS_NAME,CNAME)\
Define_Dtool_FreeInstanceRef(CLASS_NAME,CNAME)\
Define_Dtool_Class(MODULE_NAME,CLASS_NAME,PUBLIC_NAME)

// A heler function to glu methed definition together .. that can not be done
// at code generation time becouse of multiple generation passes in
// interigate..
typedef std::map<std::string, PyMethodDef *> MethodDefmap;

// We need a way to runtime merge compile units into a python "Module" .. this
// is done with the fallowing structors and code.. along with the support of
// interigate_module

struct Dtool_TypeDef {
  const char *const name;
  Dtool_PyTypedObject *type;
};

struct LibraryDef {
  PyMethodDef *const _methods;
  const Dtool_TypeDef *const _types;
  Dtool_TypeDef *const _external_types;
  const struct InterrogateModuleDef *const _module_def;
};

#if PY_MAJOR_VERSION >= 3
EXPCL_PYPANDA PyObject *Dtool_PyModuleInitHelper(const LibraryDef *defs[], PyModuleDef *module_def);
#else
EXPCL_PYPANDA PyObject *Dtool_PyModuleInitHelper(const LibraryDef *defs[], const char *modulename);
#endif

// HACK.... Be carefull Dtool_BorrowThisReference This function can be used to
// grab the "THIS" pointer from an object and use it Required to support fom
// historical inharatence in the for of "is this instance of"..
EXPCL_PYPANDA PyObject *Dtool_BorrowThisReference(PyObject *self, PyObject *args);

#define DTOOL_PyObject_HashPointer DtoolInstance_HashPointer
#define DTOOL_PyObject_ComparePointers DtoolInstance_ComparePointers

EXPCL_PYPANDA PyObject *
Dtool_AddToDictionary(PyObject *self1, PyObject *args);

EXPCL_PYPANDA PyObject *
copy_from_make_copy(PyObject *self, PyObject *noargs);

EXPCL_PYPANDA PyObject *
copy_from_copy_constructor(PyObject *self, PyObject *noargs);

EXPCL_PYPANDA PyObject *
map_deepcopy_to_copy(PyObject *self, PyObject *args);

/**
 * These functions check whether the arguments passed to a function conform to
 * certain expectations.
 */
ALWAYS_INLINE bool Dtool_CheckNoArgs(PyObject *args);
ALWAYS_INLINE bool Dtool_CheckNoArgs(PyObject *args, PyObject *kwds);
EXPCL_PYPANDA bool Dtool_ExtractArg(PyObject **result, PyObject *args,
                                    PyObject *kwds, const char *keyword);
EXPCL_PYPANDA bool Dtool_ExtractArg(PyObject **result, PyObject *args,
                                    PyObject *kwds);
EXPCL_PYPANDA bool Dtool_ExtractOptionalArg(PyObject **result, PyObject *args,
                                            PyObject *kwds, const char *keyword);
EXPCL_PYPANDA bool Dtool_ExtractOptionalArg(PyObject **result, PyObject *args,
                                            PyObject *kwds);

#include "py_support.I"

#include "py_wrappers.h"

#endif  // HAVE_PYTHON && !CPPPARSER

#endif // PY_SUPPORT_H
