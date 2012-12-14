//
//  MappingEntry.h
//  bmcpp
//
//  Created by Joao Carvalho on 12/14/12.
//  Copyright (c) 2012 IST. All rights reserved.
//

#ifndef __bmcpp__MappingEntry__
#define __bmcpp__MappingEntry__

#include <iostream>
#include <list>

using namespace std;

class MappingEntry {
    
public:
    
    string name;
    
    list<int> ks;
    
    MappingEntry(string);
    
    string getZ3Name();
    
};

#endif /* defined(__bmcpp__MappingEntry__) */
