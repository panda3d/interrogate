/**
 * @file py_support.cxx
 * @author rdb
 * @date 2025-09-03
 */

#include "py_support.h"

#ifdef HAVE_PYTHON

#define _STRINGIFY_VERSION(a, b) (#a "." #b)
#define STRINGIFY_VERSION(a, b) _STRINGIFY_VERSION(a, b)

/**
 * This is similar to a PyErr_Occurred() check, except that it also checks
 * Notify to see if an assertion has occurred.  If that is the case, then it
 * raises an AssertionError.
 *
 * Returns true if there is an active exception, false otherwise.
 *
 * In the NDEBUG case, this is simply a #define to PyErr_Occurred().
 */
bool _Dtool_CheckErrorOccurred() {
  if (PyErr_Occurred()) {
    return true;
  }
  if (Notify::ptr()->has_assert_failed()) {
    Dtool_Raise_AssertionError();
    return true;
  }
  return false;
}

/**
 * Convenience method that checks for exceptions, and if one occurred, returns
 * NULL, otherwise Py_None.
 */
PyObject *_Dtool_Return_None() {
  if (UNLIKELY(PyErr_Occurred())) {
    return nullptr;
  }
#ifndef NDEBUG
  if (UNLIKELY(Notify::ptr()->has_assert_failed())) {
    return Dtool_Raise_AssertionError();
  }
#endif
  return Py_NewRef(Py_None);
}

/**
 * Convenience method that checks for exceptions, and if one occurred, returns
 * NULL, otherwise the given boolean value as a PyObject *.
 */
PyObject *Dtool_Return_Bool(bool value) {
  if (UNLIKELY(PyErr_Occurred())) {
    return nullptr;
  }
#ifndef NDEBUG
  if (UNLIKELY(Notify::ptr()->has_assert_failed())) {
    return Dtool_Raise_AssertionError();
  }
#endif
  return Py_NewRef(value ? Py_True : Py_False);
}

/**
 * Convenience method that checks for exceptions, and if one occurred, returns
 * NULL, otherwise the given return value.  Its reference count is not
 * increased.
 */
PyObject *_Dtool_Return(PyObject *value) {
  if (UNLIKELY(PyErr_Occurred())) {
    return nullptr;
  }
#ifndef NDEBUG
  if (UNLIKELY(Notify::ptr()->has_assert_failed())) {
    return Dtool_Raise_AssertionError();
  }
#endif
  return value;
}

#if PY_VERSION_HEX < 0x03040000
/**
 * This function converts an int value to the appropriate enum instance.
 */
static PyObject *Dtool_EnumType_New(PyTypeObject *subtype, PyObject *args, PyObject *kwds) {
  PyObject *arg;
  if (!Dtool_ExtractArg(&arg, args, kwds, "value")) {
    return PyErr_Format(PyExc_TypeError,
                        "%s() missing 1 required argument: 'value'",
                        subtype->tp_name);
  }

  if (Py_IS_TYPE(arg, subtype)) {
    return Py_NewRef(arg);
  }

  PyObject *value2member = PyDict_GetItemString(subtype->tp_dict, "_value2member_map_");
  nassertr_always(value2member != nullptr, nullptr);

  PyObject *member;
  if (PyDict_GetItemRef(value2member, arg, &member) > 0) {
    return member;
  }

  PyObject *repr = PyObject_Repr(arg);
  PyErr_Format(PyExc_ValueError, "%s is not a valid %s",
#if PY_MAJOR_VERSION >= 3
               PyUnicode_AS_STRING(repr),
#else
               PyString_AS_STRING(repr),
#endif
               subtype->tp_name);
  Py_DECREF(repr);
  return nullptr;
}

static PyObject *Dtool_EnumType_Str(PyObject *self) {
  PyObject *name = PyObject_GetAttrString(self, "name");
#if PY_MAJOR_VERSION >= 3
  PyObject *repr = PyUnicode_FromFormat("%s.%s", Py_TYPE(self)->tp_name, PyString_AS_STRING(name));
#else
  PyObject *repr = PyString_FromFormat("%s.%s", Py_TYPE(self)->tp_name, PyString_AS_STRING(name));
#endif
  Py_DECREF(name);
  return repr;
}

static PyObject *Dtool_EnumType_Repr(PyObject *self) {
  PyObject *name = PyObject_GetAttrString(self, "name");
  PyObject *value = PyObject_GetAttrString(self, "value");
#if PY_MAJOR_VERSION >= 3
  PyObject *repr = PyUnicode_FromFormat("<%s.%s: %ld>", Py_TYPE(self)->tp_name, PyString_AS_STRING(name), PyLongOrInt_AS_LONG(value));
#else
  PyObject *repr = PyString_FromFormat("<%s.%s: %ld>", Py_TYPE(self)->tp_name, PyString_AS_STRING(name), PyLongOrInt_AS_LONG(value));
#endif
  Py_DECREF(name);
  Py_DECREF(value);
  return repr;
}
#endif

/**
 * Creates a Python 3.4-style enum type.  Steals reference to 'names', which
 * should be a tuple of (name, value) pairs.
 */
PyTypeObject *Dtool_EnumType_Create(const char *name, PyObject *names, const char *module) {
#if PY_VERSION_HEX >= 0x03040000
  PyObject *enum_module = PyImport_ImportModule("enum");
  nassertr_always(enum_module != nullptr, nullptr);

  PyObject *enum_meta = PyObject_GetAttrString(enum_module, "EnumMeta");
  nassertr(enum_meta != nullptr, nullptr);

  PyObject *enum_class = PyObject_GetAttrString(enum_module, "Enum");
  Py_DECREF(enum_module);
  nassertr(enum_class != nullptr, nullptr);

  PyObject *enum_create = PyObject_GetAttrString(enum_meta, "_create_");
  Py_DECREF(enum_meta);

  PyObject *result = PyObject_CallFunction(enum_create, (char *)"OsN", enum_class, name, names);
  Py_DECREF(enum_create);
  Py_DECREF(enum_class);
  nassertr(result != nullptr, nullptr);
#else
  static PyObject *enum_class = nullptr;
  static PyObject *name_str;
  static PyObject *name_sunder_str;
  static PyObject *value_str;
  static PyObject *value_sunder_str;
  static PyObject *value2member_map_sunder_str;
  // Emulate something vaguely like the enum module.
  if (enum_class == nullptr) {
#if PY_MAJOR_VERSION >= 3
    name_str = PyUnicode_InternFromString("name");
    value_str = PyUnicode_InternFromString("value");
    name_sunder_str = PyUnicode_InternFromString("_name_");
    value_sunder_str = PyUnicode_InternFromString("_value_");
    value2member_map_sunder_str = PyUnicode_InternFromString("_value2member_map_");
#else
    name_str = PyString_InternFromString("name");
    value_str = PyString_InternFromString("value");
    name_sunder_str = PyString_InternFromString("_name_");
    value_sunder_str = PyString_InternFromString("_value_");
    value2member_map_sunder_str = PyString_InternFromString("_value2member_map_");
#endif
    PyObject *name_value_tuple = PyTuple_New(4);
    PyTuple_SET_ITEM(name_value_tuple, 0, Py_NewRef(name_str));
    PyTuple_SET_ITEM(name_value_tuple, 1, Py_NewRef(value_str));
    PyTuple_SET_ITEM(name_value_tuple, 2, name_sunder_str);
    PyTuple_SET_ITEM(name_value_tuple, 3, value_sunder_str);

    PyObject *slots_dict = PyDict_New();
    PyDict_SetItemString(slots_dict, "__slots__", name_value_tuple);
    Py_DECREF(name_value_tuple);

    enum_class = PyObject_CallFunction((PyObject *)&PyType_Type, (char *)"s()N", "Enum", slots_dict);
    nassertr(enum_class != nullptr, nullptr);
  }

  // Create a subclass of this generic Enum class we just created.
  PyObject *value2member = PyDict_New();
  PyObject *dict = PyDict_New();
  PyDict_SetItem(dict, value2member_map_sunder_str, value2member);
  PyObject *result = PyObject_CallFunction((PyObject *)&PyType_Type, (char *)"s(O)N", name, enum_class, dict);
  nassertr(result != nullptr, nullptr);

  ((PyTypeObject *)result)->tp_new = Dtool_EnumType_New;
  ((PyTypeObject *)result)->tp_str = Dtool_EnumType_Str;
  ((PyTypeObject *)result)->tp_repr = Dtool_EnumType_Repr;

  PyObject *empty_tuple = PyTuple_New(0);

  // Copy the names as instances of the above to the class dict, and create a
  // reverse mapping in the _value2member_map_ dict.
  Py_ssize_t size = PyTuple_GET_SIZE(names);
  for (Py_ssize_t i = 0; i < size; ++i) {
    PyObject *item = PyTuple_GET_ITEM(names, i);
    PyObject *name = PyTuple_GET_ITEM(item, 0);
    PyObject *value = PyTuple_GET_ITEM(item, 1);
    PyObject *member = PyType_GenericNew((PyTypeObject *)result, empty_tuple, nullptr);
    PyObject_SetAttr(member, name_str, name);
    PyObject_SetAttr(member, name_sunder_str, name);
    PyObject_SetAttr(member, value_str, value);
    PyObject_SetAttr(member, value_sunder_str, value);
    PyObject_SetAttr(result, name, member);
    PyDict_SetItem(value2member, value, member);
    Py_DECREF(member);
  }
  Py_DECREF(names);
  Py_DECREF(value2member);
  Py_DECREF(empty_tuple);
#endif

  if (module != nullptr) {
    PyObject *modstr = PyUnicode_FromString(module);
    PyObject_SetAttrString(result, "__module__", modstr);
    Py_DECREF(modstr);
  }
  nassertr(PyType_Check(result), nullptr);
  return (PyTypeObject *)result;
}

/**
 * Returns a borrowed reference to the global type dictionary.
 */
Dtool_TypeMap *Dtool_GetGlobalTypeMap() {
#if PY_VERSION_HEX >= 0x030d0000 // 3.13
  PyObject *istate_dict = PyInterpreterState_GetDict(PyInterpreterState_Get());
  PyObject *key = PyUnicode_InternFromString("_interrogate_types");
  PyObject *capsule = PyDict_GetItem(istate_dict, key);
  if (capsule != nullptr) {
    Py_DECREF(key);
    return (Dtool_TypeMap *)PyCapsule_GetPointer(capsule, nullptr);
  }
#else
  PyObject *capsule = PySys_GetObject((char *)"_interrogate_types");
  if (capsule != nullptr) {
    return (Dtool_TypeMap *)PyCapsule_GetPointer(capsule, nullptr);
  }
#endif

  Dtool_TypeMap *type_map = new Dtool_TypeMap;
  capsule = PyCapsule_New((void *)type_map, nullptr, nullptr);

#if PY_VERSION_HEX >= 0x030d0000 // 3.13
  PyObject *result;
  if (PyDict_SetDefaultRef(istate_dict, key, capsule, &result) != 0) {
    // Another thread already beat us to it.
    Py_DECREF(capsule);
    delete type_map;
    capsule = result;
    type_map = (Dtool_TypeMap *)PyCapsule_GetPointer(capsule, nullptr);
  }
  Py_DECREF(key);
#endif

  PySys_SetObject((char *)"_interrogate_types", capsule);
  Py_DECREF(capsule);
  return type_map;
}

/**
 *
 */
void DtoolProxy_Init(DtoolProxy *proxy, PyObject *self,
                     Dtool_PyTypedObject &classdef,
                     TypeRegistry::PythonWrapFunc *wrap_func) {
  if (proxy == nullptr) {
    // Out of memory, the generated code will handle this.
    return;
  }

  proxy->_self = Py_NewRef(self);
  PyTypeObject *cls = Py_TYPE(self);
  if (cls != &classdef._PyType) {
    TypeRegistry *registry = TypeRegistry::ptr();
    TypeHandle handle = registry->register_dynamic_type(cls->tp_name);
    registry->record_derivation(handle, classdef._type);
    //TODO unregister type when it is unloaded? weak callback?
    PyTypeObject *cls_ref = (PyTypeObject *)Py_NewRef((PyObject *)cls);
    registry->record_python_type(handle, cls_ref, wrap_func);
    proxy->_type = handle;
  } else {
    proxy->_type = classdef._type;
  }
}

#define PY_MAJOR_VERSION_STR #PY_MAJOR_VERSION "." #PY_MINOR_VERSION

#if PY_MAJOR_VERSION >= 3
PyObject *Dtool_PyModuleInitHelper(const LibraryDef *defs[], PyModuleDef *module_def) {
#else
PyObject *Dtool_PyModuleInitHelper(const LibraryDef *defs[], const char *modulename) {
#endif
  // Check the version so we can print a helpful error if it doesn't match.
  std::string version = Py_GetVersion();
  size_t version_len = version.find('.', 2);
  if (version_len != std::string::npos) {
    version.resize(version_len);
  }

  if (version != STRINGIFY_VERSION(PY_MAJOR_VERSION, PY_MINOR_VERSION)) {
    // Raise a helpful error message.  We can safely do this because the
    // signature and behavior for PyErr_SetString has remained consistent.
    std::ostringstream errs;
    errs << "this module was compiled for Python "
         << PY_MAJOR_VERSION << "." << PY_MINOR_VERSION << ", which is "
         << "incompatible with Python " << version;
    std::string error = errs.str();
    PyErr_SetString(PyExc_ImportError, error.c_str());
    return nullptr;
  }

  Dtool_TypeMap *type_map = Dtool_GetGlobalTypeMap();

#ifdef Py_GIL_DISABLED
  PyMutex_Lock(&type_map->_lock);
#endif

  // the module level function inits....
  MethodDefmap functions;
  for (size_t i = 0; defs[i] != nullptr; i++) {
    const LibraryDef &def = *defs[i];

    // Accumulate method definitions.
    for (PyMethodDef *meth = def._methods; meth->ml_name != nullptr; meth++) {
      if (functions.find(meth->ml_name) == functions.end()) {
        functions[meth->ml_name] = meth;
      }
    }

    // Define exported types.
    const Dtool_TypeDef *types = def._types;
    if (types != nullptr) {
      while (types->name != nullptr) {
        (*type_map)[std::string(types->name)] = types->type;
        ++types;
      }
    }
  }

  // Resolve external types, in a second pass.
  for (size_t i = 0; defs[i] != nullptr; i++) {
    const LibraryDef &def = *defs[i];

    Dtool_TypeDef *types = def._external_types;
    if (types != nullptr) {
      while (types->name != nullptr) {
        auto it = type_map->find(std::string(types->name));
        if (it != type_map->end()) {
          types->type = it->second;
        } else {
          PyErr_Format(PyExc_NameError, "name '%s' is not defined", types->name);
#ifdef Py_GIL_DISABLED
          PyMutex_Unlock(&type_map->_lock);
#endif
          return nullptr;
        }
        ++types;
      }
    }
  }
#ifdef Py_GIL_DISABLED
  PyMutex_Unlock(&type_map->_lock);
#endif

  PyMethodDef *newdef = new PyMethodDef[functions.size() + 1];
  MethodDefmap::iterator mi;
  int offset = 0;
  for (mi = functions.begin(); mi != functions.end(); mi++, offset++) {
    newdef[offset] = *mi->second;
  }
  newdef[offset].ml_doc = nullptr;
  newdef[offset].ml_name = nullptr;
  newdef[offset].ml_meth = nullptr;
  newdef[offset].ml_flags = 0;

#if PY_MAJOR_VERSION >= 3
  module_def->m_methods = newdef;
  PyObject *module = PyModule_Create(module_def);
#else
  PyObject *module = Py_InitModule((char *)modulename, newdef);
#endif

  if (module == nullptr) {
#if PY_MAJOR_VERSION >= 3
    return Dtool_Raise_TypeError("PyModule_Create returned NULL");
#else
    return Dtool_Raise_TypeError("Py_InitModule returned NULL");
#endif
  }

  PyModule_AddIntConstant(module, "Dtool_PyNativeInterface", 1);
  return module;
}

// HACK.... Be careful Dtool_BorrowThisReference This function can be used to
// grab the "THIS" pointer from an object and use it Required to support
// historical inheritance in the form of "is this instance of"..
PyObject *Dtool_BorrowThisReference(PyObject *self, PyObject *args) {
  PyObject *from_in = nullptr;
  PyObject *to_in = nullptr;
  if (PyArg_UnpackTuple(args, "Dtool_BorrowThisReference", 2, 2, &to_in, &from_in)) {

    if (DtoolInstance_Check(from_in) && DtoolInstance_Check(to_in)) {
      Dtool_PyInstDef *from = (Dtool_PyInstDef *) from_in;
      Dtool_PyInstDef *to = (Dtool_PyInstDef *) to_in;

      // if (PyObject_TypeCheck(to_in, Py_TYPE(from_in))) {
      if (from->_My_Type == to->_My_Type) {
        to->_memory_rules = false;
        to->_is_const = from->_is_const;
        to->_ptr_to_object = from->_ptr_to_object;

        return Py_NewRef(Py_None);
      }

      return PyErr_Format(PyExc_TypeError, "types %s and %s do not match",
                          Py_TYPE(from)->tp_name, Py_TYPE(to)->tp_name);
    } else {
      return Dtool_Raise_TypeError("One of these does not appear to be DTOOL Instance ??");
    }
  }
  return nullptr;
}

// We do expose a dictionay for dtool classes .. this should be removed at
// some point..
EXPCL_PYPANDA PyObject *
Dtool_AddToDictionary(PyObject *self1, PyObject *args) {
  PyObject *self;
  PyObject *subject;
  PyObject *key;
  if (PyArg_ParseTuple(args, "OSO", &self, &key, &subject)) {
    PyObject *dict = ((PyTypeObject *)self)->tp_dict;
    if (dict == nullptr || !PyDict_Check(dict)) {
      return Dtool_Raise_TypeError("No dictionary On Object");
    } else {
      PyDict_SetItem(dict, key, subject);
    }
  }
  if (PyErr_Occurred()) {
    return nullptr;
  }
  return Py_NewRef(Py_None);
}

/**
 * This is a support function for a synthesized __copy__() method from a C++
 * make_copy() method.
 */
PyObject *copy_from_make_copy(PyObject *self, PyObject *noargs) {
  PyObject *callable = PyObject_GetAttrString(self, "make_copy");
  if (callable == nullptr) {
    return nullptr;
  }
  PyObject *result = PyObject_CallNoArgs(callable);
  Py_DECREF(callable);
  return result;
}

/**
 * This is a support function for a synthesized __copy__() method from a C++
 * copy constructor.
 */
PyObject *copy_from_copy_constructor(PyObject *self, PyObject *noargs) {
  PyObject *callable = (PyObject *)Py_TYPE(self);
  return PyObject_CallOneArg(callable, self);
}

/**
 * This is a support function for a synthesized __deepcopy__() method for any
 * class that has a __copy__() method.  The sythethic method simply invokes
 * __copy__().
 */
PyObject *map_deepcopy_to_copy(PyObject *self, PyObject *args) {
  PyObject *callable = PyObject_GetAttrString(self, "__copy__");
  if (callable == nullptr) {
    return nullptr;
  }
  PyObject *result = PyObject_CallNoArgs(callable);
  Py_DECREF(callable);
  return result;
}

/**
 * A more efficient version of PyArg_ParseTupleAndKeywords for the special
 * case where there is only a single PyObject argument.
 */
bool Dtool_ExtractArg(PyObject **result, PyObject *args, PyObject *kwds,
                      const char *keyword) {

  if (PyTuple_GET_SIZE(args) == 1) {
    if (kwds == nullptr || PyDict_GET_SIZE(kwds) == 0) {
      *result = PyTuple_GET_ITEM(args, 0);
      return true;
    }
  }
  else if (!keyword || !keyword[0]) {
    return false;
  }
  else if (PyTuple_GET_SIZE(args) == 0) {
    PyObject *key;
    Py_ssize_t ppos = 0;
    if (kwds != nullptr && PyDict_GET_SIZE(kwds) == 1 &&
        PyDict_Next(kwds, &ppos, &key, result)) {
      // We got the item, we just need to make sure that it had the right key.
#if PY_MAJOR_VERSION >= 3
      return PyUnicode_CheckExact(key) && PyUnicode_CompareWithASCIIString(key, keyword) == 0;
#else
      return PyString_CheckExact(key) && strcmp(PyString_AS_STRING(key), keyword) == 0;
#endif
    }
  }

  return false;
}

/**
 * Variant of Dtool_ExtractArg that does not accept a keyword argument.
 */
bool Dtool_ExtractArg(PyObject **result, PyObject *args, PyObject *kwds) {
  if (PyTuple_GET_SIZE(args) == 1 &&
      (kwds == nullptr || PyDict_GET_SIZE(kwds) == 0)) {
    *result = PyTuple_GET_ITEM(args, 0);
    return true;
  }
  return false;
}

/**
 * A more efficient version of PyArg_ParseTupleAndKeywords for the special
 * case where there is only a single optional PyObject argument.
 *
 * Returns true if valid (including if there were 0 items), false if there was
 * an error, such as an invalid number of parameters.
 */
bool Dtool_ExtractOptionalArg(PyObject **result, PyObject *args, PyObject *kwds,
                              const char *keyword) {

  if (PyTuple_GET_SIZE(args) == 1) {
    if (kwds == nullptr || PyDict_GET_SIZE(kwds) == 0) {
      *result = PyTuple_GET_ITEM(args, 0);
      return true;
    }
  }
  else if (!keyword || !keyword[0]) {
    return (kwds == nullptr || PyDict_GET_SIZE(kwds) == 0);
  }
  else if (PyTuple_GET_SIZE(args) == 0) {
    if (kwds != nullptr && PyDict_GET_SIZE(kwds) == 1) {
      PyObject *key;
      Py_ssize_t ppos = 0;
      if (!PyDict_Next(kwds, &ppos, &key, result)) {
        return true;
      }

      // We got the item, we just need to make sure that it had the right key.
#if PY_VERSION_HEX >= 0x030d0000
      return PyUnicode_CheckExact(key) && PyUnicode_EqualToUTF8(key, keyword);
#elif PY_VERSION_HEX >= 0x03060000
      return PyUnicode_CheckExact(key) && _PyUnicode_EqualToASCIIString(key, keyword);
#elif PY_MAJOR_VERSION >= 3
      return PyUnicode_CheckExact(key) && PyUnicode_CompareWithASCIIString(key, keyword) == 0;
#else
      return PyString_CheckExact(key) && strcmp(PyString_AS_STRING(key), keyword) == 0;
#endif
    } else {
      return true;
    }
  }

  return false;
}

/**
 * Variant of Dtool_ExtractOptionalArg that does not accept a keyword argument.
 */
bool Dtool_ExtractOptionalArg(PyObject **result, PyObject *args, PyObject *kwds) {
  if (kwds != nullptr && PyDict_GET_SIZE(kwds) != 0) {
    return false;
  }
  if (PyTuple_GET_SIZE(args) == 1) {
    *result = PyTuple_GET_ITEM(args, 0);
    return true;
  }
  return (PyTuple_GET_SIZE(args) == 0);
}

#endif  // HAVE_PYTHON
