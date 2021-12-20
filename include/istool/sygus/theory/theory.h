//
// Created by pro on 2021/12/7.
//

#ifndef ISTOOL_THEORY_H
#define ISTOOL_THEORY_H

#include "istool/basic/specification.h"
#include "istool/basic/env.h"

enum class TheoryToken {
    CLIA, BV, STRING
};

typedef std::function<void(Env*, TheoryToken)> TheoryLoader;

class SyGuSExtension: public Extension {
public:
    TheoryToken theory;
    SyGuSExtension(TheoryToken _theory);
    ~SyGuSExtension() = default;
};

namespace ext {
    namespace sygus {
        SyGuSExtension* getSyGuSExtension(Env* env);
        SyGuSExtension* initSyGuSExtension(Env* env, TheoryToken theory);
        void loadSyGuSTheories(Env* env, const TheoryLoader& loader);
    }
}

#endif //ISTOOL_THEORY_H
