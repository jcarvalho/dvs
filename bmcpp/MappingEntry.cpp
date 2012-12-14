//
//  MappingEntry.cpp
//  bmcpp
//
//  Created by Joao Carvalho on 12/14/12.
//  Copyright (c) 2012 IST. All rights reserved.
//

#include "MappingEntry.h"
#include <sstream>

MappingEntry::MappingEntry(string ident) {
    name = ident;
}

string MappingEntry::getZ3Name() {
    stringstream ss;
    
    ss << name;
    
    for(int k : ks) {
        ss << k;
    }
    
    return ss.str();
}
