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
#include <algorithm>

class Checkpoint {
public:
    bool isCycle;
    bool isPending;
    int identifier;
    int k;
    Clause *clause;
    Head* head;
    map<string, string> *mapping;
};

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
 * Called from recursive expandHead
 */
void expandClause(Clause *clause, Z3_context context, unordered_map<int, list<Clause*>*> *clauses, map<string, string> *mapping,
                  map<int, pair<Clause*, int>> *callStack, int nPendings, vector<string> calleeVars) {
    
    std::cout << "Expanding h" << clause->head->identifier << std::endl;
    
    // Update mapping
    
    for(string varToCreate : (*(clause->unboundVars))) {
        (*mapping)[varToCreate] = genNewVar(varToCreate);
    }
    
    debugMapping(mapping);
    
    std::cout << "{--";
    for(string str : calleeVars) {
        std::cout << str << " ";
    }
    std::cout << "--}" << std::endl;
    
    for(int i = 0; i < calleeVars.size(); i++) {
        if(clause->head->vars[i] != calleeVars[i]) {
            (*mapping)[clause->head->vars[i]] = (*mapping)[calleeVars[i]];
        }
    }
    
    debugMapping(mapping);
    
    for(BoolExpression* expression : *(clause->expressions)) {
        assertIt(context, expression->getAst(context, mapping));
    }
    
    // Check if we're not in the and case
    
    if(clause->endClause && nPendings == 0) {

        std::cout << "Calling Z3..." << std::endl;
        
        Z3_model model2;
        
        if(Z3_check_and_get_model(context, &model2) == Z3_L_TRUE) {
            printf("Program is not correct! Model:\n%s", Z3_model_to_string(context, model2));
            exit(-1);
        }
        
    } 
}

Checkpoint* popCheckpoint(list<Checkpoint*> *heads, map<int, pair<Clause *, int>> *callStack) {
    
    Checkpoint *cp = NULL;
        
    while(heads->size() != 0) {
        cp = heads->front();
        heads->pop_front();
        if(!cp->isCycle || cp->isPending) {
            return cp;
        }
                
        (*callStack)[cp->identifier] = pair<Clause*, int>(cp->clause, cp->k);
        delete cp;
    }
    
    return NULL;
}

/*
 * Called by init
 */
void expandHeads(Head* head, Z3_context context, unordered_map<int, list<Clause*>*> *clauses, map<string, string> *mapping,
                 map<int, pair<Clause*, int>> *callStack) {
    
    int pendings = 0;
    
    list<Checkpoint*> *checkpoints = new list<Checkpoint*>();
    
    // Right head
    Head *nextHead = head;
    
    while(1) {
        list<Clause*>* clauseList = clauses->find(nextHead->identifier)->second;
        
        if(clauseList->size() != 1) {
            // Do stuff
            
            auto it = callStack->find(nextHead->identifier);
            
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
                    callStack->insert(pair<int, pair<Clause*, int>>(nextHead->identifier, pair<Clause*,int>(cycleClause, K_MAX)));
                }
                
            }
            
            if(cycleClause != NULL) {
                
                auto ks = (*callStack)[nextHead->identifier];
                
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
                
                std::cout << "Exploring LOOP branch of clause " << nextHead->identifier << std::endl;
                
                if(toExpand->recursionState == CYCLE && ks.second > 0)
                    (*callStack)[nextHead->identifier] = pair<Clause*, int>(toExpand, ks.second - 1);
                
                expandClause(toExpand, context, clauses, mapping, callStack, pendings, nextHead->vars);
                
                if(toExpand->endClause) {
                    Checkpoint *cp = popCheckpoint(checkpoints, callStack);
                    if(cp == NULL)
                        break;
                    
                    delete mapping;
                    Z3_pop(context, 1);
                    
                    if(!cp->isPending)
                        mapping = cp->mapping;
                    else
                        pendings--;
                    
                    nextHead = cp->head;
                    
                    continue;
                }
                
                // Create a Cycle Checkpoint
                
                if(toExpand->recursionState == CYCLE && ks.second > 0) {
                    Checkpoint *cp = new Checkpoint();
                    cp->isCycle = true;
                    cp->identifier = nextHead->identifier;
                    cp->clause = toExpand;
                    cp->k = ks.second;
                    
                    checkpoints->push_front(cp);
                }
                
                // Expand Last right head, and add the rest to the queue
                // Note that it SHOULD be the "function" call head
                for(Head* head : (*(toExpand->formulas))) {
                    Checkpoint *cp = new Checkpoint();
                    cp->isPending = true;
                    cp->head = head;
                    checkpoints->push_front(cp);
                    pendings++;
                }
                
                Checkpoint *cp = checkpoints->front();
                
                nextHead = cp->head;
                checkpoints->pop_front();
                pendings--;
                
                delete cp;
                
            } else {
                
                // NOTE: Assume two clauses only
                
                auto it = clauseList->begin();
                
                Clause *clause = *it;
		std:cout << " branch clause " << clause->head->identifier << " is end? " << clause->endClause << " pendings: " << pendings << std::endl;

                while(it != clauseList->end() && clause->endClause && pendings != 0) {
                    clause = *(++it);
		    cout << " branch clause " << clause->head->identifier << " is end? " << clause->endClause << " pendings: " << pendings << std::endl;
                }
                
                Z3_push(context);
                
                map<string, string> *newMapping = new map<string, string>(*mapping);
                
                Checkpoint *cp;
                
                if(it != clauseList->end() && (!clause->endClause || (clause->endClause && pendings == 0))) {
		    cout << "expanding one of the previous branch clauses" << endl;
                    expandClause(clause, context, clauses, newMapping, callStack, pendings, nextHead->vars);
                    // Expand Last right head, and add the rest to the queue
                    // Note that it SHOULD be the "function" call head
                    for(Head* head : (*(clause->formulas))) {
                        Checkpoint *cp = new Checkpoint();
                        cp->isPending = true;
                        cp->head = head;
                        checkpoints->push_front(cp);
                        pendings++;
                    }
                    
                    cp = checkpoints->front();
                    
                    nextHead = cp->head;
                    checkpoints->pop_front();
                    pendings--;
                    
                    delete cp;
                } 

		if(it == clauseList->end() || (clause->endClause && pendings != 0)) {
                    Checkpoint *cp = popCheckpoint(checkpoints, callStack);
                    if(cp == NULL) {
                        break;
                    }
                    
                    if(!cp->isPending)
                        mapping = cp->mapping;
                    else
                        pendings--;
                    
                    nextHead = cp->head;
                    
                    continue;
                } else {
		    cout << "adding the other branches not explored as checkpoints " << endl;
		    ++it;
                    while(it != clauseList->end()) {
                        cp = new Checkpoint();
                        
                        cp->isCycle = false;
                        cp->mapping = newMapping;
                        
                        // TODO: There could be more than one!
                        cp->head = (*(it))->formulas->front();
                        
                        checkpoints->push_front(cp);
			it++;
                    }
                }
            }
            
        } else {
            
            Clause *nextClause = clauseList->front();
            
            expandClause(nextClause, context, clauses, mapping, callStack, pendings, nextHead->vars);
            
            if(nextClause->endClause) {
                Checkpoint *cp = popCheckpoint(checkpoints, callStack);
                if(cp == NULL) {
                    std::cout << "This false is taken care of." << std::endl;
                    break;
                }
                
                if(!cp->isPending)
                    mapping = cp->mapping;
                else
                    pendings--;
                
                nextHead = cp->head;
                
                continue;
            } else {
                
                // Expand Last right head, and add the rest to the queue
                // Note that it SHOULD be the "function" call head
                for(Head* head : (*(nextClause->formulas))) {
                    Checkpoint *cp = new Checkpoint();
                    cp->isPending = true;
                    cp->head = head;
                    checkpoints->push_front(cp);
                    pendings++;
                }
                
                Checkpoint *cp = checkpoints->front();
                
                nextHead = cp->head;
                checkpoints->pop_front();
                pendings--;
                
                delete cp;
            }
        }
    }
    
}

/*
 * Called from main
 */
void Head::expandHead(Z3_context context, unordered_map<int, list<Clause*>*> *clauses) {
    
    map<string, string> *mapping = new map<string, string>();
    
    map<int, pair<Clause*, int>> *callStack = new map<int, pair<Clause*, int>>();
    
    for(string variable : this->vars) {
        mapping->insert(pair<string, string>(variable, variable));
    }
    
    expandHeads(this, context, clauses, mapping, callStack);
    
    delete callStack;
    delete mapping;
}

void Head::fillRecursionState(unordered_map<int, list<Clause*>*> *clauses, set<int> &calls) {
    
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
              clauses->find(this->identifier)->second->size() > 1) {
        isToInsert = true;
    }
    
    for(Head *h : *(this->clause->formulas)) {
        
        list<Clause*>* clauseList = clauses->find(h->identifier)->second;
        
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

void Head::collectVars(set<string> *vars) {
    for(auto it = this->vars.begin(); it != this->vars.end(); it++) {
        vars->insert((*it));
    }
}

void Head::fillUnboundVars() {
    
    if(this->identifier == 0)
        return;
    
    this->clause->unboundVars = new set<string>();
    
    set<string> *allVars = new set<string>();
    
    for(Head* formula : (*(clause->formulas))) {
        formula->collectVars(allVars);
    }
    
    for(BoolExpression* expression : (*(clause->expressions))) {
        if(expression->collectVars(allVars)) {
            clause->endClause = true;
            return;
        }
    }
    
    for(auto it = allVars->begin(); it != allVars->end(); it++) {
        if(std::find(vars.begin(), vars.end(), (*it)) == vars.end()) {
            std::cout << "In clause " << this->identifier << ", " << (*it) << " is unbound!" << std::endl;
            this->clause->unboundVars->insert((*it));
        }
    }
}
