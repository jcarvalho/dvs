//
//  Clause.cpp
//  bmcpp
//
//  Created by Joao Carvalho on 11/23/12.
//  Copyright (c) 2012 IST. All rights reserved.
//

#include "Clause.h"

Clause::Clause() {
    
    formulas = new list<Head*>();
    expressions = new list<BoolExpression*>();
    
}

Clause::~Clause() {
    delete formulas;
    delete expressions;
}