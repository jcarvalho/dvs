//
//  Head.cpp
//  bmcpp
//
//  Created by Joao Carvalho on 11/23/12.
//  Copyright (c) 2012 IST. All rights reserved.
//

#include "Head.h"
#include "helper.h"
#include <sstream>

Head::Head(string headStr) {
    
    size_t paren = headStr.find("(");
    
    if(paren == string::npos) {
        this->identifier = 0;
        return;
    }
    
    string ident = headStr.substr(1, paren-1);
    this->identifier = atoi(ident.c_str());

    size_t rparen = headStr.find(")");
    
    string varsStr = headStr.substr(paren + 1, rparen - paren - 1);
    
    vector<string> variables = split(varsStr, ',');
    
    for(string variable : variables) {
        vars.push_back(variable);
    }
    
}

Head::~Head() {
}

string Head::toString() {
    stringstream ss;
    ss << identifier;
    return ss.str();
}

void Head::expandHead(Z3_context context, unordered_map<int, list<Clause*>*> &clauses) {
    
    map<string,string> mapping;
    
    for(string variable : this->vars) {
        mapping.insert(pair<string, string>(variable, variable));
    }
    
    return expandHead(context, clauses, mapping);
}

void Head::expandClause(Clause *clause, Z3_context context, unordered_map<int, list<Clause*>*> &clauses, map<string, string> mapping) {
    
    for(int i = 0; i < this->vars.size(); i++) {
        mapping.insert(pair<string, string>(clause->head->vars[i], this->vars[i]));
    }
    
    debugMapping(mapping);
    
    for(BoolExpression* expression : *(clause->expressions)) {
        assertIt(context, expression->getAst(context, mapping));
    }
    
    for(Head* head : *(clause->formulas)) {
        head->expandHead(context, clauses, mapping);
    }
}

void Head::expandHead(Z3_context context, unordered_map<int, list<Clause*>*> &clauses, map<string, string> mapping) {
    
    std::cout << "Expanding h" << this->identifier << std::endl;
    
    debugMapping(mapping);
    
    list<Clause*>* clauseList = clauses.find(this->identifier)->second;
    
    if(clauseList->size() != 1) {        
        for(Clause *clause : *clauseList) {
            Z3_push(context);
            
            std::cout << "Exploring branch of clause " << clause->head->identifier << std::endl;
            
            expandClause(clause, context, clauses, mapping);
            
            Z3_pop(context, 1);
        }
    }
    
    Clause *clause = clauseList->front();
    
    expandClause(clause, context, clauses, mapping);
    
}