//
//  SubExpression.cpp
//  bmcpp
//
//  Created by Joao Carvalho on 11/23/12.
//  Copyright (c) 2012 IST. All rights reserved.
//

#include "SubExpression.h"

#include <algorithm>
#include <map>
#include "helper.h"

static map<int, Z3_ast> intMap;

size_t getLastOp(size_t op1, size_t op2) {
    if (op1 == string::npos) {
        return op2;
    } else if (op2 == string::npos) {
        return op1;
    } else {
        return op1 > op2 ? op1 : op2;
    }
}

SubExpression::SubExpression(string expr) {
    expr.erase (remove(expr.begin(), expr.end(), '.'), expr.end());
    expr.erase (remove(expr.begin(), expr.end(), ','), expr.end());
    
    size_t lastOp = getLastOp(expr.find("+"), expr.find("-"));
    if (lastOp == string::npos) {
        lastOp = getLastOp(expr.find("*"), expr.find("/"));
    }
    if (lastOp == string::npos) {
        // we reached a basic value --- this expr has no operators
        
        // HACK! THE GRAMMAR MUST BE PROPERLY DEFINED!
        
        expr.erase (remove(expr.begin(), expr.end(), '('), expr.end());
        expr.erase (remove(expr.begin(), expr.end(), ')'), expr.end());
        this->value = expr;
    } else {
        // there is at least one operator to process
        this->operatorCode = expr.at(lastOp);
        string leftExpr = expr.substr(0, lastOp);
        string rightExpr = expr.substr(lastOp + 1, expr.length());
        this->leftExpr = new SubExpression(leftExpr);
        this->rightExpr = new SubExpression(rightExpr);
    }
}

Z3_ast SubExpression::getAst(Z3_context context, map<string, string> mapping) {
    
    if(this->value != "") {
        
        char firstChar = this->value.at(0);
        
        if(firstChar >= '0' && firstChar <= '9') {
            int value = atoi(this->value.c_str());
            // Remember to check the map!
            return mk_int(context, value);
        } else {
            string actualValue = mapping.count(this->value) == 0 ? this->value : mapping[this->value];
            return mk_str_var(context, actualValue.c_str());
        }
        
    }
    
    Z3_ast operatorArgs[2];
    operatorArgs[0] = this->leftExpr->getAst(context, mapping);
    operatorArgs[1] = this->rightExpr->getAst(context, mapping);
    
    switch(this->operatorCode) {
        case '+':
            return Z3_mk_add(context, 2, operatorArgs);
        case '-':
            return Z3_mk_sub(context, 2, operatorArgs);
        case '*':
            return Z3_mk_mul(context, 2, operatorArgs);
        case '/':
            return Z3_mk_div(context, operatorArgs[0], operatorArgs[1]);
        default:
            std::cerr << "Error, cannot generate AST for expression! Unknown Operator: " << this->operatorCode << std::endl;
            exit(-1);
    }
}

