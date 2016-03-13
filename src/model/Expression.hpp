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
#ifndef EXPRESSION_HPP
#define EXPRESSION_HPP

#include <string>
#include <iostream>
#include <memory>

class ConstExpression;
class NameExpression;
class BiExpression;

class Expression {
protected:
  Expression() {}
public:
  ~Expression() {}

public:
  class Visitor {
  protected:
    Visitor() {}
    ~Visitor() {}

  public:
    virtual void visit(ConstExpression const &expr) = 0;
    virtual void visit(NameExpression  const &expr) = 0;
    virtual void visit(BiExpression    const &expr) = 0;
  };
  virtual void accept(Visitor &vis) const = 0;
};

class ConstExpression : public Expression {
  int const  m_val;

public:
  ConstExpression(int const  val) : m_val(val) {}
  ~ConstExpression();

public:
  int value() const { return  m_val; }

public:
  void accept(Visitor &vis) const;
};

class NameExpression : public Expression {
  std::string const  m_name;

public:
  NameExpression(std::string const &name) : m_name(name) {}
  ~NameExpression();

public:
  std::string const& name() const { return  m_name; }

public:
  void accept(Visitor &vis) const;
};

class BiExpression : public Expression {
public:
  enum class Op { AND, OR, XOR, ADD, SUB, MUL, DIV, MOD, POW, SEL };
  static std::array<char const *const, 11>  OPS;

private:
  Op const  m_op;
  std::shared_ptr<Expression const>  m_left, m_right;

public:
  BiExpression(Op const  op,
	       std::shared_ptr<Expression const>  left,
	       std::shared_ptr<Expression const>  right) 
   : m_op(op), m_left(left), m_right(right) {}
  ~BiExpression();

public:
  Op op() const { return  m_op; }
  Expression const& lhs() const { return *m_left; }
  Expression const& rhs() const { return *m_right; }

public:
  void accept(Visitor &vis) const;
};

class ExpressionPrinter : public Expression::Visitor {
  std::ostream &m_out;
public:
  ExpressionPrinter(std::ostream &out) : m_out(out) {}
  ~ExpressionPrinter() {}

public:
  void visit(ConstExpression const &expr);
  void visit(NameExpression const &expr);
  void visit(BiExpression const &expr);
};

inline std::ostream &operator<<(std::ostream &out, Expression const& expr) {
  ExpressionPrinter  printer(out);
  expr.accept(printer);
  return  out;
}
#endif
