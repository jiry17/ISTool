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

IncreInfo::IncreInfo(const IncreProgram &_program, EnvContext *_ctx, const AlignTypeInfoList &infos, IncreExamplePool *pool, const grammar::ComponentPool& _pool):
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

#include "istool/incre/io/incre_printer.h"

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

    auto builder = getCollectorBuilder(CollectorType::ENV);
    auto* pool = new NoDuplicatedIncreExamplePool(labeled_program, env, cared_vals, builder);

    // build components
    auto component_pool = incre::grammar::collectComponent(pool->ctx, pool->type_ctx, env, labeled_program.get());
    return new IncreInfo(labeled_program, pool->ctx, align_info, pool, component_pool);
}

std::pair<std::vector<std::string>, Grammar *> incre::buildFinalGrammar(IncreInfo *info, int align_id, const TyList &final_compress_list) {
    int command_id = info->align_infos[align_id]->command_id;

    grammar::SynthesisComponentList component_list;
    std::unordered_set<std::string> used_map;
    for (auto& component: info->component_pool.compress_list) {
        if (used_map.find(component->name) != used_map.end()) continue;
        used_map.insert(component->name);
        if (component->command_id < command_id) component_list.push_back(component);
    }
    for (auto& component: info->component_pool.comb_list) {
        if (used_map.find(component->name) != used_map.end()) continue;
        used_map.insert(component->name);
        if (component->command_id < command_id) component_list.push_back(component);
    }

    TypeList inp_type_list;
    std::vector<std::string> param_names;
    for (auto& [name, var_ty]: info->align_infos[align_id]->inp_types) {
        auto type = incre::typeFromIncre(incre::getFinalType(var_ty, final_compress_list));
        inp_type_list.push_back(type); param_names.push_back(name);
    }
    for (auto& [name, inp_ty]: info->example_pool->input_list) {
        auto type = incre::typeFromIncre(incre::getFinalType(inp_ty, final_compress_list));
        inp_type_list.push_back(type); param_names.push_back(name);
    }

    auto oup_type = incre::typeFromIncre(incre::getFinalType(info->align_infos[align_id]->oup_type, final_compress_list));
    auto grammar = grammar::builder::buildGrammar(inp_type_list, component_list, oup_type);

    return std::make_pair(param_names, grammar);
}