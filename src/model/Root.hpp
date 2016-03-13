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
#ifndef ROOT_HPP
#define ROOT_HPP

#include "Node.hpp"
#include "Bus.hpp"
#include "Statement.hpp"
#include "Component.hpp"

class Root : public Context {
  static constexpr Node const  FIRST_CONFIG = Node::TOP>Node::BOT? Node::TOP : Node::BOT;
  static constexpr Node const  FIRST_INPUT  = 0x3F000000;
  static constexpr Node const  FIRST_SIGNAL = 0x40000000;

  std::vector<int>  m_clauses;
  unsigned  m_confignxt;
  unsigned  m_inputnxt;
  unsigned  m_signalnxt;

  Instantiation const  m_inst;
  Component     const  m_top;

public:
  Root(CompDecl const &decl)
    : m_confignxt(FIRST_CONFIG),
      m_inputnxt (FIRST_INPUT),
      m_signalnxt(FIRST_SIGNAL),
      m_inst("<top>", decl), m_top(*this, m_inst) {
    for(int const  i : m_clauses)  std::cout << i << " ";
    std::cout << std::endl;
  }
  ~Root() {}

public:
  Bus allocateConfig(unsigned  width) override;
  Bus allocateInput (unsigned  width) override;
  Bus allocateSignal(unsigned  width) override;

public:
  void addClause(int const *beg, int const *end) override;

public:
  Component const& top() const { return  m_top; }
};
#endif