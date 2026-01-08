/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file interrogateFunctionWrapper.cxx
 * @author drose
 * @date 2000-08-06
 */

#include "interrogateFunctionWrapper.h"
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
void InterrogateFunctionWrapper::Parameter::
output(ostream &out) const {
  idf_output_string(out, _name);
  out << _parameter_flags << " " << _type << " ";
}

/**
 *
 */
void InterrogateFunctionWrapper::Parameter::
input(istream &in) {
  idf_input_string(in, _name);
  in >> _parameter_flags >> _type;
}

/**
 * Formats the function wrapper in a human-readable manner.
 */
void InterrogateFunctionWrapper::
write(std::ostream &out, int indent_level, const char *tag) const {
  indent(out, indent_level) << tag;
  if (has_name()) {
    out << " ";
    write_names(out);
  }
  out << " {\n";

  if (_flags != 0) {
    indent(out, indent_level) << "  flags:";
    if (_flags & F_caller_manages) {
      out << " caller_manages";
    }
    if (_flags & F_has_return) {
      out << " has_return";
    }
    if (_flags & F_callable_by_name) {
      out << " callable_by_name";
    }
    if (_flags & F_copy_constructor) {
      out << " copy_constructor";
    }
    if (_flags & F_coerce_constructor) {
      out << " coerce_constructor";
    }
    if (_flags & F_extension) {
      out << " extension";
    }
    if (_flags & F_deprecated) {
      out << " deprecated";
    }
    out << "\n";
  }

  InterrogateDatabase *idb = InterrogateDatabase::get_ptr();
  if (_return_type != 0) {
    indent(out, indent_level) << "  return_type: " << idb->get_type(_return_type).get_scoped_name() << "\n";
  }

  if (!_unique_name.empty()) {
    indent(out, indent_level) << "  unique_name: " << _unique_name << "\n";
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

  if (!_parameters.empty()) {
    out << "\n";
  }
  for (const Parameter &param : _parameters) {
    indent(out, indent_level + 2);
    out << "parameter";
    if (param._parameter_flags & PF_has_name) {
      out << " \"" << param._name << "\"";
    }
    if (param._parameter_flags & PF_is_this) {
      out << " (this)";
    }
    if (param._parameter_flags & PF_is_optional) {
      out << " (optional)";
    }
    out << ": " << idb->get_type(param._type).get_scoped_name() << "\n";
  }

  indent(out, indent_level) << "}\n";
}

/**
 * Formats the InterrogateFunctionWrapper data for output to a data file.
 */
void InterrogateFunctionWrapper::
output(ostream &out) const {
  InterrogateComponent::output(out);
  out << _flags << " "
      << _function << " "
      << _return_type << " "
      << _return_value_destructor << " ";
  idf_output_string(out, _unique_name);
  idf_output_string(out, _comment);
  idf_output_vector(out, _parameters);
}

/**
 * Reads the data file as previously formatted by output().
 */
void InterrogateFunctionWrapper::
input(istream &in) {
  InterrogateComponent::input(in);
  in >> _flags
     >> _function
     >> _return_type
     >> _return_value_destructor;
  idf_input_string(in, _unique_name);
  idf_input_string(in, _comment);
  idf_input_vector(in, _parameters);
}

/**
 * Remaps all internal index numbers according to the indicated map.  This
 * called from InterrogateDatabase::remap_indices().
 */
void InterrogateFunctionWrapper::
remap_indices(const IndexRemapper &remap) {
  _function = remap.map_from(_function);
  _return_value_destructor = remap.map_from(_return_value_destructor);
  _return_type = remap.map_from(_return_type);

  Parameters::iterator pi;
  for (pi = _parameters.begin(); pi != _parameters.end(); ++pi) {
    (*pi)._type = remap.map_from((*pi)._type);
  }
}
