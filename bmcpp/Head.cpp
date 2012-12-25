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
    bool isCycle; // if not cycle, then pending
    int identifier;
    int k;
    Clause *clause;
    Head* head;
};

class BranchCheckpoint {
public:
    int k;
    Head* head;
    map<string, string> *mapping;
    set<int> *alreadySeen;
    map<int, pair<Clause*, int>> *callStack;
    list<Checkpoint*>* checkpoints;
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
    
    debugMapping(mapping);
    
    std::cout << "{--";
    for(string str : calleeVars) {
        std::cout << str << " ";
    }
    std::cout << "--}" << std::endl;
    
    // We always need to update the new mapping from the old one, so that we do not make 
    // updates that are influenced by new updates. Look into h14 -> h13 in SG3 for an example
    map<string, string> *backupMapping = new map<string, string>(*mapping);

    // Clauses such as h2(A,B,C,D,A,B,C,D) :- B=3,A=5, h1(E,F). are problematic. Note that the 
    // list of variables in h2 has repeating identifiers. So we ignore the repeated ones. There 
    // is no guarantee this is the correct way to handle it though...
    set<string> *alreadySeen = new set<string>();
    for(int i = 0; i < calleeVars.size(); i++) {
        if (alreadySeen->find(clause->head->vars[i]) != alreadySeen->end()) {
            continue; // we reached a repeated variable
        } 
        alreadySeen->insert(clause->head->vars[i]);

        if(clause->head->vars[i] != calleeVars[i]) {
            (*mapping)[clause->head->vars[i]] = (*backupMapping)[calleeVars[i]];
        }
    }
    delete backupMapping;

    // Update mapping
    for(string varToCreate : (*(clause->unboundVars))) {
        (*mapping)[varToCreate] = genNewVar(varToCreate);
    }
    
    debugMapping(mapping);
    
    for(BoolExpression* expression : *(clause->expressions)) {
        assertIt(context, expression->getAst(context, mapping));
    }
    
    // Check if we're not in the and case
    
if (clause->endClause) {
    std::cout << "CALL Z3? " << nPendings << std::endl;
}

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
        if(!cp->isCycle) {
            return cp;
        }
                
        (*callStack)[cp->identifier] = pair<Clause*, int>(cp->clause, cp->k);
        // delete cp;
    }
    
    return NULL;
}

/*
 * Called by init
 */
void expandHeads(Head* head, Z3_context context, int K_MAX, unordered_map<int, list<Clause*>*> *clauses, map<string, string> *mapping,
                 map<int, pair<Clause*, int>> *callStack) {
    
    int pendings = 0;
    
    list<Checkpoint*> *checkpoints = new list<Checkpoint*>();

    // We need to detach branch checkpoints from the rest. The idea is that we want to 
    // explore all pending before reverting to another branch. Additionally, the branch 
    // will need to explore pendings that were discovered before the branch was discovered
    // but that were left pending. Because of that, we need to also keep in the BranchCheckpoint
    // the checkpoints that existed at the time the branch was created.
    list<BranchCheckpoint*> *branches = new list<BranchCheckpoint*>();

    // It is possible that a "right" head expands into a cycle. SG3 is a good example 
    // of that with hte h16->h3 expansion. This is one of the two expansions of h16 and 
    // thus is not caught as a Cycle (which is actually not, even though it does form 
    // one in the graph. If we find a pending head that was already expanded in the current 
    // cycle, we do not expand it.
    set<int> *alreadyEnqueuedInCurrentCheckpoint = new set<int>();
    
    list<Checkpoint*> *toCollect = new list<Checkpoint*>();

    // Right head
    Head *nextHead = head;
    
    while(1) {
        // Here at the start we deal with expansions that lead to nodes where we have already been in this branch/iteration
        // These happen due to cycles in multiple-formula expansion in the same clause
        if (alreadyEnqueuedInCurrentCheckpoint->find(nextHead->identifier) == alreadyEnqueuedInCurrentCheckpoint->end()) {
            alreadyEnqueuedInCurrentCheckpoint->insert(nextHead->identifier);
        } else {
            Checkpoint *cp = popCheckpoint(checkpoints, callStack);
            if(cp == NULL) {
                if(branches->size() > 0) {
                    // it is a branch checkpoint
                    BranchCheckpoint* bc = branches->front();
                    branches->pop_front();
                
                    std::cout << "Calling Z3...at start" << std::endl;
                    Z3_model model2;
                    if(Z3_check_and_get_model(context, &model2) == Z3_L_TRUE) {
                        printf("Program is not correct! Model:\n%s", Z3_model_to_string(context, model2));
                        exit(-1);
                    }

                    delete mapping;
                    delete alreadyEnqueuedInCurrentCheckpoint;
                    delete callStack;
                    delete checkpoints;
                    Z3_pop(context, 1);
                    mapping = bc->mapping;
                    alreadyEnqueuedInCurrentCheckpoint = bc->alreadySeen;
                    callStack = bc->callStack;
                    checkpoints = bc->checkpoints;
                    nextHead = bc->head;
                } else {
                    // we're done here
                    break;
                }
            } else {
                // it is a pending checkpoint
                pendings--;
                // delete cp;
                nextHead = cp->head;
            }
                    
                    
            continue;
        }

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
                
                if(toExpand->recursionState == CYCLE && ks.second > 0) {
                    (*callStack)[nextHead->identifier] = pair<Clause*, int>(toExpand, ks.second - 1);
                    alreadyEnqueuedInCurrentCheckpoint->clear();
                }
                
                expandClause(toExpand, context, clauses, mapping, callStack, pendings, nextHead->vars);
                
                if(toExpand->endClause) {
                    Checkpoint *cp = popCheckpoint(checkpoints, callStack);
                    if(cp == NULL) {
                        if(branches->size() > 0) {
                            // it is a branch checkpoint
                            BranchCheckpoint* bc = branches->front();
                            branches->pop_front();
                            delete mapping;
                            delete alreadyEnqueuedInCurrentCheckpoint;
                            delete callStack;
                            delete checkpoints;
                            Z3_pop(context, 1);
                            mapping = bc->mapping;
                            alreadyEnqueuedInCurrentCheckpoint = bc->alreadySeen;
                            callStack = bc->callStack;
                            checkpoints = bc->checkpoints;
                            nextHead = bc->head;
                        } else {
                            // we're done here
                            break;
                        }
                    } else {
                        // it is a pending checkpoint
                        pendings--;
                        // delete cp;
                        nextHead = cp->head;
                    }
                            
                            
                    continue;
                }
                
                // Create a Cycle Checkpoint
                
                if(toExpand->recursionState == CYCLE && ks.second > 0) {
                    Checkpoint *cp = new Checkpoint();
                    toCollect->push_front(cp);
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
                    toCollect->push_front(cp);
                    cp->isCycle = false;
                    cp->head = head;
                    checkpoints->push_front(cp);
                    pendings++;
                }
                
                Checkpoint *cp = checkpoints->front();
                
                nextHead = cp->head;
                checkpoints->pop_front();
                pendings--;
                
                // delete cp;
                
            } else {
                
                auto it = clauseList->begin();
                
                Clause *clause = *it;
		        std:cout << " branch clause " << clause->head->identifier << " is end? " << clause->endClause << " pendings: " << pendings << std::endl;

                while(it != clauseList->end() && clause->endClause && pendings != 0) {
                    clause = *(++it);
                    if (it != clauseList->end()) {
		                cout << " branch clause " << clause->head->identifier << " is end? " << clause->endClause << " pendings: " << pendings << std::endl;
                    }
                }
                
                Z3_push(context);
                
                map<string, string> *backupMapping = new map<string, string>(*mapping);
                
                Checkpoint *cp;
                
                if(it != clauseList->end() && (!clause->endClause || (clause->endClause && pendings == 0))) {
		            cout << "expanding one of the previous branch clauses" << endl;
                    expandClause(clause, context, clauses, mapping, callStack, pendings, nextHead->vars);
                    // Expand Last right head, and add the rest to the queue
                    // Note that it SHOULD be the "function" call head
                    for(Head* head : (*(clause->formulas))) {
                        Checkpoint *cp = new Checkpoint();
                        toCollect->push_front(cp);
                        cp->isCycle = false;
                        cp->head = head;
                        checkpoints->push_front(cp);
                        pendings++;
                    }
                    
                    cp = checkpoints->front();
                    
                    nextHead = cp->head;
                    checkpoints->pop_front();
                    pendings--;
                    
                    // delete cp;
                } 

		        if(it == clauseList->end() || (clause->endClause && pendings != 0)) {
                    Checkpoint *cp = popCheckpoint(checkpoints, callStack);
                    if(cp == NULL) {
                        if(branches->size() > 0) {
                            // it is a branch checkpoint
                            BranchCheckpoint* bc = branches->front();
                            branches->pop_front();
                            delete mapping;
                            delete alreadyEnqueuedInCurrentCheckpoint;
                            delete callStack;
                            delete checkpoints;
                            Z3_pop(context, 1);
                            mapping = bc->mapping;
                            alreadyEnqueuedInCurrentCheckpoint = bc->alreadySeen;
                            callStack = bc->callStack;
                            checkpoints = bc->checkpoints;
                            nextHead = bc->head;
                        } else {
                            // we're done here
                            delete backupMapping;
                            break;
                        }
                    } else {
                        // it is a pending checkpoint
                        pendings--;
                        // delete cp;
                        nextHead = cp->head;
                    }
                            
                            
                    delete backupMapping;
                    continue;
                } else {
		            ++it;
                    while(it != clauseList->end()) {
                        BranchCheckpoint* bc = new BranchCheckpoint();
                        
                        bc->mapping = new map<string, string>(*backupMapping);
                        
                        // TODO: There could be more than one!
                        // Moreover, we are skipping the "duplicate" clause expansion, at this moment we 
                        // are only expanding it once and then we checkpoint the first right head of each of the "duplicates" clauses
                        // both these are problematic in evenodd.c
                        bc->head = (*(it))->formulas->front();
                        bc->alreadySeen = alreadyEnqueuedInCurrentCheckpoint;
                        alreadyEnqueuedInCurrentCheckpoint = new set<int>(*alreadyEnqueuedInCurrentCheckpoint);                        
                        bc->callStack = new map<int, pair<Clause*, int>>(*callStack);
                        bc->checkpoints = new list<Checkpoint*>(*checkpoints);

                        branches->push_front(bc);
            			it++;

		                cout << "added the other branches of " << bc->head->identifier << " as branch checkpoint" << endl;
                    }
                    delete backupMapping;
                }
            }
            
        } else {
            
            Clause *nextClause = clauseList->front();
            
            expandClause(nextClause, context, clauses, mapping, callStack, pendings, nextHead->vars);
            
            if(nextClause->endClause) {
                Checkpoint *cp = popCheckpoint(checkpoints, callStack);
                if(cp == NULL) {
                    if(branches->size() > 0) {
                        // it is a branch checkpoint
                        BranchCheckpoint* bc = branches->front();
                        branches->pop_front();
                        delete mapping;
                        delete alreadyEnqueuedInCurrentCheckpoint;
                        delete callStack;
                        delete checkpoints;
                        Z3_pop(context, 1);
                        mapping = bc->mapping;
                        alreadyEnqueuedInCurrentCheckpoint = bc->alreadySeen;
                        callStack = bc->callStack;
                        checkpoints = bc->checkpoints;
                        nextHead = bc->head;
                    } else {
                        // we're done here
                        break;
                    }
                } else {
                    // it is a pending checkpoint
                    pendings--;
                    // delete cp;
                    nextHead = cp->head;
                }
                        
                        
                continue;
            } else {
                
                // Expand Last right head, and add the rest to the queue
                // Note that it SHOULD be the "function" call head
                for(Head* head : (*(nextClause->formulas))) {
                    Checkpoint *cp = new Checkpoint();
                    toCollect->push_front(cp);
                    cp->isCycle = false;
                    cp->head = head;
                    checkpoints->push_front(cp);
                    pendings++;
                }
                
                Checkpoint *cp = checkpoints->front();
                
                nextHead = cp->head;
                checkpoints->pop_front();
                pendings--;
                
                // delete cp;
            }
        }
    }

    delete mapping;
    delete callStack;
    delete branches;
    delete checkpoints;
    delete alreadyEnqueuedInCurrentCheckpoint;
    for (Checkpoint* c : (*toCollect)) {
        delete c;
    }
    delete toCollect;
}

/*
 * Called from main
 */
void Head::expandHead(Z3_context context, int K_MAX, unordered_map<int, list<Clause*>*> *clauses) {
    
    map<string, string> *mapping = new map<string, string>();
    
    map<int, pair<Clause*, int>> *callStack = new map<int, pair<Clause*, int>>();
    
    for(string variable : this->vars) {
        mapping->insert(pair<string, string>(variable, variable));
    }
    
    expandHeads(this, context, K_MAX, clauses, mapping, callStack);
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
