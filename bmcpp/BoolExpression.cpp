//
//  BoolExpression.cpp
//  bmcpp
//
//  Created by Joao Carvalho on 11/23/12.
//  Copyright (c) 2012 IST. All rights reserved.
//

#include "BoolExpression.h"

const string BoolExpression::operators[] = { "=", "<=", ">=", "<", ">"};

BoolExpression::BoolExpression(string expr) {
  size_t opIdx = string::npos;
  for (int i = 0; i < 5; i++) {
    opIdx = expr.find(operators[i]);
    if (opIdx != string::npos) {
      this->operator = operators[i];
      break;
    }
  }
  if (opIdx == string::npos) {
    std::cout << "Incorrect expr operator: " << expr << std::endl;
  }
  
  string leftExpr = expr.substr(0, opIdx - 1);
  string rightExpr = expr.substr(opIdx + this->operator.length(), expr.length());
  this->leftOperand = new SubExpression(leftExpr);
  this->rightOperand = new SubExpression(rightExpr);
}

Z3_ast BoolExpression::getAst() {
    return NULL;
}
