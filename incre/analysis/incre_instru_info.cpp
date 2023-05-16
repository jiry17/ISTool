//
// Created by pro on 2022/9/23.
//

#include "istool/incre/analysis/incre_instru_info.h"
#include "istool/incre/grammar/incre_component_collector.h"
#include "istool/incre/trans/incre_trans.h"
#include "istool/ext/deepcoder/deepcoder_semantics.h"
#include "istool/sygus/theory/theory.h"
#include "istool/sygus/theory/basic/theory_semantics.h"
#include <iostream>
#include "glog/logging.h"

using namespace incre;

AlignTypeInfoData::AlignTypeInfoData(const Term& __term, const std::unordered_map<std::string, Ty> &type_ctx, const Ty &_oup_type, int _command_id):
    oup_type(_oup_type), _term(__term), term(dynamic_cast<TmLabeledAlign*>(__term.get())), command_id(_command_id) {
    /*auto inps = incre::getUnboundedVars(term->content.get());
    for (const auto& inp: inps) {
        auto it = type_ctx.find(inp);
        if (it != type_ctx.end()) {
            inp_types.emplace_back(inp, it->second);
        }
    }*/
    for (auto& [inp_name, inp_type]: type_ctx) {
        inp_types.emplace_back(inp_name, inp_type);
    }
}
int AlignTypeInfoData::getId() const {
    return term->id;
}
void AlignTypeInfoData::print() const {
    std::cout << "align term #" << term->id << ": " << oup_type->toString() << std::endl;
    std::cout << term->toString() << std::endl;
    for (const auto& [name, ty]: inp_types) {
        std::cout << "  " << name << ": " << ty->toString() << std::endl;
    }
}

IncreInfo::IncreInfo(const IncreProgram &_program, Context *_ctx, const AlignTypeInfoList &infos, IncreExamplePool *pool, const grammar::ComponentPool& _pool):
    program(_program), ctx(_ctx), align_infos(infos), example_pool(pool), component_pool(_pool) {
}
IncreInfo::~IncreInfo() {
    delete ctx; delete example_pool;
}

#include "istool/incre/language/incre_term.h"

void incre::prepareEnv(Env *env) {
    theory::loadBasicSemantics(env, TheoryToken::CLIA);
    incre::initBasicOperators(env);
    ext::ho::loadDeepCoderSemantics(env);
}

IncreInfo* incre::buildIncreInfo(const IncreProgram &program, Env* env) {
    //auto labeled_program = incre::eliminateUnboundedCreate(program);
    incre::checkAllLabelBounded(program.get());
    auto labeled_program = incre::labelCompress(program);
    auto align_info = incre::collectAlignType(labeled_program);
    std::vector<std::unordered_set<std::string>> cared_vals(align_info.size());
    for (const auto& info: align_info) {
        info->print();
        for (auto& [name, _]: info->inp_types) {
            cared_vals[info->getId()].insert(name);
        }
    }

    auto* pool = new NoDuplicatedIncreExamplePool(labeled_program, env, cared_vals);

    // build components
    auto component_pool = incre::grammar::collectComponent(pool->ctx, env, labeled_program.get());
    return new IncreInfo(labeled_program, pool->ctx, align_info, pool, component_pool);
}