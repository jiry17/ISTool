//
// Created by pro on 2023/4/5.
//

#include "istool/incre/grammar/incre_component_collector.h"
#include "istool/incre/trans/incre_trans.h"
#include "istool/incre/analysis/incre_instru_info.h"
#include "glog/logging.h"

using namespace incre;
using namespace incre::grammar;

namespace {
    struct _UserProvidedComponentInfo {
        std::string name;
        bool is_compress_related, is_recursive;
        int command_id;
        _UserProvidedComponentInfo(const std::string& _name, bool _is_compress_related, bool _is_recursive, int _command_id):
            name(_name), is_compress_related(_is_compress_related), is_recursive(_is_recursive), command_id(_command_id) {
        }
        ~_UserProvidedComponentInfo() = default;
    };
    typedef std::unordered_map<std::string, _UserProvidedComponentInfo> _UserComponentInfoMap;

    _UserComponentInfoMap _constructInfoMap(ProgramData* program) {
        match::MatchTask compress_task, recursive_task;
        compress_task.term_matcher = [](TermData* term, const match::MatchContext& ctx) -> bool{
            return term->getType() == TermType::LABEL || term->getType() == TermType::UNLABEL || term->getType() == TermType::ALIGN;
        };
        compress_task.type_matcher = [](TyData* term, const match::MatchContext& ctx) -> bool {
            return term->getType() == TyType::COMPRESS;
        };
        recursive_task.term_matcher = [](TermData* term, const match::MatchContext& ctx) -> bool {
            return term->getType() == TermType::FIX;
        };
        auto compress_info = match::match(program, compress_task), recursive_info = match::match(program, recursive_task);

        // Collect command id infos
        std::unordered_map<std::string, int> id_map;
        for (int command_id = 0; command_id < program->commands.size(); ++command_id) {
            auto& command = program->commands[command_id];
            switch (command->getType()) {
                case CommandType::IMPORT: break;
                case CommandType::DEF_IND: {
                    auto* cd = dynamic_cast<CommandDefInductive*>(command.get());
                    id_map[cd->type->name] = command_id;
                    for (auto& [cname, _]: cd->type->constructors) {
                        id_map[cname] = command_id;
                    }
                    break;
                }
                case CommandType::BIND: {
                    auto* cb = dynamic_cast<CommandBind*>(command.get());
                    id_map[cb->name] = command_id;
                }
            }
        }

        // Merge all infos
        _UserComponentInfoMap res;
        assert(id_map.size() == compress_info.size() && id_map.size() == recursive_info.size());
        for (auto& [name, command_id]: id_map) {
            assert(compress_info.count(name) && recursive_info.count(name));
            res.insert({name, _UserProvidedComponentInfo(name, compress_info[name], recursive_info[name], command_id)});
        }
        return res;
    }
}

ComponentPool collector::collectComponentFromSource(Context* ctx, Env* env, ProgramData *program) {
    auto pool = collector::getBasicComponentPool(ctx, env, false);
    LOG(INFO) << "Begin construct info map";
    auto info_map = _constructInfoMap(program);
    LOG(INFO) << "End construct info map";
    auto* type_ctx = new TypeContext(ctx);
    for (const auto& [name, binding]: ctx->binding_map) {
        if (binding->getType() != BindingType::TERM) continue;
        auto it = info_map.find(name); assert(it != info_map.end());
        auto component_info = it->second;

        auto full_type = incre::unfoldBasicType(ctx->getType(name), type_ctx);
        auto component_type = incre::typeFromIncre(full_type);
        auto term = std::make_shared<TmVar>(name);
        auto component_value = incre::run(term, ctx);
        auto component = std::make_shared<IncreComponent>(name, component_type, component_value, term, component_info.command_id);

        if (component_info.is_compress_related) continue;
        if (!component_info.is_recursive) {
            pool.compress_list.push_back(component);
            pool.comb_list.push_back(component);
        }
        pool.align_list.push_back(component);
    }
    delete type_ctx;
    LOG(INFO) << "Print Component Pool";
    pool.print();
    int kk; std::cin >> kk;
    return pool;
}