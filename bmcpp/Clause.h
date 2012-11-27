//
//  Clause.h
//  bmcpp
//
//  Created by Joao Carvalho on 11/23/12.
//  Copyright (c) 2012 IST. All rights reserved.
//

#ifndef __bmcpp__Clause__
#define __bmcpp__Clause__

#include <iostream>
#include <list>
#include "BoolExpression.h"

using namespace std;

class Head;

class BoolExpression;

class Clause {
    
public:
    Head *head;
    
    list<BoolExpression*> *expressions;
    
    list<Head*> *formulas;
    
    Clause();
    
    ~Clause();
    
};

#endif /* defined(__bmcpp__Clause__) */
