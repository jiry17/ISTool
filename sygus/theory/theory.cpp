//
// Created by pro on 2021/12/7.
//

#include "istool/basic/env.h"
#include "istool/sygus/theory/theory.h"
#include "istool/sygus/theory/basic/clia/clia_semantics.h"
#include <map>

#include "glog/logging.h"

SyGuSExtension::SyGuSExtension(TheoryToken _theory): theory(_theory) {
}

namespace {
    const std::string KSyGuSName = "sygus";
}

#define AddTokenItem(name) case TheoryToken::name: return #name

std::string sygus::theoryToken2String(TheoryToken token) {
    switch (token) {
        AddTokenItem(CLIA);
        AddTokenItem(BV);
        AddTokenItem(STRING);
    }
}

SyGuSExtension * sygus::getSyGuSExtension(Env *env) {
    auto* ext = env->getExtension(KSyGuSName);
    if (ext) {
        auto* sygus_ext = dynamic_cast<SyGuSExtension*>(ext);
        if (!sygus_ext) {
            LOG(FATAL) << "Unmatched extension " << KSyGuSName;
        }
        return sygus_ext;
    }
    LOG(FATAL) << "Uninitialized extension " << KSyGuSName;
}

SyGuSExtension * sygus::initSyGuSExtension(Env *env, TheoryToken theory) {
    auto* ext = new SyGuSExtension(theory);
    env->registerExtension(KSyGuSName, ext);
    return ext;
}

namespace {
    const std::map<TheoryToken, std::vector<TheoryToken>> theory_dependency = {
            {TheoryToken::CLIA, {TheoryToken::CLIA}},
            {TheoryToken::BV, {TheoryToken::BV}},
            {TheoryToken::STRING, {TheoryToken::CLIA, TheoryToken::STRING}}
    };
}

void sygus::loadSyGuSTheories(Env *env, const TheoryLoader &loader) {
    auto* ext = getSyGuSExtension(env);
    return loader(env, ext->theory);
}