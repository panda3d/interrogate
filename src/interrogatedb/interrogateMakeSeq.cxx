/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file interrogateMakeSeq.cxx
 * @author drose
 * @date 2009-09-15
 */

#include "interrogateMakeSeq.h"
#include "indexRemapper.h"
#include "interrogate_datafile.h"
#include "interrogateDatabase.h"
#include "indent.h"

/**
 * Formats the make_seq in a human-readable manner.
 */
void InterrogateMakeSeq::
write(std::ostream &out, int indent_level) const {
  indent(out, indent_level) << "make_seq ";
  write_names(out);
  out << " {\n";

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

  if (_length_getter != 0 || _element_getter != 0) {
    InterrogateDatabase *idb = InterrogateDatabase::get_ptr();
    if (_length_getter != 0) {
      //idb->get_function(_length_getter).write(out, indent_level + 2, "length getter");
      indent(out, indent_level + 2) << "length_getter: "
        << idb->get_function(_length_getter).get_scoped_name() << "\n";
    }

    if (_element_getter != 0) {
      //out << "\n";
      //idb->get_function(_element_getter).write(out, indent_level + 2, "element getter");
      indent(out, indent_level + 2) << "element_getter: "
        << idb->get_function(_element_getter).get_scoped_name() << "\n";
    }
  }

  indent(out, indent_level) << "}\n";
}

/**
 * Formats the InterrogateMakeSeq data for output to a data file.
 */
void InterrogateMakeSeq::
output(std::ostream &out) const {
  InterrogateComponent::output(out);
  out << _length_getter << " "
      << _element_getter << " ";
  idf_output_string(out, _scoped_name);
  idf_output_string(out, _comment, '\n');
}

/**
 * Reads the data file as previously formatted by output().
 */
void InterrogateMakeSeq::
input(std::istream &in) {
  InterrogateComponent::input(in);

  in >> _length_getter >> _element_getter;
  idf_input_string(in, _scoped_name);
  idf_input_string(in, _comment);
}

/**
 * Remaps all internal index numbers according to the indicated map.  This
 * called from InterrogateDatabase::remap_indices().
 */
void InterrogateMakeSeq::
remap_indices(const IndexRemapper &remap) {
  _length_getter = remap.map_from(_length_getter);
  _element_getter = remap.map_from(_element_getter);
}
