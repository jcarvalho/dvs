//
//  main.cpp
//  bmcpp
//
//  Created by Joao Carvalho on 11/23/12.
//  Copyright (c) 2012 IST. All rights reserved.
//

#include <iostream>
#include <z3.h>
#include <fstream>
#include "helper.h"
#include "Head.h"
#include "Clause.h"
#include "BoolExpression.h"
#include <unordered_map>

using namespace std;

Clause* getCycleClause(list<Clause *> *clauses, RecursionState state) {
    
    for(Clause *clause : (*clauses)) {
        if(clause->recursionState == state) {
            return clause;
        }
    }
    return NULL;
}

void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

int main(int argc, const char * argv[])
{
    
    if(argc < 3) {
        std::cerr << "Usage bmc <filename> <k>" << std::endl;
        exit(-1);
    }
    
    const char *path = argv[1];
    
    ifstream infile(path, ifstream::in);
    
    string line;
    
    unordered_map<int, list<Clause*>*> *clauses = new unordered_map<int, list<Clause *>*>();
    
    int i = 0;
    
    Z3_config config = Z3_mk_config();
    Z3_context context = Z3_mk_context(config);
    
    while (getline(infile, line))
    {
        i++;
	    replaceAll(line, " -", "0-");
        
        vector<string> tokens = split(line, ' ');
        
        // std::cout << line << std::endl;
        
        Clause *clause = new Clause();
        
        Head *head = new Head(tokens[0], clause);
        
        clause->head = head;
         
        // Not all formulas can be split with ' '. Sometimes they are next to each other
        // Ex: SG3 h2(A,B,C,D,A,B,C,D) :- B=3,A=5, h1(E,F).
        for(int i = 2; i < tokens.size(); i++) {
            string item = tokens[i];
            if(item.c_str()[0] == 'h') {
                clause->formulas->push_back(new Head(item, clause));
            } else {
                vector<string> subtokens = split(item, ',');
                for (int k = 0; k < subtokens.size(); k++) {
                    BoolExpression *expression = new BoolExpression(subtokens[k]);
                    clause->expressions->push_back(expression);
                }
            }
        }
        
        unordered_map<int, list<Clause*>*>::const_iterator iterator = clauses->find(head->identifier);
        
        list<Clause*> *clauseList;
        
        if(iterator == clauses->end()) {
            clauseList = new list<Clause*>();
            clauses->insert(pair<int, list<Clause*>*>(head->identifier, clauseList));
        } else {
            clauseList = iterator->second;
        }
        
        head->fillUnboundVars();
        
        clauseList->push_back(clause);
        
    }
    
    std::cout << "Parsing done, now expanding..." << std::endl;
    
    std::list<Clause *> *falseClauses = clauses->find(0)->second;
    
    for ( auto local_it = falseClauses->begin(); local_it!= falseClauses->end(); ++local_it ) {
        
        set<int> st;
        
        (*local_it)->head->fillRecursionState(clauses, st);
        
    }
    
    for ( auto local_it = clauses->begin(); local_it != clauses->end(); local_it++) {
        
        if((*local_it).second->size() <= 1)
            continue;
        
        Clause *cycleToExpand = getCycleClause((*local_it).second, CYCLE);

        if(cycleToExpand == NULL)
            continue;
        
        Clause *iter = cycleToExpand;
        
        while(1) {
            
            Head *headToExpand = iter->formulas->front();
            
            list<Clause *> *headClauses = (*clauses)[headToExpand->identifier];
            
            if(cycleToExpand == getCycleClause(headClauses, CYCLE)) {
                std::cout << "Clause " << cycleToExpand->head->identifier << " @ " << cycleToExpand << " is actually a cycle!" << std::endl;
                std::cout << "--> " << cycleToExpand->formulas->front()->identifier << std::endl;
                break;
            }
            
            iter = getCycleClause(headClauses, NOT_CYCLE);
            
            if(iter->formulas->size() == 0) {
                std::cout << "Clause " << cycleToExpand->head->identifier << " @ " << cycleToExpand << " was not a cycle after all!" << std::endl;
                cycleToExpand->recursionState = NOT_CYCLE;
                break;
            }
        }
        
    }
    
    for ( auto local_it = falseClauses->begin(); local_it!= falseClauses->end(); ++local_it ) {
        
        (*local_it)->formulas->front()->expandHead(context, atoi(argv[2]), clauses);
    }

    for ( auto local_it = clauses->begin(); local_it != clauses->end(); local_it++) {
        list<Clause*> *list = (*local_it).second;
        for (Clause* c : (*list)) {
            delete c->head;
            for (Head* h: (*(c->formulas))) {
                delete h;
            }
            delete c;
        }
        delete list;
    }
    
    delete clauses;
    
    std::cout << "Congratulations, your program is correct!" << std::endl;
    
    return 0;
}

