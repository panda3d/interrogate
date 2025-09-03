/**
 * @file py_panda.cxx
 * @author drose
 * @date 2005-07-04
 */

#include "py_panda.h"

#ifdef HAVE_PYTHON

using std::string;

/**

 */
void DTOOL_Call_ExtractThisPointerForType(PyObject *self, Dtool_PyTypedObject *classdef, void **answer) {
  if (DtoolInstance_Check(self)) {
    *answer = DtoolInstance_UPCAST(self, *classdef);
  } else {
    *answer = nullptr;
  }
}

/**
 * This is a support function for the Python bindings: it extracts the
 * underlying C++ pointer of the given type for a given Python object.  If it
 * was of the wrong type, raises an AttributeError.
 */
bool Dtool_Call_ExtractThisPointer(PyObject *self, Dtool_PyTypedObject &classdef, void **answer) {
  if (self == nullptr || !DtoolInstance_Check(self) || DtoolInstance_VOID_PTR(self) == nullptr) {
    Dtool_Raise_TypeError("C++ object is not yet constructed, or already destructed.");
    return false;
  }

  *answer = DtoolInstance_UPCAST(self, classdef);
  return true;
}

/**
 * The same thing as Dtool_Call_ExtractThisPointer, except that it performs
 * the additional check that the pointer is a non-const pointer.  This is
 * called by function wrappers for functions of which all overloads are non-
 * const, and saves a bit of code.
 *
 * The extra method_name argument is used in formatting the error message.
 */
bool Dtool_Call_ExtractThisPointer_NonConst(PyObject *self, Dtool_PyTypedObject &classdef,
                                            void **answer, const char *method_name) {

  if (self == nullptr || !DtoolInstance_Check(self) || DtoolInstance_VOID_PTR(self) == nullptr) {
    Dtool_Raise_TypeError("C++ object is not yet constructed, or already destructed.");
    return false;
  }

  if (DtoolInstance_IS_CONST(self)) {
    // All overloads of this function are non-const.
    PyErr_Format(PyExc_TypeError,
                 "Cannot call %s() on a const object.",
                 method_name);
    return false;
  }

  *answer = DtoolInstance_UPCAST(self, classdef);
  return true;
}

/**
 * Extracts the C++ pointer for an object, given its Python wrapper object,
 * for passing as the parameter to a C++ function.
 *
 * self is the Python wrapper object in question.
 *
 * classdef is the Python class wrapper for the C++ class in which the this
 * pointer should be returned.  (This may require an upcast operation, if self
 * is not already an instance of classdef.)
 *
 * param and function_name are used for error reporting only, and describe the
 * particular function and parameter index for this parameter.
 *
 * const_ok is true if the function is declared const and can therefore be
 * called with either a const or non-const "this" pointer, or false if the
 * function is declared non-const, and can therefore be called with only a
 * non-const "this" pointer.
 *
 * The return value is the C++ pointer that was extracted, or NULL if there
 * was a problem (in which case the Python exception state will have been
 * set).
 */
void *
DTOOL_Call_GetPointerThisClass(PyObject *self, Dtool_PyTypedObject *classdef,
                               int param, const string &function_name, bool const_ok,
                               bool report_errors) {
  // if (PyErr_Occurred()) { return nullptr; }
  if (self == nullptr) {
    if (report_errors) {
      return Dtool_Raise_TypeError("self is nullptr");
    }
    return nullptr;
  }

  if (DtoolInstance_Check(self)) {
    void *result = DtoolInstance_UPCAST(self, *classdef);

    if (result != nullptr) {
      if (const_ok || !DtoolInstance_IS_CONST(self)) {
        return result;
      }

      if (report_errors) {
        return PyErr_Format(PyExc_TypeError,
                            "%s() argument %d may not be const",
                            function_name.c_str(), param);
      }
      return nullptr;
    }
  }

  if (report_errors) {
    return Dtool_Raise_ArgTypeError(self, param, function_name.c_str(), classdef->_PyType.tp_name);
  }

  return nullptr;
}

/**
 * Raises an AssertionError containing the last thrown assert message, and
 * clears the assertion flag.  Returns NULL.
 */
PyObject *Dtool_Raise_AssertionError() {
  Notify *notify = Notify::ptr();
#if PY_MAJOR_VERSION >= 3
  PyObject *message = PyUnicode_FromString(notify->get_assert_error_message().c_str());
#else
  PyObject *message = PyString_FromString(notify->get_assert_error_message().c_str());
#endif
  PyErr_SetObject(PyExc_AssertionError, message);
  notify->clear_assert_failed();
  return nullptr;
}

/**
 * Raises a TypeError with the given message, and returns NULL.
 */
PyObject *Dtool_Raise_TypeError(const char *message) {
  PyErr_SetString(PyExc_TypeError, message);
  return nullptr;
}

/**
 * Raises a TypeError of the form: function_name() argument n must be type,
 * not type for a given object passed to a function.
 *
 * Always returns NULL so that it can be conveniently used as a return
 * expression for wrapper functions that return a PyObject pointer.
 */
PyObject *Dtool_Raise_ArgTypeError(PyObject *obj, int param, const char *function_name, const char *type_name) {
#if PY_MAJOR_VERSION >= 3
  PyObject *message = PyUnicode_FromFormat(
#else
  PyObject *message = PyString_FromFormat(
#endif
    "%s() argument %d must be %s, not %s",
    function_name, param, type_name,
    Py_TYPE(obj)->tp_name);

  PyErr_SetObject(PyExc_TypeError, message);
  return nullptr;
}

/**
 * Raises an AttributeError of the form: 'type' has no attribute 'attr'
 *
 * Always returns NULL so that it can be conveniently used as a return
 * expression for wrapper functions that return a PyObject pointer.
 */
PyObject *Dtool_Raise_AttributeError(PyObject *obj, const char *attribute) {
#if PY_MAJOR_VERSION >= 3
  PyObject *message = PyUnicode_FromFormat(
#else
  PyObject *message = PyString_FromFormat(
#endif
    "'%.100s' object has no attribute '%.200s'",
    Py_TYPE(obj)->tp_name, attribute);

  PyErr_SetObject(PyExc_AttributeError, message);
  return nullptr;
}

/**
 * Raises a TypeError of the form: can't delete attr attribute
 *
 * Always returns -1 so that it can be conveniently used as a return
 * expression for wrapper functions that return an int.
 */
int Dtool_Raise_CantDeleteAttributeError(const char *attribute) {
#if PY_MAJOR_VERSION >= 3
  PyObject *message = PyUnicode_FromFormat(
#else
  PyObject *message = PyString_FromFormat(
#endif
    "can't delete %s attribute", attribute);

  PyErr_SetObject(PyExc_TypeError, message);
  return -1;
}

/**
 * Raises a TypeError of the form: Arguments must match: <list of overloads>
 *
 * However, in release builds, this instead is defined to a function that just
 * prints out a generic message, to help reduce the amount of strings in the
 * compiled library.
 *
 * If there is already an exception set, does nothing.
 *
 * Always returns NULL so that it can be conveniently used as a return
 * expression for wrapper functions that return a PyObject pointer.
 */
PyObject *_Dtool_Raise_BadArgumentsError(const char *message) {
  if (!PyErr_Occurred()) {
    return PyErr_Format(PyExc_TypeError, "Arguments must match:\n%s", message);
  }
  return nullptr;
}

/**
 * Overload that prints a generic message instead.
 */
PyObject *_Dtool_Raise_BadArgumentsError() {
  if (!PyErr_Occurred()) {
    PyErr_SetString(PyExc_TypeError, "arguments do not match any function overload");
  }
  return nullptr;
}

/**
 * Overload that returns -1 instead of nullptr.
 */
int _Dtool_Raise_BadArgumentsError_Int(const char *message) {
  if (!PyErr_Occurred()) {
    PyErr_Format(PyExc_TypeError, "Arguments must match:\n%s", message);
  }
  return -1;
}

/**
 * Overload that returns -1 instead of nullptr and prints a generic message.
 */
int _Dtool_Raise_BadArgumentsError_Int() {
  if (!PyErr_Occurred()) {
    PyErr_SetString(PyExc_TypeError, "arguments do not match any function overload");
  }
  return -1;
}

/**

 */
PyObject *DTool_CreatePyInstanceTyped(void *local_this_in, Dtool_PyTypedObject &known_class_type, bool memory_rules, bool is_const, int type_index) {
  // We can't do the NULL check here like in DTool_CreatePyInstance, since the
  // caller will have to get the type index to pass to this function to begin
  // with.  That code probably would have crashed by now if it was really NULL
  // for whatever reason.
  nassertr(local_this_in != nullptr, nullptr);

  // IF the class is possibly a run time typed object
  if (type_index > 0) {
    // get best fit class...
    Dtool_PyInstDef *self = (Dtool_PyInstDef *)TypeHandle::from_index(type_index).wrap_python(local_this_in, &known_class_type._PyType);
    if (self != nullptr) {
      self->_memory_rules = memory_rules;
      self->_is_const = is_const;
      return (PyObject *)self;
    }
  }

  // if we get this far .. just wrap the thing in the known type ?? better
  // than aborting...I guess....
  Dtool_PyInstDef *self = (Dtool_PyInstDef *)PyType_GenericAlloc(&known_class_type._PyType, 0);
  if (self != nullptr) {
    self->_signature = PY_PANDA_SIGNATURE;
    self->_My_Type = &known_class_type;
    self->_ptr_to_object = local_this_in;
    self->_memory_rules = memory_rules;
    self->_is_const = is_const;
  }
  return (PyObject *)self;
}

// DTool_CreatePyInstance .. wrapper function to finalize the existance of a
// general dtool py instance..
PyObject *DTool_CreatePyInstance(void *local_this, Dtool_PyTypedObject &in_classdef, bool memory_rules, bool is_const) {
  if (local_this == nullptr) {
    // This is actually a very common case, so let's allow this, but return
    // Py_None consistently.  This eliminates code in the wrappers.
    return Py_NewRef(Py_None);
  }

  Dtool_PyInstDef *self = (Dtool_PyInstDef *)PyType_GenericAlloc(&in_classdef._PyType, 0);
  if (self != nullptr) {
    self->_signature = PY_PANDA_SIGNATURE;
    self->_My_Type = &in_classdef;
    self->_ptr_to_object = local_this;
    self->_memory_rules = memory_rules;
    self->_is_const = is_const;
    self->_My_Type = &in_classdef;
  }
  return (PyObject *)self;
}

#endif  // HAVE_PYTHON
