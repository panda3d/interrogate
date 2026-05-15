/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file parameterRemapBasicStringViewToStringView.h
 * @author rdb
 * @date 2026-05-15
 */

#ifndef PARAMETERREMAPBASICSTRINGVIEWTOSTRINGVIEW_H
#define PARAMETERREMAPBASICSTRINGVIEWTOSTRINGVIEW_H

#include "dtoolbase.h"

#include "parameterRemap.h"

/**
 * Maps a concrete basic_string_view<char> to an atomic string.
 */
class ParameterRemapBasicStringViewToStringView : public ParameterRemap {
public:
  ParameterRemapBasicStringViewToStringView(CPPType *orig_type);

  virtual void pass_parameter(std::ostream &out, const std::string &variable_name);
  virtual std::string prepare_return_expr(std::ostream &out, int indent_level,
                                     const std::string &expression);
  virtual std::string get_return_expr(const std::string &expression);

  virtual bool new_type_is_atomic_string();
};

/**
 * Maps a concrete basic_string_view<wchar_t> to an atomic string.
 */
class ParameterRemapBasicWStringViewToWStringView : public ParameterRemap {
public:
  ParameterRemapBasicWStringViewToWStringView(CPPType *orig_type);

  virtual void pass_parameter(std::ostream &out, const std::string &variable_name);
  virtual std::string prepare_return_expr(std::ostream &out, int indent_level,
                                     const std::string &expression);
  virtual std::string get_return_expr(const std::string &expression);

  virtual bool new_type_is_atomic_string();
};

#endif
