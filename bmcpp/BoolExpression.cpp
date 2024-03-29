//
//  BoolExpression.cpp
//  bmcpp
//
//  Created by Joao Carvalho on 11/23/12.
//  Copyright (c) 2012 IST. All rights reserved.
//

#include "BoolExpression.h"

#include <cstdlib>

static const string operators[] = { "=<", ">=", "<", ">", "="};

BoolExpression::BoolExpression(string expr) {
    size_t opIdx = string::npos;
    for (int i = 0; i < 5; i++) {
        opIdx = expr.find(operators[i]);
        if (opIdx != string::npos) {
            this->operatorCode = operators[i];
            break;
        }
    }
    if (opIdx == string::npos) {
        std::cout << "Incorrect expr operator: " << expr << std::endl;
        exit(-1);
    }
    
    string leftExpr = expr.substr(0, opIdx);
    string rightExpr = expr.substr(opIdx + this->operatorCode.length(), expr.length());
    this->leftOperand = new SubExpression(leftExpr);
    this->rightOperand = new SubExpression(rightExpr);
}

Z3_ast BoolExpression::getAst(Z3_context context, map<string, string> *mapping) {
    
    Z3_ast left = this->leftOperand->getAst(context, mapping);
    Z3_ast right = this->rightOperand->getAst(context, mapping);

    if(this->operatorCode == "=") {
        return Z3_mk_eq(context, left, right);
    }
    
    if(this->operatorCode == "<") {
        return Z3_mk_lt(context, left, right);
    }
    
    if(this->operatorCode == "=<") {
        return Z3_mk_le(context, left, right);
    }
    
    if(this->operatorCode == ">") {
        return Z3_mk_gt(context, left, right);
    }
    
    if(this->operatorCode == ">=") {
        return Z3_mk_ge(context, left, right);
    }
    
    std::cerr << "Error, cannot generate AST for expression! Unknown Operator: " << this->operatorCode << std::endl;
    exit(-1);
    
}


bool BoolExpression::collectVars(set<string> *vars) {
    
    if(this->operatorCode == "=" && this->leftOperand->value == "1" && this->rightOperand->value == "1")
        return true;
    
    this->leftOperand->collectVars(vars);
    this->rightOperand->collectVars(vars);
    
    return false;
}