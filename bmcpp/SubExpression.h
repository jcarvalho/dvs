//
//  SubExpression.h
//  bmcpp
//
//  Created by Joao Carvalho on 11/23/12.
//  Copyright (c) 2012 IST. All rights reserved.
//

#ifndef __bmcpp__SubExpression__
#define __bmcpp__SubExpression__

#include <iostream>
#include <map>
#include <z3.h>

using namespace std;

class SubExpression {
    
public:
    char operatorCode;
    
    SubExpression* leftExpr;
    SubExpression* rightExpr;
    
    // either the above are null or this value is null
    // inheritance would fit nicely here
    string value;
    
    SubExpression(string expr);
    
    Z3_ast getAst(Z3_context context, map<string, string> mapping, int k);
    
};

#endif /* defined(__bmcpp__SubExpression__) */
