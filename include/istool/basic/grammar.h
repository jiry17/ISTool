//
// Created by pro on 2021/12/4.
//

#ifndef ISTOOL_GRAMMAR_H
#define ISTOOL_GRAMMAR_H

#include "program.h"

class Rule;

class NonTerminal {
public:
    PType type;
    std::string name;
    std::vector<Rule*> rule_list;
    int id = 0;
    NonTerminal(const std::string& _name, const PType& _type);
    ~NonTerminal();
};

typedef std::vector<NonTerminal*> NTList;

class Rule {
public:
    PSemantics semantics;
    NTList param_list;
    Rule(PSemantics&& _semantics, NTList&& _param_list);
    virtual PProgram buildProgram(const ProgramList& sub_list);
    virtual std::string toString() const;
    ~Rule() = default;
};

class Grammar {
public:
    NonTerminal* start;
    NTList symbol_list;
    Grammar(NonTerminal* _start, const NTList& _symbol_list);
    void removeUseless();
    void indexSymbol();
    ~Grammar();
};

typedef std::shared_ptr<Grammar> PGrammar;


#endif //ISTOOL_GRAMMAR_H
