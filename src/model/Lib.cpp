/*****************************************************************************
 * This file is part of the QBM (Quantified Binary Matching) program.
 *
 * Copyright (C) 2016
 *      Thomas B. Preusser <thomas.preusser@utexas.edu>
 *****************************************************************************
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/
#include "Lib.hpp"

#include "Root.hpp"
#include "CompDecl.hpp"

CompDecl& Lib::declareComponent(std::string const &name) {
  auto const  res = m_components.emplace(std::piecewise_construct,
					 std::forward_as_tuple(name),
					 std::forward_as_tuple(name));
  if(!res.second)  throw "Component type " + name + " already declared.";
  return  res.first->second;
}
Component const& Lib::compile(std::string const &top) {
  CompDecl const &decl = m_components.at(top);
  m_root.reset(new Root(decl));
  return  m_root->top();
}