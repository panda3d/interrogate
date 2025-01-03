/**
 * @file py_wrappers.cxx
 * @author rdb
 * @date 2017-11-26
 */

#include "py_wrappers.h"

#ifdef HAVE_PYTHON

#if PY_VERSION_HEX >= 0x03040000
#define _COLLECTIONS_ABC "_collections_abc"
#elif PY_VERSION_HEX >= 0x03030000
#define _COLLECTIONS_ABC "collections.abc"
#else
#define _COLLECTIONS_ABC "_abcoll"
#endif

#ifndef Py_TPFLAGS_IMMUTABLETYPE
#define Py_TPFLAGS_IMMUTABLETYPE 0
#endif

/**
 * Registers the type with collections.abc.
 */
static void
_register_collection(PyObject *type, const char *abc) {
#if PY_MAJOR_VERSION >= 3
  PyObject *module_name = PyUnicode_InternFromString(_COLLECTIONS_ABC);
#else
  PyObject *module_name = PyString_InternFromString(_COLLECTIONS_ABC);
#endif
  PyObject *module = PyImport_GetModule(module_name);
  Py_DECREF(module_name);
  if (module != nullptr) {
    PyObject *dict = PyModule_GetDict(module);
    if (dict != nullptr) {
#if PY_MAJOR_VERSION >= 3
      PyObject *register_str = PyUnicode_InternFromString("register");
#else
      PyObject *register_str = PyString_InternFromString("register");
#endif
      PyObject *obj = nullptr;
      if (register_str == nullptr ||
          PyDict_GetItemStringRef(dict, abc, &obj) <= 0 ||
          PyObject_CallMethodOneArg(obj, register_str, type) == nullptr) {
        PyErr_Print();
      }
      Py_XDECREF(obj);
      Py_XDECREF(register_str);
    } else {
      PyErr_Clear();
    }
    Py_DECREF(module);
  }
}

/**
 * Creates the given type from a spec, unless it's already been created in
 * this interpreter before.  If abc is not null, registers the type with
 * the given collections.abc type.
 *
 * Does not increment the reference count of the result.
 */
static PyObject *
_init_wrapper_type(PyType_Spec &spec, const char *abc = nullptr) {
#if PY_VERSION_HEX >= 0x030d0000 // 3.13
  PyObject *istate_dict = PyInterpreterState_GetDict(PyInterpreterState_Get());
  PyObject *key_obj = PyUnicode_FromString(spec.name);

  PyObject *type;
  Py_BEGIN_CRITICAL_SECTION(istate_dict);
  type = PyDict_GetItem(istate_dict, key_obj);
  if (type == nullptr) {
    type = PyType_FromSpec(&spec);
    if (type != nullptr) {
      PyDict_SetItem(istate_dict, key_obj, type);
      Py_DECREF(type);

      if (abc != nullptr) {
        _register_collection(type, abc);
      }
    }
  }
  Py_END_CRITICAL_SECTION();
  Py_DECREF(key_obj);
#else
  PyObject *type = PySys_GetObject(spec.name);
  if (type == nullptr) {
    type = PyType_FromSpec(&spec);
    if (type != nullptr) {
      PySys_SetObject(spec.name, type);
      Py_DECREF(type);

      // If the collections.abc module is loaded, register this as a subclass.
      if (abc != nullptr) {
        _register_collection(type, abc);
      }
    }
  }
#endif
  return type;
}

/**
 * These classes are returned from properties that require a subscript
 * interface, ie. something.children[i] = 3.
 */
static void Dtool_WrapperBase_dealloc(PyObject *self) {
  Dtool_WrapperBase *wrap = (Dtool_WrapperBase *)self;
  nassertv(wrap);
  PyTypeObject *tp = Py_TYPE(self);
  Py_XDECREF(wrap->_self);
  tp->tp_free(self);
  Py_DECREF(tp);
}

static PyObject *Dtool_WrapperBase_repr(PyObject *self) {
  Dtool_WrapperBase *wrap = (Dtool_WrapperBase *)self;
  nassertr(wrap, nullptr);

  PyObject *repr = PyObject_Repr(wrap->_self);
  PyObject *result;
#if PY_MAJOR_VERSION >= 3
  result = PyUnicode_FromFormat("<%s[] of %s>", wrap->_name, PyUnicode_AsUTF8(repr));
#else
  result = PyString_FromFormat("<%s[] of %s>", wrap->_name, PyString_AS_STRING(repr));
#endif
  Py_DECREF(repr);
  return result;
}

static PyObject *Dtool_SequenceWrapper_repr(PyObject *self) {
  Dtool_SequenceWrapper *wrap = (Dtool_SequenceWrapper *)self;
  nassertr(wrap, nullptr);

  Py_ssize_t len = -1;
  if (wrap->_len_func != nullptr) {
    len = wrap->_len_func(wrap->_base._self);
  }

  if (len < 0) {
    PyErr_Clear();
    return Dtool_WrapperBase_repr(self);
  }

  PyObject *repr = PyObject_Repr(wrap->_base._self);
  PyObject *result;
#if PY_MAJOR_VERSION >= 3
  result = PyUnicode_FromFormat("<%s[%zd] of %s>", wrap->_base._name, len, PyUnicode_AsUTF8(repr));
#else
  result = PyString_FromFormat("<%s[%zd] of %s>", wrap->_base._name, len, PyString_AS_STRING(repr));
#endif
  Py_DECREF(repr);
  return result;
}

static Py_ssize_t Dtool_SequenceWrapper_length(PyObject *self) {
  Dtool_SequenceWrapper *wrap = (Dtool_SequenceWrapper *)self;
  nassertr(wrap, -1);
  if (wrap->_len_func != nullptr) {
    return wrap->_len_func(wrap->_base._self);
  } else {
    Dtool_Raise_TypeError("property does not support len()");
    return -1;
  }
}

static PyObject *Dtool_SequenceWrapper_getitem(PyObject *self, Py_ssize_t index) {
  Dtool_SequenceWrapper *wrap = (Dtool_SequenceWrapper *)self;
  nassertr(wrap, nullptr);
  nassertr(wrap->_getitem_func, nullptr);
  return wrap->_getitem_func(wrap->_base._self, index);
}

/**
 * Implementation of (x in property)
 */
static int Dtool_SequenceWrapper_contains(PyObject *self, PyObject *value) {
  Dtool_SequenceWrapper *wrap = (Dtool_SequenceWrapper *)self;
  nassertr(wrap, -1);
  nassertr(wrap->_len_func, -1);
  nassertr(wrap->_getitem_func, -1);

  Py_ssize_t length = wrap->_len_func(wrap->_base._self);

  // Iterate through the items, invoking the equality function for each, until
  // we have found the matching one.
  for (Py_ssize_t index = 0; index < length; ++index) {
    PyObject *item = wrap->_getitem_func(wrap->_base._self, index);
    if (item != nullptr) {
      int cmp = PyObject_RichCompareBool(item, value, Py_EQ);
      if (cmp > 0) {
        return 1;
      }
      if (cmp < 0) {
        return -1;
      }
    } else {
      return -1;
    }
  }
  return 0;
}

/**
 * Implementation of property.index(x) which returns the index of the first
 * occurrence of x in the sequence, or raises a ValueError if it isn't found.
 */
static PyObject *Dtool_SequenceWrapper_index(PyObject *self, PyObject *value) {
  Dtool_SequenceWrapper *wrap = (Dtool_SequenceWrapper *)self;
  nassertr(wrap, nullptr);
  nassertr(wrap->_len_func, nullptr);
  nassertr(wrap->_getitem_func, nullptr);

  Py_ssize_t length = wrap->_len_func(wrap->_base._self);

  // Iterate through the items, invoking the equality function for each, until
  // we have found the right one.
  for (Py_ssize_t index = 0; index < length; ++index) {
    PyObject *item = wrap->_getitem_func(wrap->_base._self, index);
    if (item != nullptr) {
      int cmp = PyObject_RichCompareBool(item, value, Py_EQ);
      if (cmp > 0) {
        return Dtool_WrapValue(index);
      }
      if (cmp < 0) {
        return nullptr;
      }
    } else {
      return nullptr;
    }
  }
  // Not found, raise ValueError.
  return PyErr_Format(PyExc_ValueError, "%s.index() did not find value", wrap->_base._name);
}

/**
 * Implementation of property.count(x) which returns the number of occurrences
 * of x in the sequence.
 */
static PyObject *Dtool_SequenceWrapper_count(PyObject *self, PyObject *value) {
  Dtool_SequenceWrapper *wrap = (Dtool_SequenceWrapper *)self;
  nassertr(wrap, nullptr);
  Py_ssize_t index = 0;
  if (wrap->_len_func != nullptr) {
    index = wrap->_len_func(wrap->_base._self);
  } else {
    return Dtool_Raise_TypeError("property does not support count()");
  }
  // Iterate through the items, invoking the == operator for each.
  long count = 0;
  nassertr(wrap->_getitem_func, nullptr);
  while (index > 0) {
    --index;
    PyObject *item = wrap->_getitem_func(wrap->_base._self, index);
    if (item == nullptr) {
      return nullptr;
    }
    int cmp = PyObject_RichCompareBool(item, value, Py_EQ);
    if (cmp > 0) {
      ++count;
    }
    if (cmp < 0) {
      return nullptr;
    }
  }
#if PY_MAJOR_VERSION >= 3
  return PyLong_FromLong(count);
#else
  return PyInt_FromLong(count);
#endif
}

/**
 * Implementation of `property[i] = x`
 */
static int Dtool_MutableSequenceWrapper_setitem(PyObject *self, Py_ssize_t index, PyObject *value) {
  Dtool_MutableSequenceWrapper *wrap = (Dtool_MutableSequenceWrapper *)self;
  nassertr(wrap, -1);
  if (wrap->_setitem_func != nullptr) {
    return wrap->_setitem_func(wrap->_base._self, index, value);
  } else {
    Dtool_Raise_TypeError("property does not support item assignment");
    return -1;
  }
}

/**
 * Implementation of property.clear() which removes all elements in the
 * sequence, starting with the last.
 */
static PyObject *Dtool_MutableSequenceWrapper_clear(PyObject *self, PyObject *) {
  Dtool_MutableSequenceWrapper *wrap = (Dtool_MutableSequenceWrapper *)self;
  nassertr(wrap, nullptr);
  Py_ssize_t index = 0;
  if (wrap->_len_func != nullptr && wrap->_setitem_func != nullptr) {
    index = wrap->_len_func(wrap->_base._self);
  } else {
    return Dtool_Raise_TypeError("property does not support clear()");
  }

  // Iterate through the items, invoking the delete function for each.  We do
  // this in reverse order, which may be more efficient.
  while (index > 0) {
    --index;
    if (wrap->_setitem_func(wrap->_base._self, index, nullptr) != 0) {
      return nullptr;
    }
  }
  return Py_NewRef(Py_None);
}

/**
 * Implementation of property.remove(x) which removes the first occurrence of
 * x in the sequence, or raises a ValueError if it isn't found.
 */
static PyObject *Dtool_MutableSequenceWrapper_remove(PyObject *self, PyObject *value) {
  Dtool_MutableSequenceWrapper *wrap = (Dtool_MutableSequenceWrapper *)self;
  nassertr(wrap, nullptr);
  Py_ssize_t length = 0;
  if (wrap->_len_func != nullptr && wrap->_setitem_func != nullptr) {
    length = wrap->_len_func(wrap->_base._self);
  } else {
    return Dtool_Raise_TypeError("property does not support remove()");
  }

  // Iterate through the items, invoking the equality function for each, until
  // we have found the right one.
  nassertr(wrap->_getitem_func, nullptr);
  for (Py_ssize_t index = 0; index < length; ++index) {
    PyObject *item = wrap->_getitem_func(wrap->_base._self, index);
    if (item != nullptr) {
      int cmp = PyObject_RichCompareBool(item, value, Py_EQ);
      if (cmp > 0) {
        if (wrap->_setitem_func(wrap->_base._self, index, nullptr) == 0) {
          return Py_NewRef(Py_None);
        } else {
          return nullptr;
        }
      }
      if (cmp < 0) {
        return nullptr;
      }
    } else {
      return nullptr;
    }
  }
  // Not found, raise ValueError.
  return PyErr_Format(PyExc_ValueError, "%s.remove() did not find value", wrap->_base._name);
}

/**
 * Implementation of property.pop([i=-1]) which returns and removes the
 * element at the indicated index in the sequence.  If no index is provided,
 * it removes from the end of the list.
 */
static PyObject *Dtool_MutableSequenceWrapper_pop(PyObject *self, PyObject *args) {
  Dtool_MutableSequenceWrapper *wrap = (Dtool_MutableSequenceWrapper *)self;
  nassertr(wrap, nullptr);
  if (wrap->_getitem_func == nullptr || wrap->_setitem_func == nullptr ||
      wrap->_len_func == nullptr) {
    return Dtool_Raise_TypeError("property does not support pop()");
  }

  Py_ssize_t length = wrap->_len_func(wrap->_base._self);
  Py_ssize_t index;
  switch (PyTuple_GET_SIZE(args)) {
  case 0:
    index = length - 1;
    break;
  case 1:
    index = PyNumber_AsSsize_t(PyTuple_GET_ITEM(args, 0), PyExc_IndexError);
    if (index == -1 && PyErr_Occurred()) {
      return nullptr;
    }
    if (index < 0) {
      index += length;
    }
    break;
  default:
    return Dtool_Raise_TypeError("pop([i=-1]) takes 0 or 1 arguments");
  }

  if (length <= 0) {
    return PyErr_Format(PyExc_IndexError, "%s.pop() from empty sequence", wrap->_base._name);
  }

  // Index error will be caught by getitem_func.
  PyObject *value = wrap->_getitem_func(wrap->_base._self, index);
  if (value != nullptr) {
    if (wrap->_setitem_func(wrap->_base._self, index, nullptr) != 0) {
      return nullptr;
    }
    return value;
  }
  return nullptr;
}

/**
 * Implementation of property.append(x) which is an alias for
 * property.insert(len(property), x).
 */
static PyObject *Dtool_MutableSequenceWrapper_append(PyObject *self, PyObject *arg) {
  Dtool_MutableSequenceWrapper *wrap = (Dtool_MutableSequenceWrapper *)self;
  nassertr(wrap, nullptr);
  if (wrap->_insert_func == nullptr) {
    return Dtool_Raise_TypeError("property does not support append()");
  }
  return wrap->_insert_func(wrap->_base._self, (size_t)-1, arg);
}

/**
 * Implementation of property.insert(i, x) which inserts the given item at the
 * given position.
 */
static PyObject *Dtool_MutableSequenceWrapper_insert(PyObject *self, PyObject *args) {
  Dtool_MutableSequenceWrapper *wrap = (Dtool_MutableSequenceWrapper *)self;
  nassertr(wrap, nullptr);
  if (wrap->_insert_func == nullptr) {
    return Dtool_Raise_TypeError("property does not support insert()");
  }
  if (PyTuple_GET_SIZE(args) != 2) {
    return Dtool_Raise_TypeError("insert() takes exactly 2 arguments");
  }
  Py_ssize_t index = PyNumber_AsSsize_t(PyTuple_GET_ITEM(args, 0), PyExc_IndexError);
  if (index == -1 && PyErr_Occurred()) {
    return nullptr;
  }
  if (index < 0) {
    if (wrap->_len_func != nullptr) {
      index += wrap->_len_func(wrap->_base._self);
    } else {
      return PyErr_Format(PyExc_TypeError, "%s.insert() does not support negative indices", wrap->_base._name);
    }
  }
  return wrap->_insert_func(wrap->_base._self, (size_t)std::max(index, (Py_ssize_t)0), PyTuple_GET_ITEM(args, 1));
}

/**
 * Implementation of property.extend(seq) which is equivalent to:
 * @code
 * for x in seq:
 *   property.append(seq)
 * @endcode
 */
static PyObject *Dtool_MutableSequenceWrapper_extend(PyObject *self, PyObject *arg) {
  Dtool_MutableSequenceWrapper *wrap = (Dtool_MutableSequenceWrapper *)self;
  nassertr(wrap, nullptr);
  if (wrap->_insert_func == nullptr) {
    return Dtool_Raise_TypeError("property does not support extend()");
  }
  PyObject *iter = PyObject_GetIter(arg);
  if (iter == nullptr) {
    return nullptr;
  }
  PyObject *next = PyIter_Next(iter);
  PyObject *retval = nullptr;
  while (next != nullptr) {
    retval = wrap->_insert_func(wrap->_base._self, (size_t)-1, next);
    Py_DECREF(next);
    if (retval == nullptr) {
      Py_DECREF(iter);
      return nullptr;
    }
    Py_DECREF(retval);
    next = PyIter_Next(iter);
  }

  Py_DECREF(iter);
  return Py_NewRef(Py_None);
}

/**
 * Implementation of `x in mapping`.
 */
static int Dtool_MappingWrapper_contains(PyObject *self, PyObject *key) {
  Dtool_MappingWrapper *wrap = (Dtool_MappingWrapper *)self;
  nassertr(wrap, -1);
  nassertr(wrap->_getitem_func, -1);
  PyObject *value = wrap->_getitem_func(wrap->_base._self, key);
  if (value != nullptr) {
    Py_DECREF(value);
    return 1;
  } else if (PyErr_ExceptionMatches(PyExc_KeyError) ||
             PyErr_ExceptionMatches(PyExc_TypeError)) {
    PyErr_Clear();
    return 0;
  } else {
    return -1;
  }
}

static PyObject *Dtool_MappingWrapper_getitem(PyObject *self, PyObject *key) {
  Dtool_MappingWrapper *wrap = (Dtool_MappingWrapper *)self;
  nassertr(wrap, nullptr);
  nassertr(wrap->_getitem_func, nullptr);
  return wrap->_getitem_func(wrap->_base._self, key);
}

/**
 * Implementation of iter(property) that returns an iterable over all the
 * keys.
 */
static PyObject *Dtool_MappingWrapper_iter(PyObject *self) {
  Dtool_MappingWrapper *wrap = (Dtool_MappingWrapper *)self;
  nassertr(wrap, nullptr);

  if (wrap->_keys._len_func == nullptr || wrap->_keys._getitem_func == nullptr) {
    return PyErr_Format(PyExc_TypeError, "%s is not iterable", wrap->_base._name);
  }

  Dtool_SequenceWrapper *keys = Dtool_NewSequenceWrapper(wrap->_base._self, wrap->_base._name);
  if (keys != nullptr) {
    keys->_len_func = wrap->_keys._len_func;
    keys->_getitem_func = wrap->_keys._getitem_func;
    return PySeqIter_New((PyObject *)keys);
  } else {
    return nullptr;
  }
}

/**
 * Implementation of property.get(key[,def=None]) which returns the value with
 * the given key in the mapping, or the given default value (which defaults to
 * None) if the key isn't found in the mapping.
 */
static PyObject *Dtool_MappingWrapper_get(PyObject *self, PyObject *args) {
  Dtool_MappingWrapper *wrap = (Dtool_MappingWrapper *)self;
  nassertr(wrap, nullptr);
  nassertr(wrap->_getitem_func, nullptr);
  Py_ssize_t size = PyTuple_GET_SIZE(args);
  if (size != 1 && size != 2) {
    return PyErr_Format(PyExc_TypeError, "%s.get() takes 1 or 2 arguments", wrap->_base._name);
  }
  PyObject *defvalue = Py_None;
  if (size >= 2) {
    defvalue = PyTuple_GET_ITEM(args, 1);
  }
  PyObject *key = PyTuple_GET_ITEM(args, 0);
  PyObject *value = wrap->_getitem_func(wrap->_base._self, key);
  if (value != nullptr) {
    return value;
  } else if (PyErr_ExceptionMatches(PyExc_KeyError)) {
    PyErr_Clear();
    return Py_NewRef(defvalue);
  } else {
    return nullptr;
  }
}

/**
 * This is returned by mapping.keys().
 */
static PyObject *Dtool_MappingWrapper_Keys_repr(PyObject *self) {
  Dtool_WrapperBase *wrap = (Dtool_WrapperBase *)self;
  nassertr(wrap, nullptr);

  PyObject *repr = PyObject_Repr(wrap->_self);
  PyObject *result;
#if PY_MAJOR_VERSION >= 3
  result = PyUnicode_FromFormat("<%s.keys() of %s>", wrap->_name, PyUnicode_AsUTF8(repr));
#else
  result = PyString_FromFormat("<%s.keys() of %s>", wrap->_name, PyString_AS_STRING(repr));
#endif
  Py_DECREF(repr);
  return result;
}

/**
 * Implementation of property.keys(...) that returns a view of all the keys.
 */
static PyObject *Dtool_MappingWrapper_keys(PyObject *self, PyObject *) {
  Dtool_MappingWrapper *wrap = (Dtool_MappingWrapper *)self;
  nassertr(wrap, nullptr);

  if (wrap->_keys._len_func == nullptr || wrap->_keys._getitem_func == nullptr) {
    return Dtool_Raise_TypeError("property does not support keys()");
  }

  static PyType_Slot wrapper_slots[] = {
    {Py_tp_dealloc, (void *)&Dtool_WrapperBase_dealloc},
    {Py_tp_repr, (void *)&Dtool_MappingWrapper_Keys_repr},
    {Py_tp_iter, (void *)&PySeqIter_New},
    {Py_sq_length, (void *)&Dtool_SequenceWrapper_length},
    {Py_sq_item, (void *)&Dtool_SequenceWrapper_getitem},
    {Py_sq_contains, (void *)&Dtool_SequenceWrapper_contains},
    {0, nullptr},
  };

  static PyType_Spec wrapper_spec = {
    "sys._interrogate_mapping_wrapper_keys",
    sizeof(Dtool_MappingWrapper),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    wrapper_slots,
  };

  PyObject *wrapper_type = _init_wrapper_type(wrapper_spec, "MappingView");
  nassertr_always(wrapper_type != nullptr, nullptr);

  Dtool_MappingWrapper *keys = PyObject_New(Dtool_MappingWrapper, (PyTypeObject *)wrapper_type);
  if (keys != nullptr) {
    keys->_base._self = Py_XNewRef(wrap->_base._self);
    keys->_base._name = wrap->_base._name;
    keys->_keys._len_func = wrap->_keys._len_func;
    keys->_keys._getitem_func = wrap->_keys._getitem_func;
    keys->_getitem_func = wrap->_getitem_func;
    keys->_setitem_func = nullptr;
  }
  return (PyObject *)keys;
}

/**
 * This is returned by mapping.values().
 */
static PyObject *Dtool_MappingWrapper_Values_repr(PyObject *self) {
  Dtool_WrapperBase *wrap = (Dtool_WrapperBase *)self;
  nassertr(wrap, nullptr);

  PyObject *repr = PyObject_Repr(wrap->_self);
  PyObject *result;
#if PY_MAJOR_VERSION >= 3
  result = PyUnicode_FromFormat("<%s.values() of %s>", wrap->_name, PyUnicode_AsUTF8(repr));
#else
  result = PyString_FromFormat("<%s.values() of %s>", wrap->_name, PyString_AS_STRING(repr));
#endif
  Py_DECREF(repr);
  return result;
}

static PyObject *Dtool_MappingWrapper_Values_getitem(PyObject *self, Py_ssize_t index) {
  Dtool_MappingWrapper *wrap = (Dtool_MappingWrapper *)self;
  nassertr(wrap, nullptr);
  nassertr(wrap->_keys._getitem_func, nullptr);

  PyObject *key = wrap->_keys._getitem_func(wrap->_base._self, index);
  if (key != nullptr) {
    PyObject *value = wrap->_getitem_func(wrap->_base._self, key);
    Py_DECREF(key);
    return value;
  }
  return nullptr;
}

/**
 * Implementation of property.values(...) that returns a view of the values.
 */
static PyObject *Dtool_MappingWrapper_values(PyObject *self, PyObject *) {
  Dtool_MappingWrapper *wrap = (Dtool_MappingWrapper *)self;
  nassertr(wrap, nullptr);
  nassertr(wrap->_getitem_func, nullptr);

  if (wrap->_keys._len_func == nullptr || wrap->_keys._getitem_func == nullptr) {
    return Dtool_Raise_TypeError("property does not support values()");
  }

  static PyType_Slot wrapper_slots[] = {
    {Py_tp_dealloc, (void *)&Dtool_WrapperBase_dealloc},
    {Py_tp_repr, (void *)&Dtool_MappingWrapper_Values_repr},
    {Py_tp_iter, (void *)&PySeqIter_New},
    {Py_sq_length, (void *)&Dtool_SequenceWrapper_length},
    {Py_sq_item, (void *)&Dtool_MappingWrapper_Values_getitem},
    {0, nullptr},
  };

  static PyType_Spec wrapper_spec = {
    "sys._interrogate_mapping_wrapper_values",
    sizeof(Dtool_MappingWrapper),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    wrapper_slots,
  };

  PyObject *wrapper_type = _init_wrapper_type(wrapper_spec, "ValuesView");
  nassertr_always(wrapper_type != nullptr, nullptr);

  Dtool_MappingWrapper *values = PyObject_New(Dtool_MappingWrapper, (PyTypeObject *)wrapper_type);
  if (values != nullptr) {
    values->_base._self = Py_XNewRef(wrap->_base._self);
    values->_base._name = wrap->_base._name;
    values->_keys._len_func = wrap->_keys._len_func;
    values->_keys._getitem_func = wrap->_keys._getitem_func;
    values->_getitem_func = wrap->_getitem_func;
    values->_setitem_func = nullptr;
  }
  return (PyObject *)values;
}

/**
 * This is returned by mapping.items().
 */
static PyObject *Dtool_MappingWrapper_Items_repr(PyObject *self) {
  Dtool_WrapperBase *wrap = (Dtool_WrapperBase *)self;
  nassertr(wrap, nullptr);

  PyObject *repr = PyObject_Repr(wrap->_self);
  PyObject *result;
#if PY_MAJOR_VERSION >= 3
  result = PyUnicode_FromFormat("<%s.items() of %s>", wrap->_name, PyUnicode_AsUTF8(repr));
#else
  result = PyString_FromFormat("<%s.items() of %s>", wrap->_name, PyString_AS_STRING(repr));
#endif
  Py_DECREF(repr);
  return result;
}

static PyObject *Dtool_MappingWrapper_Items_getitem(PyObject *self, Py_ssize_t index) {
  Dtool_MappingWrapper *wrap = (Dtool_MappingWrapper *)self;
  nassertr(wrap, nullptr);
  nassertr(wrap->_keys._getitem_func, nullptr);

  PyObject *key = wrap->_keys._getitem_func(wrap->_base._self, index);
  if (key != nullptr) {
    PyObject *value = wrap->_getitem_func(wrap->_base._self, key);
    if (value != nullptr) {
      // PyTuple_SET_ITEM steals the reference.
      PyObject *item = PyTuple_New(2);
      PyTuple_SET_ITEM(item, 0, key);
      PyTuple_SET_ITEM(item, 1, value);
      return item;
    } else {
      Py_DECREF(key);
    }
  }
  return nullptr;
}

/**
 * Implementation of property.items(...) that returns an iterable yielding a
 * `(key, value)` tuple for every item.
 */
static PyObject *Dtool_MappingWrapper_items(PyObject *self, PyObject *) {
  Dtool_MappingWrapper *wrap = (Dtool_MappingWrapper *)self;
  nassertr(wrap, nullptr);
  nassertr(wrap->_getitem_func, nullptr);

  if (wrap->_keys._len_func == nullptr || wrap->_keys._getitem_func == nullptr) {
    return Dtool_Raise_TypeError("property does not support items()");
  }

  static PyType_Slot wrapper_slots[] = {
    {Py_tp_dealloc, (void *)&Dtool_WrapperBase_dealloc},
    {Py_tp_repr, (void *)&Dtool_MappingWrapper_Items_repr},
    {Py_tp_iter, (void *)&PySeqIter_New},
    {Py_sq_length, (void *)&Dtool_SequenceWrapper_length},
    {Py_sq_item, (void *)&Dtool_MappingWrapper_Items_getitem},
    {Py_sq_contains, (void *)&Dtool_MappingWrapper_contains},
    {0, nullptr},
  };

  static PyType_Spec wrapper_spec = {
    "sys._interrogate_mapping_wrapper_items",
    sizeof(Dtool_MappingWrapper),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    wrapper_slots,
  };

  PyObject *wrapper_type = _init_wrapper_type(wrapper_spec, "MappingView");
  nassertr_always(wrapper_type != nullptr, nullptr);

  Dtool_MappingWrapper *items = PyObject_New(Dtool_MappingWrapper, (PyTypeObject *)wrapper_type);
  if (items != nullptr) {
    items->_base._self = Py_XNewRef(wrap->_base._self);
    items->_base._name = wrap->_base._name;
    items->_keys._len_func = wrap->_keys._len_func;
    items->_keys._getitem_func = wrap->_keys._getitem_func;
    items->_getitem_func = wrap->_getitem_func;
    items->_setitem_func = nullptr;
  }
  return (PyObject *)items;
}

/**
 * Implementation of `property[key] = value`
 */
static int Dtool_MutableMappingWrapper_setitem(PyObject *self, PyObject *key, PyObject *value) {
  Dtool_MappingWrapper *wrap = (Dtool_MappingWrapper *)self;
  nassertr(wrap->_setitem_func != nullptr, -1);
  return wrap->_setitem_func(wrap->_base._self, key, value);
}

/**
 * Implementation of property.pop(key[,def=None]) which is the same as get()
 * except that it also removes the element from the mapping.
 */
static PyObject *Dtool_MutableMappingWrapper_pop(PyObject *self, PyObject *args) {
  Dtool_MappingWrapper *wrap = (Dtool_MappingWrapper *)self;
  nassertr(wrap, nullptr);
  if (wrap->_getitem_func == nullptr || wrap->_setitem_func == nullptr) {
    return Dtool_Raise_TypeError("property does not support pop()");
  }

  Py_ssize_t size = PyTuple_GET_SIZE(args);
  if (size != 1 && size != 2) {
    return PyErr_Format(PyExc_TypeError, "%s.pop() takes 1 or 2 arguments", wrap->_base._name);
  }
  PyObject *defvalue = Py_None;
  if (size >= 2) {
    defvalue = PyTuple_GET_ITEM(args, 1);
  }

  PyObject *key = PyTuple_GET_ITEM(args, 0);
  PyObject *value = wrap->_getitem_func(wrap->_base._self, key);
  if (value != nullptr) {
    // OK, now set unset this value.
    if (wrap->_setitem_func(wrap->_base._self, key, nullptr) == 0) {
      return value;
    } else {
      Py_DECREF(value);
      return nullptr;
    }
  } else if (PyErr_ExceptionMatches(PyExc_KeyError)) {
    PyErr_Clear();
    return Py_NewRef(defvalue);
  } else {
    return nullptr;
  }
}

/**
 * Implementation of property.popitem() which returns and removes an arbitrary
 * (key, value) pair from the mapping.  Useful for destructive iteration.
 */
static PyObject *Dtool_MutableMappingWrapper_popitem(PyObject *self, PyObject *) {
  Dtool_MappingWrapper *wrap = (Dtool_MappingWrapper *)self;
  nassertr(wrap, nullptr);
  if (wrap->_getitem_func == nullptr || wrap->_setitem_func == nullptr ||
      wrap->_keys._len_func == nullptr || wrap->_keys._getitem_func == nullptr) {
    return Dtool_Raise_TypeError("property does not support popitem()");
  }

  Py_ssize_t length = wrap->_keys._len_func(wrap->_base._self);
  if (length < 1) {
    return PyErr_Format(PyExc_KeyError, "%s is empty", wrap->_base._name);
  }

  PyObject *key = wrap->_keys._getitem_func(wrap->_base._self, length - 1);
  if (key != nullptr) {
    PyObject *value = wrap->_getitem_func(wrap->_base._self, key);
    if (value != nullptr) {
      // OK, now set unset this value.
      if (wrap->_setitem_func(wrap->_base._self, key, nullptr) == 0) {
        PyObject *item = PyTuple_New(2);
        PyTuple_SET_ITEM(item, 0, key);
        PyTuple_SET_ITEM(item, 1, value);
        return item;
      }
      Py_DECREF(value);
    }
  }
  return nullptr;
}

/*
 * Implementation of property.clear() which removes all elements in the
 * mapping.
 */
static PyObject *Dtool_MutableMappingWrapper_clear(PyObject *self, PyObject *) {
  Dtool_MappingWrapper *wrap = (Dtool_MappingWrapper *)self;
  nassertr(wrap, nullptr);
  Py_ssize_t index = 0;
  if (wrap->_keys._len_func != nullptr && wrap->_keys._getitem_func != nullptr &&
      wrap->_setitem_func != nullptr) {
    index = wrap->_keys._len_func(wrap->_base._self);
  } else {
    return Dtool_Raise_TypeError("property does not support clear()");
  }

  // Iterate through the items, invoking the delete function for each.  We do
  // this in reverse order, which may be more efficient.
  while (index > 0) {
    --index;
    PyObject *key = wrap->_keys._getitem_func(wrap->_base._self, index);
    if (key != nullptr) {
      int result = wrap->_setitem_func(wrap->_base._self, key, nullptr);
      Py_DECREF(key);
      if (result != 0) {
        return nullptr;
      }
    }
  }
  return Py_NewRef(Py_None);
}

/**
 * Implementation of property.setdefault(key[,def=None]) which is the same as
 * get() except that it also writes the default value back to the mapping if
 * the key was not found is missing.
 */
static PyObject *Dtool_MutableMappingWrapper_setdefault(PyObject *self, PyObject *args) {
  Dtool_MappingWrapper *wrap = (Dtool_MappingWrapper *)self;
  nassertr(wrap, nullptr);

  if (wrap->_getitem_func == nullptr || wrap->_setitem_func == nullptr) {
    return Dtool_Raise_TypeError("property does not support setdefault()");
  }

  Py_ssize_t size = PyTuple_GET_SIZE(args);
  if (size != 1 && size != 2) {
    return PyErr_Format(PyExc_TypeError, "%s.setdefault() takes 1 or 2 arguments", wrap->_base._name);
  }
  PyObject *defvalue = Py_None;
  if (size >= 2) {
    defvalue = PyTuple_GET_ITEM(args, 1);
  }
  PyObject *key = PyTuple_GET_ITEM(args, 0);
  PyObject *value = wrap->_getitem_func(wrap->_base._self, key);
  if (value != nullptr) {
    return value;
  } else if (PyErr_ExceptionMatches(PyExc_KeyError)) {
    PyErr_Clear();
    if (wrap->_setitem_func(wrap->_base._self, key, defvalue) == 0) {
      return Py_NewRef(defvalue);
    }
  }
  return nullptr;
}

/**
 * Implementation of property.update(...) which sets multiple values in one
 * go.  It accepts either a single dictionary or keyword arguments, not both.
 */
static PyObject *Dtool_MutableMappingWrapper_update(PyObject *self, PyObject *args, PyObject *kwargs) {
  Dtool_MappingWrapper *wrap = (Dtool_MappingWrapper *)self;
  nassertr(wrap, nullptr);

  if (wrap->_getitem_func == nullptr || wrap->_setitem_func == nullptr) {
    return Dtool_Raise_TypeError("property does not support update()");
  }

  // We accept either a dict argument or keyword arguments, but not both.
  PyObject *dict;
  switch (PyTuple_GET_SIZE(args)) {
  case 0:
    if (kwargs == nullptr) {
      // This is legal.
      return Py_NewRef(Py_None);
    }
    dict = kwargs;
    break;
  case 1:
    if (PyDict_Check(PyTuple_GET_ITEM(args, 0)) && (kwargs == nullptr || Py_SIZE(kwargs) == 0)) {
      dict = PyTuple_GET_ITEM(args, 0);
      break;
    }
    // Fall through
  default:
    return PyErr_Format(PyExc_TypeError, "%s.update() takes either a dict argument or keyword arguments", wrap->_base._name);
  }

  PyObject *result = Py_None;
  PyObject *key, *value;
  Py_ssize_t pos = 0;
  Py_BEGIN_CRITICAL_SECTION(dict);
  while (PyDict_Next(dict, &pos, &key, &value)) {
    if (wrap->_setitem_func(wrap->_base._self, key, value) != 0) {
      result = nullptr;
      break;
    }
  }
  Py_END_CRITICAL_SECTION();

  return Py_XNewRef(result);
}

/**
 * This variant defines only a generator interface.
 */
static PyObject *Dtool_GeneratorWrapper_iternext(PyObject *self) {
  Dtool_GeneratorWrapper *wrap = (Dtool_GeneratorWrapper *)self;
  nassertr(wrap, nullptr);
  nassertr(wrap->_iternext_func, nullptr);
  return wrap->_iternext_func(wrap->_base._self);
}

/**
 * This is a variant of the Python getset mechanism that permits static
 * properties.
 */
static void
Dtool_StaticProperty_dealloc(PyDescrObject *descr) {
#if PY_VERSION_HEX >= 0x03080000
  PyObject_GC_UnTrack(descr);
#else
  _PyObject_GC_UNTRACK(descr);
#endif
  Py_XDECREF(descr->d_type);
  Py_XDECREF(descr->d_name);
//#if PY_MAJOR_VERSION >= 3
//  Py_XDECREF(descr->d_qualname);
//#endif
  PyObject_GC_Del(descr);
}

static PyObject *
Dtool_StaticProperty_repr(PyDescrObject *descr, const char *format) {
#if PY_MAJOR_VERSION >= 3
  return PyUnicode_FromFormat("<attribute '%s' of '%s'>",
                              PyUnicode_AsUTF8(descr->d_name),
                              descr->d_type->tp_name);
#else
  return PyString_FromFormat("<attribute '%s' of '%s'>",
                             PyString_AS_STRING(descr->d_name),
                             descr->d_type->tp_name);
#endif
}

static int
Dtool_StaticProperty_traverse(PyObject *self, visitproc visit, void *arg) {
  PyDescrObject *descr = (PyDescrObject *)self;
  Py_VISIT(descr->d_type);
  return 0;
}

static PyObject *
Dtool_StaticProperty_get(PyGetSetDescrObject *descr, PyObject *obj, PyObject *type) {
  if (descr->d_getset->get != nullptr) {
    return descr->d_getset->get(obj, descr->d_getset->closure);
  } else {
    return PyErr_Format(PyExc_AttributeError,
                        "attribute '%s' of type '%.100s' is not readable",
#if PY_MAJOR_VERSION >= 3
                        PyUnicode_AsUTF8(((PyDescrObject *)descr)->d_name),
#else
                        PyString_AS_STRING(((PyDescrObject *)descr)->d_name),
#endif
                        ((PyDescrObject *)descr)->d_type->tp_name);
  }
}

static int
Dtool_StaticProperty_set(PyGetSetDescrObject *descr, PyObject *obj, PyObject *value) {
  if (descr->d_getset->set != nullptr) {
    return descr->d_getset->set(obj, value, descr->d_getset->closure);
  } else {
    PyErr_Format(PyExc_AttributeError,
                 "attribute '%s' of type '%.100s' is not writable",
#if PY_MAJOR_VERSION >= 3
                 PyUnicode_AsUTF8(((PyDescrObject *)descr)->d_name),
#else
                 PyString_AS_STRING(((PyDescrObject *)descr)->d_name),
#endif
                 ((PyDescrObject *)descr)->d_type->tp_name);
    return -1;
  }
}

/**
 * This wraps around a property that exposes a sequence interface.
 */
Dtool_SequenceWrapper *Dtool_NewSequenceWrapper(PyObject *self, const char *name) {
  static PyMethodDef methods[] = {
    {"index", &Dtool_SequenceWrapper_index, METH_O, nullptr},
    {"count", &Dtool_SequenceWrapper_count, METH_O, nullptr},
    {nullptr, nullptr, 0, nullptr}
  };

  static PyType_Slot wrapper_slots[] = {
    {Py_tp_dealloc, (void *)&Dtool_WrapperBase_dealloc},
    {Py_tp_repr, (void *)&Dtool_SequenceWrapper_repr},
    {Py_tp_iter, (void *)&PySeqIter_New},
    {Py_sq_length, (void *)&Dtool_SequenceWrapper_length},
    {Py_sq_item, (void *)&Dtool_SequenceWrapper_getitem},
    {Py_sq_contains, (void *)&Dtool_SequenceWrapper_contains},
    {Py_tp_methods, (void *)methods},
    {0, nullptr},
  };

  static PyType_Spec wrapper_spec = {
    "sys._interrogate_sequence_wrapper",
    sizeof(Dtool_SequenceWrapper),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    wrapper_slots,
  };

  PyObject *wrapper_type = _init_wrapper_type(wrapper_spec, "Sequence");
  nassertr_always(wrapper_type != nullptr, nullptr);

  Dtool_SequenceWrapper *wrap = PyObject_New(Dtool_SequenceWrapper, (PyTypeObject *)wrapper_type);
  if (wrap != nullptr) {
    wrap->_base._self = Py_XNewRef(self);
    wrap->_base._name = name;
    wrap->_len_func = nullptr;
    wrap->_getitem_func = nullptr;
  }
  return wrap;
}

/**
 * This wraps around a property that exposes a mutable sequence interface.
 */
Dtool_MutableSequenceWrapper *Dtool_NewMutableSequenceWrapper(PyObject *self, const char *name) {
  static PyMethodDef methods[] = {
    {"index", &Dtool_SequenceWrapper_index, METH_O, nullptr},
    {"count", &Dtool_SequenceWrapper_count, METH_O, nullptr},
    {"clear", &Dtool_MutableSequenceWrapper_clear, METH_NOARGS, nullptr},
    {"pop", &Dtool_MutableSequenceWrapper_pop, METH_VARARGS, nullptr},
    {"remove", &Dtool_MutableSequenceWrapper_remove, METH_O, nullptr},
    {"append", &Dtool_MutableSequenceWrapper_append, METH_O, nullptr},
    {"insert", &Dtool_MutableSequenceWrapper_insert, METH_VARARGS, nullptr},
    {"extend", &Dtool_MutableSequenceWrapper_extend, METH_O, nullptr},
    {nullptr, nullptr, 0, nullptr}
  };

  static PyType_Slot wrapper_slots[] = {
    {Py_tp_dealloc, (void *)&Dtool_WrapperBase_dealloc},
    {Py_tp_repr, (void *)&Dtool_SequenceWrapper_repr},
    {Py_tp_iter, (void *)&PySeqIter_New},
    {Py_sq_length, (void *)&Dtool_SequenceWrapper_length},
    {Py_sq_item, (void *)&Dtool_SequenceWrapper_getitem},
    {Py_sq_ass_item, (void *)&Dtool_MutableSequenceWrapper_setitem},
    {Py_sq_contains, (void *)&Dtool_SequenceWrapper_contains},
    {Py_sq_inplace_concat, (void *)&Dtool_MutableSequenceWrapper_extend},
    {Py_tp_methods, (void *)methods},
    {0, nullptr},
  };

  static PyType_Spec wrapper_spec = {
    "sys._interrogate_mutable_sequence_wrappes",
    sizeof(Dtool_MutableSequenceWrapper),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    wrapper_slots,
  };

  PyObject *wrapper_type = _init_wrapper_type(wrapper_spec, "MutableSequence");
  nassertr_always(wrapper_type != nullptr, nullptr);

  Dtool_MutableSequenceWrapper *wrap = PyObject_New(Dtool_MutableSequenceWrapper, (PyTypeObject *)wrapper_type);
  if (wrap != nullptr) {
    wrap->_base._self = Py_XNewRef(self);
    wrap->_base._name = name;
    wrap->_len_func = nullptr;
    wrap->_getitem_func = nullptr;
    wrap->_setitem_func = nullptr;
    wrap->_insert_func = nullptr;
  }
  return wrap;
}

/**
 * This wraps around a mapping interface, with getitem function.
 */
Dtool_MappingWrapper *Dtool_NewMappingWrapper(PyObject *self, const char *name) {
  static PyMethodDef methods[] = {
    {"get", &Dtool_MappingWrapper_get, METH_VARARGS, nullptr},
    {"keys", &Dtool_MappingWrapper_keys, METH_NOARGS, nullptr},
    {"values", &Dtool_MappingWrapper_values, METH_NOARGS, nullptr},
    {"items", &Dtool_MappingWrapper_items, METH_NOARGS, nullptr},
    {nullptr, nullptr, 0, nullptr}
  };

  static PyType_Slot wrapper_slots[] = {
    {Py_tp_dealloc, (void *)&Dtool_WrapperBase_dealloc},
    {Py_tp_repr, (void *)&Dtool_WrapperBase_repr},
    {Py_tp_iter, (void *)&Dtool_MappingWrapper_iter},
    {Py_sq_length, (void *)&Dtool_SequenceWrapper_length},
    {Py_sq_contains, (void *)&Dtool_MappingWrapper_contains},
    {Py_mp_length, (void *)&Dtool_SequenceWrapper_length},
    {Py_mp_subscript, (void *)&Dtool_MappingWrapper_getitem},
    {Py_tp_methods, (void *)methods},
    {0, nullptr},
  };

  static PyType_Spec wrapper_spec = {
    "sys._interrogate_mapping_wrapper",
    sizeof(Dtool_MappingWrapper),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    wrapper_slots,
  };

  PyObject *wrapper_type = _init_wrapper_type(wrapper_spec, "Mapping");
  nassertr_always(wrapper_type != nullptr, nullptr);

  Dtool_MappingWrapper *wrap = PyObject_New(Dtool_MappingWrapper, (PyTypeObject *)wrapper_type);
  if (wrap != nullptr) {
    wrap->_base._self = Py_XNewRef(self);
    wrap->_base._name = name;
    wrap->_keys._len_func = nullptr;
    wrap->_keys._getitem_func = nullptr;
    wrap->_getitem_func = nullptr;
    wrap->_setitem_func = nullptr;
  }
  return wrap;
}

/**
 * This wraps around a mapping interface, with getitem/setitem functions.
 */
Dtool_MappingWrapper *Dtool_NewMutableMappingWrapper(PyObject *self, const char *name) {
  static PyMethodDef methods[] = {
    {"get", &Dtool_MappingWrapper_get, METH_VARARGS, nullptr},
    {"pop", &Dtool_MutableMappingWrapper_pop, METH_VARARGS, nullptr},
    {"popitem", &Dtool_MutableMappingWrapper_popitem, METH_NOARGS, nullptr},
    {"clear", &Dtool_MutableMappingWrapper_clear, METH_VARARGS, nullptr},
    {"setdefault", &Dtool_MutableMappingWrapper_setdefault, METH_VARARGS, nullptr},
    {"update", (PyCFunction) &Dtool_MutableMappingWrapper_update, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"keys", &Dtool_MappingWrapper_keys, METH_NOARGS, nullptr},
    {"values", &Dtool_MappingWrapper_values, METH_NOARGS, nullptr},
    {"items", &Dtool_MappingWrapper_items, METH_NOARGS, nullptr},
    {nullptr, nullptr, 0, nullptr}
  };

  static PyType_Slot wrapper_slots[] = {
    {Py_tp_dealloc, (void *)&Dtool_WrapperBase_dealloc},
    {Py_tp_repr, (void *)&Dtool_WrapperBase_repr},
    {Py_tp_iter, (void *)&Dtool_MappingWrapper_iter},
    {Py_sq_length, (void *)&Dtool_SequenceWrapper_length},
    {Py_sq_contains, (void *)&Dtool_MappingWrapper_contains},
    {Py_mp_length, (void *)&Dtool_SequenceWrapper_length},
    {Py_mp_subscript, (void *)&Dtool_MappingWrapper_getitem},
    {Py_mp_ass_subscript, (void *)&Dtool_MutableMappingWrapper_setitem},
    {Py_tp_methods, (void *)methods},
    {0, nullptr},
  };

  static PyType_Spec wrapper_spec = {
    "sys._interrogate_mutable_mapping_wrapper",
    sizeof(Dtool_MappingWrapper),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    wrapper_slots,
  };

  PyObject *wrapper_type = _init_wrapper_type(wrapper_spec, "MutableMapping");
  nassertr_always(wrapper_type != nullptr, nullptr);

  Dtool_MappingWrapper *wrap = PyObject_New(Dtool_MappingWrapper, (PyTypeObject *)wrapper_type);
  if (wrap != nullptr) {
    wrap->_base._self = Py_XNewRef(self);
    wrap->_base._name = name;
    wrap->_keys._len_func = nullptr;
    wrap->_keys._getitem_func = nullptr;
    wrap->_getitem_func = nullptr;
    wrap->_setitem_func = nullptr;
  }
  return wrap;
}

/**
 * Creates a generator that invokes a given function with the given self arg.
 */
PyObject *
Dtool_NewGenerator(PyObject *self, iternextfunc gen_next) {
  static PyType_Slot wrapper_slots[] = {
    {Py_tp_dealloc, (void *)&Dtool_WrapperBase_dealloc},
    {Py_tp_repr, (void *)&Dtool_SequenceWrapper_repr},
    {Py_tp_iter, (void *)&PyObject_SelfIter},
    {Py_tp_iternext, (void *)&Dtool_GeneratorWrapper_iternext},
    {0, nullptr},
  };

  static PyType_Spec wrapper_spec = {
    "sys._interrogate_generator_wrapper",
    sizeof(Dtool_GeneratorWrapper),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    wrapper_slots,
  };

  PyObject *wrapper_type = _init_wrapper_type(wrapper_spec);
  nassertr_always(wrapper_type != nullptr, nullptr);

  Dtool_GeneratorWrapper *gen = PyObject_New(Dtool_GeneratorWrapper, (PyTypeObject *)wrapper_type);
  if (gen != nullptr) {
    gen->_base._self = Py_NewRef(self);
    gen->_iternext_func = gen_next;
  }
  return (PyObject *)gen;
}

/**
 * This is a variant of the Python getset mechanism that permits static
 * properties.
 */
PyObject *
Dtool_NewStaticProperty(PyTypeObject *type, const PyGetSetDef *getset) {
  static PyType_Slot wrapper_slots[] = {
    {Py_tp_dealloc, (void *)&Dtool_StaticProperty_dealloc},
    {Py_tp_repr, (void *)&Dtool_StaticProperty_repr},
    {Py_tp_traverse, (void *)&Dtool_StaticProperty_traverse},
    {Py_tp_descr_get, (void *)&Dtool_StaticProperty_get},
    {Py_tp_descr_set, (void *)&Dtool_StaticProperty_set},
    {0, nullptr},
  };

  static PyType_Spec wrapper_spec = {
    "sys._interrogate_static_property",
    sizeof(PyGetSetDescrObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    wrapper_slots,
  };

  PyObject *wrapper_type = _init_wrapper_type(wrapper_spec);
  nassertr_always(wrapper_type != nullptr, nullptr);

  PyGetSetDescrObject *descr = PyObject_New(PyGetSetDescrObject, (PyTypeObject *)wrapper_type);
  if (descr != nullptr) {
    Py_XINCREF(type);
    descr->d_getset = (PyGetSetDef *)getset;
#if PY_MAJOR_VERSION >= 3
    descr->d_common.d_type = type;
    descr->d_common.d_name = PyUnicode_InternFromString(getset->name);
#if PY_VERSION_HEX >= 0x03030000
    descr->d_common.d_qualname = nullptr;
#endif
#else
    descr->d_type = type;
    descr->d_name = PyString_InternFromString(getset->name);
#endif
  }
  return (PyObject *)descr;
}

#endif  // HAVE_PYTHON
