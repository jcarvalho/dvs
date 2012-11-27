//
//  Head.h
//  bmcpp
//
//  Created by Joao Carvalho on 11/23/12.
//  Copyright (c) 2012 IST. All rights reserved.
//

#ifndef __bmcpp__Head__
#define __bmcpp__Head__

#include <iostream>
#include <list>
#include <unordered_map>
#include <map>
#include <vector>
#include "Clause.h"
#include "BoolExpression.h"

using namespace std;

class Head {
 
public:
    int identifier;
    
    vector<string> vars;
    
    Head(string headStr);
    
    ~Head();
    
    string toString();

    void expandHead(Z3_context context, unordered_map<int, list<Clause*>*> &clauses);
    
    void expandHead(Z3_context context, unordered_map<int, list<Clause*>*> &clauses, map<string, string> mapping);
    
};

#endif /* defined(__bmcpp__Head__) */
