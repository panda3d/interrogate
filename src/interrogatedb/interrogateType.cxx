/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file interrogateType.cxx
 * @author drose
 * @date 2000-07-31
 */

#include "interrogateType.h"
#include "indexRemapper.h"
#include "interrogate_datafile.h"
#include "interrogateDatabase.h"
#include "indent.h"

#include <algorithm>

using std::istream;
using std::ostream;

/**
 *
 */
InterrogateType::
InterrogateType(InterrogateModuleDef *def) :
  InterrogateComponent(def)
{
  _flags = 0;
  _outer_class = 0;
  _atomic_token = AT_not_atomic;
  _wrapped_type = 0;
  _array_size = 1;
  _destructor = 0;

  _cpptype = nullptr;
  _cppscope = nullptr;
}

/**
 *
 */
InterrogateType::
InterrogateType(const InterrogateType &copy) {
  (*this) = copy;
}

/**
 *
 */
void InterrogateType::Derivation::
output(ostream &out) const {
  out << _flags << " " << _base << " " << _upcast << " " << _downcast;
}

/**
 *
 */
void InterrogateType::Derivation::
input(istream &in) {
  in >> _flags >> _base >> _upcast >> _downcast;
}

/**
 *
 */
void InterrogateType::EnumValue::
output(ostream &out) const {
  idf_output_string(out, _name);
  idf_output_string(out, _scoped_name);
  idf_output_string(out, _comment, '\n');
  out << _value;
}

/**
 *
 */
void InterrogateType::EnumValue::
input(istream &in) {
  idf_input_string(in, _name);
  idf_input_string(in, _scoped_name);
  idf_input_string(in, _comment);
  in >> _value;
}

/**
 *
 */
void InterrogateType::
operator = (const InterrogateType &copy) {
  InterrogateComponent::operator = (copy);
  _flags = copy._flags;
  _scoped_name = copy._scoped_name;
  _true_name = copy._true_name;
  _comment = copy._comment;
  _outer_class = copy._outer_class;
  _atomic_token = copy._atomic_token;
  _wrapped_type = copy._wrapped_type;
  _array_size = copy._array_size;
  _constructors = copy._constructors;
  _destructor = copy._destructor;
  _elements = copy._elements;
  _methods = copy._methods;
  _make_seqs = copy._make_seqs;
  _casts = copy._casts;
  _derivations = copy._derivations;
  _enum_values = copy._enum_values;
  _nested_types = copy._nested_types;

  _cpptype = copy._cpptype;
  _cppscope = copy._cppscope;
}

/**
 * Combines type with the other similar definition.  If one type is "fully
 * defined" and the other one isn't, the fully-defined type wins.  If both
 * types are fully defined, whichever type is marked "global" wins.
 */
void InterrogateType::
merge_with(const InterrogateType &other) {
  // The only thing we care about copying from the non-fully-defined type
  // right now is the global flag.

  if (is_fully_defined() &&
      (!other.is_fully_defined() || (other._flags & F_global) == 0)) {
    // We win.
    _flags |= (other._flags & F_global);

  } else {
    // They win.
    int old_flags = (_flags & F_global);
    (*this) = other;
    _flags |= old_flags;
  }
}

/**
 * Formats the type in a human-readable manner.
 */
void InterrogateType::
write(std::ostream &out, int indent_level) const {
  indent(out, indent_level) << "type ";
  //write_names(out);
  out << "\"" << get_scoped_name() << "\"";
  out << " {\n";

  //indent(out, indent_level) << "  true_name: " << _true_name << "\n";
  if (_flags != 0) {
    indent(out, indent_level) << "  flags:";
    if (_flags & F_global) {
      out << " global";
    }
    if (_flags & F_atomic) {
      out << " atomic";
    }
    if (_flags & F_unsigned) {
      out << " unsigned";
    }
    if (_flags & F_signed) {
      out << " signed";
    }
    if (_flags & F_long) {
      out << " long";
    }
    if (_flags & F_longlong) {
      out << " longlong";
    }
    if (_flags & F_short) {
      out << " short";
    }
    if (_flags & F_wrapped) {
      out << " wrapped";
    }
    if (_flags & F_pointer) {
      out << " pointer";
    }
    if (_flags & F_const) {
      out << " const";
    }
    if (_flags & F_struct) {
      out << " struct";
    }
    if (_flags & F_class) {
      out << " class";
    }
    if (_flags & F_union) {
      out << " union";
    }
    if (_flags & F_fully_defined) {
      out << " fully_defined";
    }
    if (_flags & F_true_destructor) {
      out << " true_destructor";
    }
    if (_flags & F_private_destructor) {
      out << " private_destructor";
    }
    if (_flags & F_inherited_destructor) {
      out << " inherited_destructor";
    }
    if (_flags & F_implicit_destructor) {
      out << " implicit_destructor";
    }
    if (_flags & F_nested) {
      out << " nested";
    }
    if (_flags & F_enum) {
      out << " enum";
    }
    if (_flags & F_unpublished) {
      out << " unpublished";
    }
    if (_flags & F_typedef) {
      out << " typedef";
    }
    if (_flags & F_array) {
      out << " array[";
      out << _array_size << "]";
    }
    if (_flags & F_scoped_enum) {
      out << " scoped_enum";
    }
    if (_flags & F_final) {
      out << " final";
    }
    if (_flags & F_deprecated) {
      out << " deprecated";
    }
    out << "\n";
  }

  if (_atomic_token != AT_not_atomic) {
    indent(out, indent_level + 2) << "atomic_token: ";
    switch (_atomic_token) {
    case AT_int:
      out << "int\n";
      break;
    case AT_float:
      out << "float\n";
      break;
    case AT_double:
      out << "double\n";
      break;
    case AT_bool:
      out << "bool\n";
      break;
    case AT_char:
      out << "char\n";
      break;
    case AT_void:
      out << "void\n";
      break;
    case AT_string:
      out << "string\n";
      break;
    case AT_longlong:
      out << "longlong\n";
      break;
    case AT_null:
      out << "null\n";
      break;
    default:
      out << "INVALID\n";
    }
  }

  InterrogateDatabase *idb = InterrogateDatabase::get_ptr();
  if (_wrapped_type != 0) {
    indent(out, indent_level)
      << "  wrapped_type: " << idb->get_type(_wrapped_type).get_scoped_name() << "\n";
  }

  if (!_comment.empty()) {
    indent(out, indent_level) << "  comment:";
    bool next_indent = true;
    for (char c : _comment) {
      if (c == '\n') {
        next_indent = true;
      } else {
        if (next_indent) {
          out << "\n";
          indent(out, indent_level + 4);
          next_indent = false;
        }
        out << c;
      }
    }
    out << "\n";
  }

  for (const Derivation &deriv : _derivations) {
    indent(out, indent_level) << "  derivation \"" << idb->get_type(deriv._base).get_name() << "\" {\n";
    if (deriv._flags != 0) {
      indent(out, indent_level) << "    flags:";
      if (deriv._flags & DF_upcast) {
        out << " upcast";
      }
      if (deriv._flags & DF_downcast) {
        out << " downcast";
      }
      if (deriv._flags & DF_downcast_impossible) {
        out << " downcast_impossible";
      }
      out << "\n";
    }

    if (deriv._upcast != 0) {
      //out << "\n";
      //idb->get_function(deriv._upcast).write(out, indent_level + 4, "upcast");
      indent(out, indent_level + 4) << "upcast: "
        << idb->get_function(deriv._upcast).get_scoped_name() << "\n";
    }
    if (deriv._downcast != 0) {
      //out << "\n";
      //idb->get_function(deriv._downcast).write(out, indent_level + 4, "downcast");
      indent(out, indent_level + 4) << "downcast: "
        << idb->get_function(deriv._downcast).get_scoped_name() << "\n";
    }
    indent(out, indent_level) << "  }\n";
  }

  if (!_constructors.empty() || _destructor != 0 || !_methods.empty() ||
      !_casts.empty() || !_elements.empty() || !_make_seqs.empty() ||
      !_nested_types.empty()) {
    out << "\n";
  }

  for (FunctionIndex index : _constructors) {
    //idb->get_function(index).write(out, indent_level + 2, "constructor");
    indent(out, indent_level)
      << "  constructor: " << idb->get_function(index).get_name() << "\n";
  }

  if (_destructor != 0) {
    //idb->get_function(_destructor).write(out, indent_level + 2, "destructor");
    indent(out, indent_level)
      << "  destructor: " << idb->get_function(_destructor).get_name() << "\n";
  }

  for (FunctionIndex index : _methods) {
    //idb->get_function(index).write(out, indent_level + 2, "method");
    indent(out, indent_level)
      << "  method: " << idb->get_function(index).get_name() << "\n";
  }

  for (FunctionIndex index : _casts) {
    //idb->get_function(index).write(out, indent_level + 2, "cast");
    indent(out, indent_level)
      << "  cast: " << idb->get_function(index).get_name() << "\n";
  }

  for (ElementIndex index : _elements) {
    //idb->get_element(index).write(out, indent_level + 2);
    indent(out, indent_level)
      << "  element: " << idb->get_element(index).get_name() << "\n";
  }

  for (MakeSeqIndex index : _make_seqs) {
    //idb->get_make_seq(index).write(out, indent_level + 2);
    indent(out, indent_level)
      << "  make_seq: " << idb->get_make_seq(index).get_name() << "\n";
  }

  for (TypeIndex index : _nested_types) {
    //idb->get_type(index).write(out, indent_level + 2);
    indent(out, indent_level)
      << "  type: " << idb->get_type(index).get_name() << "\n";
  }
  indent(out, indent_level) << "}\n";
}

/**
 * Formats the InterrogateType data for output to a data file.
 */
void InterrogateType::
output(ostream &out) const {
  InterrogateComponent::output(out);

  out << _flags << " ";
  idf_output_string(out, _scoped_name);
  idf_output_string(out, _true_name);
  out << _outer_class << " "
      << (int)_atomic_token << " "
      << _wrapped_type << " ";

  if (is_array()) {
    out << _array_size << " ";
  }

  idf_output_vector(out, _constructors);
  out << _destructor << " ";
  idf_output_vector(out, _elements);
  idf_output_vector(out, _methods);
  idf_output_vector(out, _make_seqs);
  idf_output_vector(out, _casts);
  idf_output_vector(out, _derivations);
  idf_output_vector(out, _enum_values);
  idf_output_vector(out, _nested_types);
  idf_output_string(out, _comment, '\n');
}

/**
 * Reads the data file as previously formatted by output().
 */
void InterrogateType::
input(istream &in) {
  InterrogateComponent::input(in);

  in >> _flags;
  idf_input_string(in, _scoped_name);
  idf_input_string(in, _true_name);

  in >> _outer_class;
  int token;
  in >> token;
  _atomic_token = (AtomicToken)token;
  in >> _wrapped_type;

  if (is_array()) {
    in >> _array_size;
  }

  idf_input_vector(in, _constructors);
  in >> _destructor;

  idf_input_vector(in, _elements);
  idf_input_vector(in, _methods);
  idf_input_vector(in, _make_seqs);
  idf_input_vector(in, _casts);
  idf_input_vector(in, _derivations);
  idf_input_vector(in, _enum_values);
  idf_input_vector(in, _nested_types);
  idf_input_string(in, _comment);
}

/**
 * Remaps all internal index numbers according to the indicated map.  This
 * called from InterrogateDatabase::remap_indices().
 */
void InterrogateType::
remap_indices(const IndexRemapper &remap) {
  _outer_class = remap.map_from(_outer_class);
  _wrapped_type = remap.map_from(_wrapped_type);

  Functions::iterator fi;
  for (fi = _constructors.begin(); fi != _constructors.end(); ++fi) {
    (*fi) = remap.map_from(*fi);
  }
  _destructor = remap.map_from(_destructor);

  Elements::iterator ei;
  for (ei = _elements.begin(); ei != _elements.end(); ++ei) {
    (*ei) = remap.map_from(*ei);
  }

  for (fi = _methods.begin(); fi != _methods.end(); ++fi) {
    (*fi) = remap.map_from(*fi);
  }
  for (fi = _casts.begin(); fi != _casts.end(); ++fi) {
    (*fi) = remap.map_from(*fi);
  }

  MakeSeqs::iterator si;
  for (si = _make_seqs.begin(); si != _make_seqs.end(); ++si) {
    (*si) = remap.map_from(*si);
  }

  Derivations::iterator di;
  for (di = _derivations.begin(); di != _derivations.end(); ++di) {
    (*di)._base = remap.map_from((*di)._base);
    (*di)._upcast = remap.map_from((*di)._upcast);
    (*di)._downcast = remap.map_from((*di)._downcast);
  }

  Types::iterator ti;
  for (ti = _nested_types.begin(); ti != _nested_types.end(); ++ti) {
    (*ti) = remap.map_from(*ti);
  }

}
