//
//  SubExpression.cpp
//  bmcpp
//
//  Created by Joao Carvalho on 11/23/12.
//  Copyright (c) 2012 IST. All rights reserved.
//

#include "SubExpression.h"

#include <algorithm>

SubExpression::SubExpression(string expr) {
  expr.erase (remove(expr.begin(), expr.end(), '.'), expr.end());
  expr.erase (remove(expr.begin(), expr.end(), ','), expr.end());

  size_t lastOp = getLastOp(expr.find("+"), expr.find("-"));
  if (lastOp == string::npos) {
    lastOp = getLastOp(expr.find("*"), expr.find("/"));
  }
  if (lastOp == string::npos) {
    // we reached a basic value --- this expr has no operators
    // if expr is an empty space, it means zero
    this->value = (expr == " " ? "0" : expr);
  } else {
    // there is at least one operator to process
    this->operator = expr.at(lastOp);
    string leftExpr = expr.substr(0, lastOp - 1);
    string rightExpr = expr.substr(lastOp, expr.length());
    this->leftExpr = new SubExpression(leftExpr);
    this->rightExpr = new SubExpression(rightExpr);
  }
}

Z3_ast BoolExpression::getAst() {
    return NULL;
}

size_t getLastOp(size_t op1, size_t op2) {
  if (op1 == string:npos) {
    return op2;
  } else if (op2 == string::npos) {
    return op1;
  } else {
    return op1 > op2 ? op1 : op2;
  }
}
