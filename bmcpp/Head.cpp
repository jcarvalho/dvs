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

Head::Head(string headStr, Clause *cls) {
    
    this->clause = cls;
    
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
    
    map<int, pair<Clause*, int>> callStack;
    
    for(string variable : this->vars) {
        mapping.insert(pair<string, string>(variable, variable));
    }
    
    return expandHead(context, clauses, mapping, callStack);
}

void Head::expandClause(Clause *clause, Z3_context context, unordered_map<int, list<Clause*>*> &clauses, map<string, string> mapping,
                        map<int, pair<Clause*, int>> &callStack) {
    
    for(int i = 0; i < this->vars.size(); i++) {
        mapping.insert(pair<string, string>(clause->head->vars[i], this->vars[i]));
    }
    
    debugMapping(mapping);
    
    for(BoolExpression* expression : *(clause->expressions)) {
        assertIt(context, expression->getAst(context, mapping));
    }
    
    for(Head* head : *(clause->formulas)) {
        head->expandHead(context, clauses, mapping, callStack);
    }
}

void fillRecursionState(list<Clause*>* clauseList);

void Head::expandHead(Z3_context context, unordered_map<int, list<Clause*>*> &clauses, map<string, string> mapping, map<int, pair<Clause*, int>> &callStack) {
    
    std::cout << "Expanding h" << this->identifier << std::endl;
    
    // debugMapping(mapping);
    
    list<Clause*>* clauseList = clauses.find(this->identifier)->second;
    
    if(clauseList->size() != 1) {
        
        auto it = callStack.find(this->identifier);
        
        Clause* cycleClause = NULL;
        
        for(Clause *clause : *clauseList) {
            if(clause->recursionState == CYCLE) {
                cycleClause = clause;
                break;
            }
        }
        
        // First time we hit the branch
        if(it == callStack.end()) {
            if(cycleClause != NULL) {
                
                // We want to expand this cycle K_MAX times
                callStack.insert(pair<int, pair<Clause*, int>>(this->identifier, pair<Clause*,int>(cycleClause, K_MAX)));
            }
            
            // fillRecursionState(clauseList);
        }
        
        auto ks = callStack[this->identifier];
        
        Clause* toExpand;
        
        if(ks.second == 0) {
            
            for(Clause *clause : *clauseList) {
                if(clause != ks.first) {
                    toExpand = clause;
                    break;
                }
            }
            
        } else {
            toExpand = ks.first;
        }
        
        if(cycleClause == NULL)
            Z3_push(context);
        
        std::cout << "Exploring branch of clause " << toExpand->head->identifier << std::endl;
        
        callStack.insert(pair<int, pair<Clause*, int>>(this->identifier, pair<Clause*, int>(toExpand, ks.second - 1)));
        
        expandClause(toExpand, context, clauses, mapping, callStack);
        
        if(cycleClause == NULL)
            Z3_pop(context, 1);
    }
    
    Clause *clause = clauseList->front();
    
    expandClause(clause, context, clauses, mapping, callStack);
    
}

void Head::fillRecursionState(unordered_map<int, list<Clause*>*> &clauses, set<int> &calls) {
    
    std::cout << "Begin: " << this->identifier << std::endl;
    
    if(this->clause->recursionState != UNKNOWN) {
        std::cout << "State known: " << (this->identifier) << " : " << this->clause << std::endl;
        return;
    }
    
    if(calls.find(this->identifier) != calls.end()) {
        std::cout << "Setting " << this->identifier << " as CYCLE " << this->clause << std::endl;
        this->clause->recursionState = CYCLE;
        calls.erase(this->identifier);
        return;
    } else if(clauses.find(this->identifier)->second->size() > 1) {
        calls.insert(this->identifier);
    }
    
    for(Head *h : *(this->clause->formulas)) {
        
        list<Clause*>* clauseList = clauses.find(h->identifier)->second;
        
        for(Clause *clause : *clauseList) {
            clause->head->fillRecursionState(clauses, calls);
            
            if(clause->recursionState == UNKNOWN) {
                std::cout << "Setting " << clause->head->identifier << " as NOT_CYCLE " << clause << std::endl;
                clause->recursionState = NOT_CYCLE;
            }
        }
    }
    
    calls.erase(this->identifier);
    
    
}
