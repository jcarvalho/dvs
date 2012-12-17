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

/*
 * Called from main
 */
void Head::expandHead(Z3_context context, unordered_map<int, list<Clause*>*> *clauses) {
    
    map<string, int> mapping;
    
    map<int, pair<Clause*, int>> callStack;
    
    for(string variable : this->vars) {
        mapping.insert(pair<string, int>(variable, 0));
    }
    
    return expandHead(context, clauses, &mapping, &callStack);
}

/*
 * Called from recursive expandHead
 */
void Head::expandClause(Clause *clause, Z3_context context, unordered_map<int, list<Clause*>*> *clauses, map<string, int> *mapping,
                        map<int, pair<Clause*, int>> *callStack) {
    
    std::cout << "Expanding h" << this->identifier << std::endl;
    // debugMapping(mapping);
    
    map<string, string> *newVariables = new map<string, string>();
    
    if(clause->formulas->size() > 0) {
        
        unsigned long bound = min(clause->formulas->front()->vars.size(), clause->head->vars.size());
        
        for(int i = 0; i < bound; i++) {
            newVariables->insert(pair<string, string>(clause->formulas->front()->vars[i],
                                                     clause->head->vars[i]));
        }
        
    }
    
    debugMapping(mapping);
    
    for(BoolExpression* expression : *(clause->expressions)) {
        assertIt(context, expression->getAst(context, mapping, newVariables));
    }
    
    delete newVariables;
    
    if(clause->formulas->size() == 0) {
        
        std::cout << "Calling Z3..." << std::endl;
        
        Z3_model model2;
        
        if(Z3_check_and_get_model(context, &model2) == Z3_L_TRUE) {
            printf("Program is not correct! Model:\n%s", Z3_model_to_string(context, model2));
            exit(1);
        }
        
    }
    
    for(Head* head : *(clause->formulas)) {
        head->expandHead(context, clauses, mapping, callStack);
    }
}

/*
 * Called by expandClause
 */
void Head::expandHead(Z3_context context, unordered_map<int, list<Clause*>*> *clauses, map<string, int> *mapping,
                      map<int, pair<Clause*, int>> *callStack) {
    
    list<Clause*>* clauseList = clauses->find(this->identifier)->second;
    
    if(clauseList->size() != 1) {
        
        auto it = callStack->find(this->identifier);
        
        Clause* cycleClause = NULL;
        
        for(Clause *clause : *clauseList) {
            if(clause->recursionState == CYCLE) {
                cycleClause = clause;
                break;
            }
        }
        
        // First time we hit the branch
        if(it == callStack->end()) {
            if(cycleClause != NULL) {
                
                // We want to expand this cycle K_MAX times
                callStack->insert(pair<int, pair<Clause*, int>>(this->identifier, pair<Clause*,int>(cycleClause, K_MAX)));
            }
            
        }
        
        if(cycleClause != NULL) {
            
            auto ks = (*callStack)[this->identifier];
            
            std::cout << "CallStack: " << ks.second << ". Cycle clause: " << cycleClause << std::endl;
            
            Clause* toExpand = NULL;
            
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
            
            if(toExpand == NULL) {
                std::cerr << "Solar flare detected!" << std::endl;
                exit(-2);
            }
            
            std::cout << "Exploring LOOP branch of clause " << this->identifier << std::endl;
            
            if(toExpand->recursionState == CYCLE && ks.second > 0)
                (*callStack)[this->identifier] = pair<Clause*, int>(toExpand, ks.second - 1);
            
            expandClause(toExpand, context, clauses, mapping, callStack);
            
            
            if(toExpand->recursionState == CYCLE && ks.second > 0)
                (*callStack)[this->identifier] = pair<Clause*, int>(toExpand, ks.second);
            
        } else {
            
            for(Clause *clause : *clauseList) {
                Z3_push(context);
                
                map<string, int> *newMapping = new map<string, int>(*mapping);
                
                std::cout << "Exploring IF branch of clause " << this->identifier << std::endl;
                expandClause(clause, context, clauses, newMapping, callStack);
                
                delete newMapping;
                
                Z3_pop(context, 1);
            }
        }
        
    } else {
        
        Clause *clause = clauseList->front();
        
        expandClause(clause, context, clauses, mapping, callStack);
        
    }
    
}

void Head::fillRecursionState(unordered_map<int, list<Clause*>*> &clauses, set<int> &calls) {
    
    std::cout << "Begin: " << this->identifier << std::endl;
    
    if(this->clause->recursionState != UNKNOWN) {
        std::cout << "State known: " << (this->identifier) << " : " << this->clause << std::endl;
        return;
    }
    
    bool setContains = calls.find(this->identifier) != calls.end();
    bool isToInsert = false;
    
    if(setContains) {
        std::cout << "Setting " << this->identifier << " as CYCLE " << this->clause << std::endl;
        this->clause->recursionState = CYCLE;
        std::cout << "Erasing: " << this->identifier << " " << this->clause << std::endl;
        calls.erase(this->identifier);
        return;
    } else if(this->identifier != 0 && !setContains &&
              clauses.find(this->identifier)->second->size() > 1) {
        isToInsert = true;
    }
    
    for(Head *h : *(this->clause->formulas)) {
        
        list<Clause*>* clauseList = clauses.find(h->identifier)->second;
        
        for(Clause *clause : *clauseList) {
            
            if(isToInsert) {
                std::cout << "Inserting: " << this->identifier << " " << this->clause << std::endl;
                calls.insert(this->identifier);
            }
            
            clause->head->fillRecursionState(clauses, calls);
            
            if(isToInsert) {
                std::cout << "Erasing: " << this->identifier << " " << this->clause << std::endl;
                calls.erase(this->identifier);
            }
            
            if(clause->recursionState == UNKNOWN) {
                std::cout << "Setting " << clause->head->identifier << " as NOT_CYCLE " << clause << std::endl;
                clause->recursionState = NOT_CYCLE;
            }
        }
    }
    
    
}
