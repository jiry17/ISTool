//
// Created by pro on 2022/9/23.
//

#include "istool/incre/analysis/incre_instru_info.h"

using namespace incre;

PassTypeInfoData::PassTypeInfoData(TmLabeledPass* _term, const std::unordered_map<std::string, Ty> &type_ctx, const Ty &_oup_type):
    oup_type(_oup_type), term(_term) {
    auto inps = incre::getUnboundedVars(term->content.get());
    for (const auto& inp: inps) {
        auto it = type_ctx.find(inp);
        assert(it != type_ctx.end());
        inp_types.emplace_back(inp, it->second);
    }
}
void PassTypeInfoData::print() const {
    std::cout << "pass term #" << term->tau_id << ": " << oup_type->toString() << std::endl;
    std::cout << term->toString() << std::endl;
    for (const auto& [name, ty]: inp_types) {
        std::cout << "  " << name << ": " << ty->toString() << std::endl;
    }
}

FComponent::FComponent(const std::string &_name, const Ty &_type, const Data &_d):
    name(_name), type(_type), d(_d) {
}

IncreInfo::IncreInfo(const IncreProgram &_program, Context *_ctx, const PassTypeInfoList &_infos, IncreExamplePool *_pool):
    program(_program), ctx(_ctx), pass_infos(_infos), example_pool(_pool) {
}
IncreInfo::~IncreInfo() {
    delete ctx; delete example_pool;
}

IncreInfo* incre::buildIncreInfo(const IncreProgram &program) {
    auto labeled_program = incre::eliminateUnboundedCreate(program);
    labeled_program = incre::labelCompress(labeled_program);
    auto pass_info = incre::collectPassType(labeled_program);
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
    auto type = incre::unfoldAll(ctx->getType(name), type_ctx);
    delete type_ctx;
    auto* generator = new DefaultStartTermGenerator(name, type.get());
    pool->generator = generator;
    return new IncreInfo(labeled_program, ctx, pass_info, pool);
}