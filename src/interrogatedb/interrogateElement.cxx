/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file interrogateElement.cxx
 * @author drose
 * @date 2000-08-11
 */

#include "interrogateElement.h"
#include "interrogateDatabase.h"
#include "indexRemapper.h"
#include "interrogate_datafile.h"
#include "indent.h"

/**
 * Formats the element in a human-readable manner.
 */
void InterrogateElement::
write(std::ostream &out, int indent_level) const {
  indent(out, indent_level) << "element \"" << get_scoped_name() << "\"";
  //write_names(out);
  out << " {\n";

  InterrogateDatabase *idb = InterrogateDatabase::get_ptr();
  if (_type != 0) {
    indent(out, indent_level) << "  type: " << idb->get_type(_type).get_scoped_name() << "\n";
  }

  if (_flags != 0) {
    indent(out, indent_level) << "  flags:";
    if (_flags & F_global) {
      out << " global";
    }
    if (_flags & F_has_getter) {
      out << " has_getter";
    }
    if (_flags & F_has_setter) {
      out << " has_setter";
    }
    if (_flags & F_has_has_function) {
      out << " has_has_function";
    }
    if (_flags & F_has_clear_function) {
      out << " has_clear_function";
    }
    if (_flags & F_has_del_function) {
      out << " has_del_function";
    }
    if (_flags & F_sequence) {
      out << " sequence";
    }
    if (_flags & F_mapping) {
      out << " mapping";
    }
    if (_flags & F_has_insert_function) {
      out << " has_insert_function";
    }
    if (_flags & F_has_getkey_function) {
      out << " has_getkey_function";
    }
    out << "\n";
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

  //out << "\n";

  if (_length_function != 0) {
    //idb->get_function(_length_function).write(out, indent_level + 2, "length function");
    indent(out, indent_level + 2) << "length_function: "
      << idb->get_function(_length_function).get_scoped_name() << "\n";
  }

  if (_getter != 0) {
    //idb->get_function(_getter).write(out, indent_level + 2, "getter");
    indent(out, indent_level + 2) << "getter: "
      << idb->get_function(_getter).get_scoped_name() << "\n";
  }

  if (_setter != 0) {
    //idb->get_function(_setter).write(out, indent_level + 2, "setter");
    indent(out, indent_level + 2) << "setter: "
      << idb->get_function(_setter).get_scoped_name() << "\n";
  }

  if (_has_function != 0) {
    //idb->get_function(_has_function).write(out, indent_level + 2, "has function");
    indent(out, indent_level + 2) << "has_function: "
      << idb->get_function(_has_function).get_scoped_name() << "\n";
  }

  if (_clear_function != 0) {
    //idb->get_function(_clear_function).write(out, indent_level + 2, "clear function");
    indent(out, indent_level + 2) << "clear_function: "
      << idb->get_function(_clear_function).get_scoped_name() << "\n";
  }

  if (_del_function != 0) {
    //idb->get_function(_del_function).write(out, indent_level + 2, "del function");
    indent(out, indent_level + 2) << "del_function: "
      << idb->get_function(_del_function).get_scoped_name() << "\n";
  }

  if (_insert_function != 0) {
    //idb->get_function(_insert_function).write(out, indent_level + 2, "insert function");
    indent(out, indent_level + 2) << "insert_function: "
      << idb->get_function(_insert_function).get_scoped_name() << "\n";
  }

  if (_getkey_function != 0) {
    //idb->get_function(_getkey_function).write(out, indent_level + 2, "getkey function");
    indent(out, indent_level + 2) << "getkey_function: "
      << idb->get_function(_getkey_function).get_scoped_name() << "\n";
  }

  indent(out, indent_level) << "}\n";
}

/**
 * Formats the InterrogateElement data for output to a data file.
 */
void InterrogateElement::
output(std::ostream &out) const {
  InterrogateComponent::output(out);
  out << _flags << " "
      << _type << " "
      << _getter << " "
      << _setter << " "
      << _has_function << " "
      << _clear_function << " "
      << _del_function << " "
      << _length_function << " "
      << _insert_function << " "
      << _getkey_function << " ";
  idf_output_string(out, _scoped_name);
  idf_output_string(out, _comment, '\n');
}

/**
 * Reads the data file as previously formatted by output().
 */
void InterrogateElement::
input(std::istream &in) {
  InterrogateComponent::input(in);
  in >> _flags >> _type >> _getter >> _setter;
  if (InterrogateDatabase::get_file_minor_version() >= 1) {
    in >> _has_function >> _clear_function;
    if (InterrogateDatabase::get_file_minor_version() >= 2) {
      in >> _del_function >> _length_function;
      if (InterrogateDatabase::get_file_minor_version() >= 3) {
        in >> _insert_function >> _getkey_function;
      }
    }
  }
  idf_input_string(in, _scoped_name);
  idf_input_string(in, _comment);
}

/**
 * Remaps all internal index numbers according to the indicated map.  This
 * called from InterrogateDatabase::remap_indices().
 */
void InterrogateElement::
remap_indices(const IndexRemapper &remap) {
  _type = remap.map_from(_type);
  _getter = remap.map_from(_getter);
  _setter = remap.map_from(_setter);
  _has_function = remap.map_from(_has_function);
  _clear_function = remap.map_from(_clear_function);
  _del_function = remap.map_from(_del_function);
  _insert_function = remap.map_from(_insert_function);
  _getkey_function = remap.map_from(_getkey_function);
  _length_function = remap.map_from(_length_function);
}
