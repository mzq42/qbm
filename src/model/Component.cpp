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
#include "Component.hpp"

#include "Root.hpp"
#include "CompDecl.hpp"
#include "Statement.hpp"

#include <cmath>
#include <functional>

Component::Component(Root &parent, Instantiation const &inst)
  : m_parent(parent), m_inst(inst) {

  // define busses for all ports
  inst.decl().forAllPorts([this](PortDecl const &decl) {
      int const  width = computeConstant(decl.width());
      registerBus(decl.name(), decl.direction() == PortDecl::Direction::in? allocateInput(width) : allocateSignal(width));
    });

  // execute statements
  compile();
}
Component::Component(Component &parent, Instantiation const &inst,
		     std::map<std::string, int> &params,
		     std::map<std::string, Bus> &connects)
  : m_parent(parent), m_inst(inst) {
  std::swap(params, m_constants); // import generics
  std::swap(connects, m_busses);  // import port connections
  compile();
}

void Component::compile() {
  std::cerr << "Compiling " << m_inst.label() << " ..." << std::endl;
  m_inst.decl().forAllStatements([this](Statement const& stmt) {
      std::cerr << stmt << std::endl;
      stmt.execute(*this);
    });
  std::cerr << "Compiling " << m_inst.label() << " done." << std::endl;
}

Bus Component::allocateConfig(unsigned  width) { return  m_parent.allocateConfig(width); }
Bus Component::allocateInput (unsigned  width) { return  m_parent.allocateInput (width); }
Bus Component::allocateSignal(unsigned  width) { return  m_parent.allocateSignal(width); }
void Component::addClause(int const *beg, int const *end) {
  m_parent.addClause(beg, end);
}

void Component::defineConstant(std::string const &name, int val) {
  if(!m_constants.emplace(name, val).second) {
    throw "Constant " + name + " already defined.";
  }
}
int Component::resolveConstant(std::string const &name) const {
  auto const  it = m_constants.find(name);
  if(it != m_constants.end())  return  it->second;
  throw  m_inst.decl().name() + ": " + name + " is not defined.";
}
int Component::computeConstant(Expression const &expr) const {
  class Computer : public Expression::Visitor {
    Component const &m_ctx;
  public:
    int m_val;

  public:
    Computer(Component const &ctx) : m_ctx(ctx) {}
    ~Computer() {}

  public:
    void visit(ConstExpression const &expr) {
      m_val = expr.value();
    }
    void visit(NameExpression const &expr) {
      m_val = m_ctx.resolveConstant(expr.name());
    }
    void visit(BiExpression const &expr) {
      expr.lhs().accept(*this);
      int const  lhs = m_val;
      expr.rhs().accept(*this);
      int const  rhs = m_val;
      switch(expr.op()) {
      case BiExpression::Op::ADD: m_val = lhs + rhs; break;
      case BiExpression::Op::SUB: m_val = lhs - rhs; break;
      case BiExpression::Op::MUL: m_val = lhs * rhs; break;
      case BiExpression::Op::DIV: m_val = lhs / rhs; break;
      case BiExpression::Op::MOD: m_val = lhs % rhs; break;
      case BiExpression::Op::POW: m_val = (int)roundl(pow(lhs, rhs)); break;
      default: throw "Unsupported Operation.";
      }
    }
  };
  Computer  comp(*this);
  expr.accept(comp);
  return  comp.m_val;
}

void Component::registerBus(std::string const &name, Bus const &bus) {
  if(!m_busses.emplace(name, bus).second) {
    throw "Bus " + name + " already defined.";
  }
}

Bus Component::resolveBus(std::string const &name) const {
  { // Name of physical bus?
    auto const  it = m_busses.find(name);
    if(it != m_busses.end())  return  it->second;
  }
  // Try a constant ...
  return  Bus(resolveConstant(name));
}

Bus Component::computeBus(Expression const &expr) {
  class Builder : public Expression::Visitor {
    Component &m_ctx;
  public:
    Bus  m_val;

  public:
    Builder(Component &ctx) : m_ctx(ctx) {}
    ~Builder() {}

  public:
    void visit(ConstExpression const &expr) {
      m_val = Bus(expr.value());
    }
    void visit(NameExpression const &expr) {
      m_val = m_ctx.resolveBus(expr.name());
    }
    void visit(BiExpression const &expr) {
      expr.lhs().accept(*this);
      Bus const  lhs = m_val;
      expr.rhs().accept(*this);
      Bus const  rhs = m_val;

      std::function<void(Node, Node, Node)>  op;
      switch(expr.op()) {
      case BiExpression::Op::AND:
	op = [this](Node y, Node a, Node b) {
	  m_ctx.addClause(y, -a, -b);
	  m_ctx.addClause(-y, a);
	  m_ctx.addClause(-y, b);
	};
	break;

      case BiExpression::Op::OR:
	op = [this](Node y, Node a, Node b) {
	  m_ctx.addClause(-y, a, b);
	  m_ctx.addClause(y, -a);
	  m_ctx.addClause(y, -b);
	};
	break;

      case BiExpression::Op::XOR:
	op = [this](Node y, Node a, Node b) {
	  m_ctx.addClause(-y, -a, -b);
	  m_ctx.addClause(-y,  a,  b);
	  m_ctx.addClause( y, -a,  b);
	  m_ctx.addClause( y,  a, -b);
	};
	break;

      case BiExpression::Op::SEL: {
	m_val = m_ctx.allocateSignal(1);
	Node     const  y = m_val[0];
	unsigned const  range = lhs.width();

	unsigned  width = 0;
	for(unsigned  r = range; r != 0; r >>= 1)  width++;
	std::unique_ptr<int[]>  clause(new int[width + 2]);
	for(unsigned  line = 0; line < range; line++) {
	  for(unsigned  i = 0; i < width; i++) {
	    clause[i] = (line & (1<<i)) == 0? -rhs[i] : (unsigned)rhs[i];
	  }
	  clause[width]   =  lhs[line];
	  clause[width+1] = -y;
	  m_ctx.addClause(clause.get(), clause.get()+width+2);
	  clause[width]   = -lhs[line];
	  clause[width+1] =  y;
	  m_ctx.addClause(clause.get(), clause.get()+width+2);
	}
	for(unsigned  line = range; line < (1u << width); line++) {
	  for(unsigned  i = 0; i < width; i++) {
	    clause[i] = (line & (1<<i)) == 0? (unsigned)rhs[i] : -rhs[i];
	  }
	  m_ctx.addClause(clause.get(), clause.get()+width);
	}
	for(unsigned  i = width; i < rhs.width(); i++) {
	  m_ctx.addClause(-rhs[i]);
	}
	return;
      }
      default:
	throw "Unsupported Operation";
      }

      Bus const  res = m_ctx.allocateSignal(std::max(lhs.width(), rhs.width()));
      for(unsigned  i = res.width(); i-- > 0;) 	op(res[i], lhs[i], rhs[i]);
      m_val = res;
      return;
    }
  };
  Builder  bld(*this);
  expr.accept(bld);
  return  bld.m_val;
}

void Component::addComponent(Instantiation        const &inst,
			     std::map<std::string, int> &params,
			     std::map<std::string, Bus> &connects) {
  std::string const &label = inst.label();
  auto const  res = m_components.emplace(std::piecewise_construct, std::forward_as_tuple(label), std::forward_as_tuple(*this, inst, params,connects));
  if(!res.second)  throw "Label " + label + " already defined.";
}
