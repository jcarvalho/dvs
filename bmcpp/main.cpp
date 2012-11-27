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
#include "split.h"
#include "Head.h"
#include "Clause.h"
#include "BoolExpression.h"
#include <unordered_map>
#include <regex>

using namespace std;

int main(int argc, const char * argv[])
{
    
    if(argc < 2) {
        std::cerr << "Usage bmc <filename>";
        exit(-1);
    }
    
    const char *path = argv[1];

    ifstream infile(path, ifstream::in);
    
    string line;
    
    unordered_map<int, list<Clause*>*> clauses;
    
    int i = 0;
    
    std::regex regex(" -");
    
    while (getline(infile, line))
    {
        i++;
        
        line = std::regex_replace(line, regex, "0-");
        
        vector<string> tokens = split(line, ' ');
        
        std::cout << line << std::endl;
        
        Head *head = new Head(tokens[0]);
        
        Clause *clause = new Clause();
        
        clause->head = head;
        
        for(int i = 2; i < tokens.size(); i++) {
            string item = tokens[i];
            if(item.c_str()[0] == 'h') {
                clause->formulas->push_back(new Head(tokens[i]));
            } else {
                clause->expressions->push_back(new BoolExpression(tokens[i]));
            }
        }
        
        unordered_map<int, list<Clause*>*>::const_iterator iterator = clauses.find(head->identifier);

        list<Clause*> *clauseList;
        
        if(iterator == clauses.end()) {
            clauseList = new list<Clause*>();
            clauses.emplace(head->identifier, clauseList);
        } else {
            clauseList = iterator->second;
        }
        
        clauseList->push_back(clause);
        
    }
    
    for ( auto local_it = clauses.begin(); local_it!= clauses.end(); ++local_it )
        std::cout << " " << local_it->first << ":" << local_it->second;
    
    return 0;
}

