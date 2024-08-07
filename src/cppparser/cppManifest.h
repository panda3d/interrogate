/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file cppManifest.h
 * @author drose
 * @date 1999-10-22
 */

#ifndef CPPMANIFEST_H
#define CPPMANIFEST_H

//#include "dtoolbase.h"

#include "cppFile.h"
#include "cppVisibility.h"
#include "cppBisonDefs.h"

#include "vector_string.h"
#include <unordered_set>

class CPPExpression;
class CPPType;

/**
 *
 */
class CPPManifest {
public:
  typedef std::unordered_set<const CPPManifest *> Ignores;

  CPPManifest(const CPPPreprocessor &parser, const std::string &args, const cppyyltype &loc);
  CPPManifest(const CPPPreprocessor &parser, const std::string &macro, const std::string &definition);
  ~CPPManifest();

  static std::string stringify(const std::string &source);
  void extract_args(vector_string &args, const std::string &expr, size_t &p) const;
  std::string expand(const vector_string &args = vector_string(),
                     bool expand_undefined = false,
                     const Ignores &ignores = Ignores()) const;


  CPPType *determine_type() const;

  bool is_equal(const CPPManifest *other) const;
  void output(std::ostream &out) const;

  const CPPPreprocessor &_parser;
  std::string _name;
  bool _has_parameters;
  size_t _num_parameters;
  int _variadic_param;
  cppyyltype _loc;
  CPPExpression *_expr;

  // Manifests don't have a visibility in the normal sense.  Normally this
  // will be V_public.  But a manifest that is defined between __begin_publish
  // and __end_publish will have a visibility of V_published.
  CPPVisibility _vis;

private:
  class ExpansionNode {
  public:
    ExpansionNode(int parm_number, bool stringify, bool paste);
    ExpansionNode(const std::string &str, bool paste = false);
    ExpansionNode(std::vector<ExpansionNode> nested, bool stringify = false, bool paste = false, bool optional = false);

    bool operator ==(const ExpansionNode &other) const;

    int _parm_number;
    bool _expand;
    bool _stringify;
    bool _paste;
    bool _optional;
    std::string _str;
    std::vector<ExpansionNode> _nested;
  };
  typedef std::vector<ExpansionNode> Expansion;

  void parse_parameters(const std::string &args, size_t &p,
                        vector_string &parameter_names);
  void save_expansion(Expansion &expansion, const std::string &exp,
                      const vector_string &parameter_names);

  std::string r_expand(const Expansion &expansion, const vector_string &args,
                       bool expand_undefined, const Ignores &ignores) const;

  Expansion _expansion;
};

inline std::ostream &operator << (std::ostream &out, const CPPManifest &manifest) {
  manifest.output(out);
  return out;
}

#endif
