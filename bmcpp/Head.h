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

using namespace std;

class Head {
 
public:
    int identifier;
    
    list<string> *vars;
    
    Head(string headStr);
    
    ~Head();
    
    string toString();
};

#endif /* defined(__bmcpp__Head__) */
