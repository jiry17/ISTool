//
// Created by pro on 2021/12/10.
//

#ifndef ISTOOL_LINEAR_ENCODER_H
#define ISTOOL_LINEAR_ENCODER_H

#include "grammar_encoder.h"

class LineEncoder: public Z3GrammarEncoder {
public:
    struct Component {
        NonTerminal* nt;
        Rule* rule;
        z3::expr oup;
        std::vector<z3::expr> inp_list;
    };
private:
    std::vector<Component> component_list;

    Component buildComponent(NonTerminal* nt, Rule* r, const std::string& prefix);
    PProgram programBuilder(int id, const z3::model& model) const;

    int factor = 1;
    std::map<std::string, int> special_usage;
public:
    LineEncoder(Grammar* _grammar, z3::context* _ctx, int _factor, const std::map<std::string, int>& _special_usage);
    virtual void enlarge();
    virtual z3::expr_vector encodeStructure(const std::string& prefix);
    virtual std::pair<z3::expr, z3::expr_vector> encodeExample(const z3::expr_vector &inp_list, const std::string& prefix) const;
    virtual PProgram programBuilder(const z3::model& model) const;
    virtual ~LineEncoder() = default;
};

class LineEncoderBuilder: public Z3GrammarEncoderBuilder {
public:
    int factor;
    std::map<std::string, int> special_usage;
    LineEncoderBuilder(int _factor = 1, const std::map<std::string, int>& _special_usage = {});
    virtual Z3GrammarEncoder* buildEncoder(Grammar* grammar, z3::context* ctx) const;
    virtual ~LineEncoderBuilder() = default;
};


#endif //ISTOOL_LINEAR_ENCODER_H
