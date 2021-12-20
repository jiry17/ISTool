//
// Created by pro on 2021/12/9.
//

#include "istool/solver/component_based_solver/grammar_encoder.h"

Z3GrammarEncoder::Z3GrammarEncoder(Grammar *_base, z3::context* _ctx): base(_base), ctx(_ctx) {
}