//
//  split.cpp
//  bmcpp
//
//  Created by Joao Carvalho on 11/23/12.
//  Copyright (c) 2012 IST. All rights reserved.
//

#include "helper.h"
#include <sstream>

using namespace std;

static int varGenerator = 0;

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    return split(s, delim, elems);
}


Z3_ast mk_int(Z3_context ctx, int v)
{
    Z3_sort ty = Z3_mk_int_sort(ctx);
    return Z3_mk_int(ctx, v, ty);
}

Z3_ast mk_str_var(Z3_context ctx, const char *name) {
    Z3_symbol symbol = Z3_mk_string_symbol(ctx, name);
    return Z3_mk_const(ctx, symbol, Z3_mk_int_sort(ctx));
}

void assertIt(Z3_context context, Z3_ast ast) {
#ifdef NDEBUG
    std::cout << "\t\tAsserting " << Z3_ast_to_string(context, ast) << std::endl;
#endif
    Z3_assert_cnstr(context, ast);
}

void debugMapping(std::map<string, string> *mapping) {
    
    std::stringstream ss(stringstream::out);
    
    ss << "{";
    
    for(auto it = mapping->begin(); it != mapping->end(); it++) {
        ss << " " << it->first << "=" << it->second;
    }
    
    ss << "}";

    std::cout << ss.str() << std::endl;
    
}

string genNewVar(string inputVar) {
    varGenerator++;
    std::stringstream ss;
    
    ss << inputVar << varGenerator;

    return ss.str();
}
