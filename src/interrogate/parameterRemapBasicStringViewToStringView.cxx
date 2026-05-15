/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file parameterRemapBasicStringViewToStringView.cxx
 * @author rdb
 * @date 2026-05-15
 */

#include "parameterRemapBasicStringViewToStringView.h"
#include "interfaceMaker.h"
#include "interrogate.h"

using std::ostream;
using std::string;

/**
 *
 */
ParameterRemapBasicStringViewToStringView::
ParameterRemapBasicStringViewToStringView(CPPType *orig_type) :
  ParameterRemap(orig_type)
{
  static CPPType *const_char_star_type = nullptr;
  if (const_char_star_type == nullptr) {
    const_char_star_type = parser.parse_type("const char *");
  }

  _new_type = const_char_star_type;
}

/**
 * Outputs an expression that converts the indicated variable from the
 * original type to the new type, for passing into the actual C++ function.
 */
void ParameterRemapBasicStringViewToStringView::
pass_parameter(ostream &out, const string &variable_name) {
  out << "std::string_view(" << variable_name << ")";
}

/**
 * This will be called immediately before get_return_expr().  It outputs
 * whatever lines the remapper needs to the function to set up its return
 * value, e.g.  to declare a temporary variable or something.  It should
 * return the modified expression.
 */
string ParameterRemapBasicStringViewToStringView::
prepare_return_expr(ostream &out, int indent_level, const string &expression) {
  // String views aren't null-terminated, so we still have to copy it.
  InterfaceMaker::indent(out, indent_level)
    << "static std::string string_holder = " << expression << ";\n";
  return "string_holder";
}

/**
 * Returns an expression that evalutes to the appropriate value type for
 * returning from the function, given an expression of the original type.
 */
string ParameterRemapBasicStringViewToStringView::
get_return_expr(const string &expression) {
  return "string_holder.c_str()";
}

/**
 * Returns true if the type represented by the conversion is now the atomic
 * string type.  We have to have this crazy method for representing atomic
 * string, because there's no such type in C (and hence no corresponding
 * CPPType *).
 */
bool ParameterRemapBasicStringViewToStringView::
new_type_is_atomic_string() {
  return true;
}

/**
 *
 */
ParameterRemapBasicWStringViewToWStringView::
ParameterRemapBasicWStringViewToWStringView(CPPType *orig_type) :
  ParameterRemap(orig_type)
{
  static CPPType *const_wchar_star_type = nullptr;
  if (const_wchar_star_type == nullptr) {
    const_wchar_star_type = parser.parse_type("const wchar_t *");
  }

  _new_type = const_wchar_star_type;
}

/**
 * Outputs an expression that converts the indicated variable from the
 * original type to the new type, for passing into the actual C++ function.
 */
void ParameterRemapBasicWStringViewToWStringView::
pass_parameter(ostream &out, const string &variable_name) {
  out << "std::wstring_view(" << variable_name << ")";
}

/**
 * This will be called immediately before get_return_expr().  It outputs
 * whatever lines the remapper needs to the function to set up its return
 * value, e.g.  to declare a temporary variable or something.  It should
 * return the modified expression.
 */
string ParameterRemapBasicWStringViewToWStringView::
prepare_return_expr(ostream &out, int indent_level, const string &expression) {
  // String views aren't null-terminated, so we still have to copy it.
  InterfaceMaker::indent(out, indent_level)
    << "static std::wstring string_holder = " << expression << ";\n";
  return "string_holder";
}

/**
 * Returns an expression that evalutes to the appropriate value type for
 * returning from the function, given an expression of the original type.
 */
string ParameterRemapBasicWStringViewToWStringView::
get_return_expr(const string &expression) {
  return "string_holder.c_str()";
}

/**
 * Returns true if the type represented by the conversion is now the atomic
 * string type.  We have to have this crazy method for representing atomic
 * string, because there's no such type in C (and hence no corresponding
 * CPPType *).
 */
bool ParameterRemapBasicWStringViewToWStringView::
new_type_is_atomic_string() {
  return true;
}
