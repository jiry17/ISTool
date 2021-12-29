//
// Created by pro on 2021/12/9.
//

#include "istool/solver/component/grammar_encoder.h"

Z3GrammarEncoder::Z3GrammarEncoder(Grammar *_base, Env* _env): base(_base), ext(ext::z3::getExtension(_env)) {
}