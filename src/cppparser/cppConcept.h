/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file cppConcept.h
 * @author rdb
 * @date 2026-01-11
 */

#ifndef CPPCONCEPT_H
#define CPPCONCEPT_H

//#include "dtoolbase.h"

#include "cppInstance.h"

/**
 *
 */
class CPPConcept : public CPPInstance {
public:
  CPPConcept(CPPIdentifier *ident, CPPExpression *initializer);

  virtual CPPDeclaration *substitute_decl(SubstDecl &subst,
                                          CPPScope *current_scope,
                                          CPPScope *global_scope);

  virtual void output(std::ostream &out, int indent_level, CPPScope *scope,
                      bool complete) const;
  virtual SubType get_subtype() const;

  virtual CPPConcept *as_concept();
};

#endif
