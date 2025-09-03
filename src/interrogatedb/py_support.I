/**
 * @file py_support.I
 * @author rdb
 * @date 2025-09-03
 */

/**
 * Function to create a hash from a wrapped Python object.
 */
INLINE Py_hash_t DtoolInstance_HashPointer(PyObject *self) {
  if (self != nullptr && DtoolInstance_Check(self)) {
    return (Py_hash_t)(intptr_t)DtoolInstance_VOID_PTR(self);
  }
  return -1;
}

/**
 * Python 2-style comparison function that compares objects by pointer.
 */
INLINE int DtoolInstance_ComparePointers(PyObject *v1, PyObject *v2) {
  void *v1_this = DtoolInstance_Check(v1) ? DtoolInstance_VOID_PTR(v1) : nullptr;
  void *v2_this = DtoolInstance_Check(v2) ? DtoolInstance_VOID_PTR(v2) : nullptr;
  if (v1_this != nullptr && v2_this != nullptr) {
    return (v1_this > v2_this) - (v1_this < v2_this);
  } else {
    return (v1 > v2) - (v1 < v2);
  }
}

/**
 * Rich comparison function that compares objects by pointer.
 */
INLINE PyObject *DtoolInstance_RichComparePointers(PyObject *v1, PyObject *v2, int op) {
  int cmpval = DtoolInstance_ComparePointers(v1, v2);
  Py_RETURN_RICHCOMPARE(cmpval, 0, op);
}

/**
 * Utility function for assigning a PyObject pointer while managing refcounts.
 */
ALWAYS_INLINE void
Dtool_Assign_PyObject(PyObject *&ptr, PyObject *value) {
  PyObject *prev_value = ptr;
  if (prev_value != value) {
    ptr = Py_XNewRef(value);
    Py_XDECREF(prev_value);
  }
}

/**
 * Converts the enum value to a C long.
 */
INLINE long Dtool_EnumValue_AsLong(PyObject *value) {
  PyObject *val = PyObject_GetAttrString(value, "value");
  if (val != nullptr) {
    long as_long = PyLongOrInt_AS_LONG(val);
    Py_DECREF(val);
    return as_long;
  } else {
    return -1;
  }
}

/**
 * Checks that the tuple is empty.
 */
ALWAYS_INLINE bool
Dtool_CheckNoArgs(PyObject *args) {
  return PyTuple_GET_SIZE(args) == 0;
}

/**
 * Checks that the tuple is empty, and that the dict is empty or NULL.
 */
ALWAYS_INLINE bool
Dtool_CheckNoArgs(PyObject *args, PyObject *kwds) {
  return PyTuple_GET_SIZE(args) == 0 &&
    (kwds == nullptr || PyDict_GET_SIZE(kwds) == 0);
}
