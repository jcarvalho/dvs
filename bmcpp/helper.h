//
//  split.h
//  bmcpp
//
//  Created by Joao Carvalho on 11/23/12.
//  Copyright (c) 2012 IST. All rights reserved.
//

#ifndef __bmcpp__split__
#define __bmcpp__split__

#include <iostream>
#include <vector>
#include <z3.h>
#include <map>

using namespace std;

Z3_ast mk_int(Z3_context ctx, int v);

Z3_ast mk_str_var(Z3_context ctx, const char *name);

std::vector<std::string> split(const std::string &s, char delim);

void assertIt(Z3_context context, Z3_ast ast);

void debugMapping(map<string, string> *mapping);

string genNewVar(string inputVar);

#endif /* defined(__bmcpp__split__) */
