//
//  BoolExpression.h
//  bmcpp
//
//  Created by Joao Carvalho on 11/23/12.
//  Copyright (c) 2012 IST. All rights reserved.
//

#ifndef __bmcpp__BoolExpression__
#define __bmcpp__BoolExpression__

#include <iostream>
#include <z3.h>
#include <map>
#include "SubExpression.h"

using namespace std;

class BoolExpression {
    
public:
    string operatorCode;
    
    SubExpression* leftOperand;
    
    SubExpression* rightOperand;
    
    BoolExpression(string expr);
        
    Z3_ast getAst(Z3_context context, map<string, int> *mapping, map<string, string>);
    
};

#endif /* defined(__bmcpp__BoolExpression__) */
