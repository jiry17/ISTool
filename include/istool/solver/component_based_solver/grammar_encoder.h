//
// Created by pro on 2021/12/9.
//

#ifndef ISTOOL_GRAMMAR_ENCODER_H
#define ISTOOL_GRAMMAR_ENCODER_H

#include "istool/basic/grammar.h"
#include "istool/ext/z3/z3_extension.h"
#include <map>

class Z3GrammarEncoder {
public:
    Grammar* base;
    z3::context* ctx;
    Z3GrammarEncoder(Grammar* _base, z3::context* _ctx);
    virtual void enlarge() = 0;
    virtual z3::expr_vector encodeStructure(const std::string& prefix = "") = 0;
    virtual std::pair<z3::expr, z3::expr_vector> encodeExample(const z3::expr_vector& inp_list, const std::string& prefix = "") const = 0;
    virtual PProgram programBuilder(const z3::model& model) const = 0;
    virtual ~Z3GrammarEncoder() = default;
};

class Z3GrammarEncoderBuilder {
public:
    virtual Z3GrammarEncoder* buildEncoder(Grammar* grammar, z3::context* ctx) const = 0;
    virtual ~Z3GrammarEncoderBuilder() = default;
};

#endif //ISTOOL_GRAMMAR_ENCODER_H
