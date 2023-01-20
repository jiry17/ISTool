//
// Created by pro on 2022/9/23.
//

#include "istool/incre/analysis/incre_instru_info.h"
#include "istool/incre/trans/incre_trans.h"
#include "istool/ext/deepcoder/deepcoder_semantics.h"
#include "istool/sygus/theory/theory.h"
#include "istool/sygus/theory/basic/theory_semantics.h"
#include <iostream>

using namespace incre;

AlignTypeInfoData::AlignTypeInfoData(const Term& __term, const std::unordered_map<std::string, Ty> &type_ctx, const Ty &_oup_type, int _command_id):
    oup_type(_oup_type), _term(__term), term(dynamic_cast<TmLabeledAlign*>(__term.get())), command_id(_command_id) {
    auto inps = incre::getUnboundedVars(term->content.get());
    for (const auto& inp: inps) {
        auto it = type_ctx.find(inp);
        if (it != type_ctx.end()) {
            inp_types.emplace_back(inp, it->second);
        }
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

IncreInfo::IncreInfo(const IncreProgram &_program, Context *_ctx, const AlignTypeInfoList &infos, IncreExamplePool *pool, const std::vector<SynthesisComponent *> &_component_list):
    program(_program), ctx(_ctx), align_infos(infos), example_pool(pool), component_list(_component_list) {
}
IncreInfo::~IncreInfo() {
    delete ctx; delete example_pool;
    for (auto* component: component_list) delete component;
}

void incre::prepareEnv(Env *env) {
    theory::loadBasicSemantics(env, TheoryToken::CLIA);
    ext::ho::loadDeepCoderSemantics(env);
}

IncreInfo* incre::buildIncreInfo(const IncreProgram &program, Env* env) {
    //auto labeled_program = incre::eliminateUnboundedCreate(program);
    incre::checkAllLabelBounded(program.get());
    auto labeled_program = incre::labelCompress(program);
    auto pass_info = incre::collectAlignType(labeled_program);
    for (const auto& info: pass_info) {
        info->print();
    }

    auto* pool = new IncreExamplePool(nullptr, nullptr);
    Context* ctx = incre::buildCollectContext(labeled_program, pool);
    pool->ctx = ctx;

    // build generator
    int command_num = labeled_program->commands.size();
    assert(command_num);
    auto* command = dynamic_cast<CommandBind*>(labeled_program->commands[command_num - 1].get());
    assert(command);
    auto name = command->name;
    auto* type_ctx = new TypeContext(ctx);
    auto type = incre::unfoldTypeWithLabeledCompress(ctx->getType(name), type_ctx);
    delete type_ctx;
    auto* generator = new DefaultStartTermGenerator(name, type.get());
    pool->generator = generator;

    // build components
    auto component_list = incre::collectComponentList(ctx, env, incre::getComponentInfo(program));
    return new IncreInfo(labeled_program, ctx, pass_info, pool, component_list);
}