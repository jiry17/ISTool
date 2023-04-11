//
// Created by pro on 2023/4/11.
//

#include "istool/incre/grammar/incre_component_collector.h"
#include "istool/incre/trans/incre_trans.h"
#include "glog/logging.h"

using namespace incre::grammar;

ComponentPool collector::collectComponentFromLabel(Context *ctx, Env *env, ProgramData *program) {
    auto* type_ctx = new TypeContext(ctx);
    ComponentPool res = collector::getBasicComponentPool(ctx, env, true);

    for (int command_id = 0; command_id < program->commands.size(); ++command_id) {
        auto& command = program->commands[command_id];
        auto* cb = dynamic_cast<CommandBind*>(command.get());
        if (!cb) continue;
        auto* tb = dynamic_cast<TermBinding*>(cb->binding.get());
        if (!tb) continue;
        auto term = std::make_shared<TmVar>(cb->name);
        auto type = ctx->getType(cb->name);
        auto full_type = incre::unfoldBasicType(type, type_ctx);
        auto component = std::make_shared<IncreComponent>(cb->name, incre::typeFromIncre(full_type), incre::run(term, ctx), term, command_id);
        if (command->isDecoratedWith(CommandDecorate::SYN_COMPRESS)) {
            res.compress_list.push_back(component);
        }
        if (command->isDecoratedWith(CommandDecorate::SYN_COMBINE)) {
            res.comb_list.push_back(component);
        }
        if (command->isDecoratedWith(CommandDecorate::SYN_ALIGN)) {
            res.align_list.push_back(component);
        }
    }
    delete type_ctx;
    return res;
}