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
    
    Z3_config config = Z3_mk_config();
    Z3_context context = Z3_mk_context(config);
    
    std::regex regex(" -");
    
    while (getline(infile, line))
    {
        i++;
        
        line = std::regex_replace(line, regex, "0-");
        
        vector<string> tokens = split(line, ' ');
        
        // std::cout << line << std::endl;
        
        Clause *clause = new Clause();
        
        Head *head = new Head(tokens[0], clause);
                
        clause->head = head;
                
        for(int i = 2; i < tokens.size(); i++) {
            string item = tokens[i];
            if(item.c_str()[0] == 'h') {
                clause->formulas->push_back(new Head(tokens[i], clause));
            } else {
                BoolExpression *expression = new BoolExpression(tokens[i]);
                clause->expressions->push_back(expression);
                // std::cout << "AST: " << Z3_ast_to_string(context, expression->getAst(context)) << std::endl;
            }
        }
        
        unordered_map<int, list<Clause*>*>::const_iterator iterator = clauses.find(head->identifier);

        list<Clause*> *clauseList;
        
        if(iterator == clauses.end()) {
            clauseList = new list<Clause*>();
            clauses.insert(pair<int, list<Clause*>*>(head->identifier, clauseList));
        } else {
            clauseList = iterator->second;
        }
        
        clauseList->push_back(clause);
        
    }
    
    std::cout << "Parsing done, now expanding..." << std::endl;
    
    std::list<Clause *> *falseClauses = clauses.find(0)->second;
    
    for ( auto local_it = falseClauses->begin(); local_it!= falseClauses->end(); ++local_it ) {
        
        set<int> st;
        
        (*local_it)->head->fillRecursionState(clauses, st, -1);
        
        (*local_it)->formulas->front()->expandHead(context, clauses);
    }

    Z3_model model2;
    
    switch (Z3_check_and_get_model(context, &model2)) {
        case Z3_L_FALSE:
            printf("Z3_L_FALSE\n");
            break;
        case Z3_L_UNDEF:
            printf("Z3_L_UNDEF\n");
            break;
        case Z3_L_TRUE:
            printf("Z3_L_TRUE\n");
            break;
    }
    
    
    printf("Model:\n%s", Z3_model_to_string(context, model2));
    
    
    return 0;
}

