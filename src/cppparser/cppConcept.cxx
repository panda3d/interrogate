/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file cppConcept.cxx
 * @author rdb
 * @date 2026-01-11
 */

#include "cppConcept.h"
#include "cppExpression.h"
#include "cppIdentifier.h"
#include "cppSimpleType.h"
#include "cppTemplateScope.h"
#include "indent.h"

/**
 * Constructs a new CPPConcept object for the given type.
 */
CPPConcept::
CPPConcept(CPPIdentifier *ident, CPPExpression *initializer) :
  CPPInstance(CPPType::new_type(new CPPSimpleType(CPPSimpleType::T_bool)), ident)
{
  _initializer = initializer;
}

/**
 *
 */
CPPDeclaration *CPPConcept::
substitute_decl(CPPDeclaration::SubstDecl &subst,
                CPPScope *current_scope, CPPScope *global_scope) {
  CPPDeclaration *decl =
    CPPInstance::substitute_decl(subst, current_scope, global_scope);
  assert(decl != nullptr);
  if (decl->as_concept()) {
    return decl;
  }
  assert(decl->as_instance() != nullptr);
  return new CPPConcept(_ident, _initializer);
}

/**
 *
 */
void CPPConcept::
output(std::ostream &out, int indent_level, CPPScope *scope, bool) const {
  if (is_template()) {
    get_template_scope()->_parameters.write_formal(out, scope);
    indent(out, indent_level);
  }

  if (!_attributes.is_empty()) {
    out << _attributes << " ";
  }

  out << "concept";

  if (_ident != nullptr) {
    out << " " << _ident->get_local_name(scope);
  }

  if (_initializer != nullptr) {
    out << " = " << *_initializer;
  }
}

/**
 *
 */
CPPDeclaration::SubType CPPConcept::
get_subtype() const {
  return ST_concept;
}

/**
 *
 */
CPPConcept *CPPConcept::
as_concept() {
  return this;
}
