//
// Created by pro on 2022/9/23.
//

#include "istool/incre/analysis/incre_instru_info.h"
#include "istool/incre/trans/incre_trans.h"
#include "glog/logging.h"

using namespace incre;

Ty incre::unfoldTypeWithLabeledCompress(const Ty &type, TypeContext *ctx) {
    auto compress_sem = [](const Ty& type, TypeContext* ctx, std::vector<std::string>& names, const ExternalUnfoldMap& ext) -> Ty {
        auto* lt = dynamic_cast<TyLabeledCompress*>(type.get()); assert(lt);
        auto content = incre::unfoldTypeAll(lt->content, ctx, names, ext);
        return std::make_shared<TyLabeledCompress>(content, lt->id);
    };
    ExternalUnfoldRule compress_rule = {compress_sem};
    std::vector<std::string> tmp_names;
    return incre::unfoldTypeAll(type, ctx, tmp_names, {{TyType::COMPRESS, compress_rule}});
}

namespace {
    void _runCommand(const Command& command, TypeContext* ctx, const std::function<Ty(const Term&, TypeContext*)>& type_checker);

    void _runCommand(CommandImport* c, TypeContext* ctx, const std::function<Ty(const Term&, TypeContext*)>& type_checker) {
        for (const auto& command: c->commands) {
            _runCommand(command, ctx, type_checker);
        }
    }
    // todo: Check whether there will some issue with LabeledCompress
    void _runCommand(CommandDefInductive* c, TypeContext* ctx) {
        auto* ty = c->type; ctx->bind(ty->name, c->_type);
        for (const auto& [name, _]: ty->constructors) {
            auto inp_type = incre::getConstructor(c->_type, name);
            ctx->bind(name, std::make_shared<TyArrow>(inp_type, c->_type));
        }
    }
    void _runCommand(CommandBind* c, TypeContext* ctx, const std::function<Ty(const Term&, TypeContext*)>& type_checker) {
        switch (c->binding->getType()) {
            case BindingType::TYPE: {
                auto* binding = dynamic_cast<TypeBinding*>(c->binding.get());
                ctx->bind(c->name, binding->type);
                break;
            }
            case BindingType::TERM: {
                auto* binding = dynamic_cast<TermBinding*>(c->binding.get());
                ctx->bind(c->name, type_checker(binding->term, ctx));
            }
        }
    }

    void _runCommand(const Command& command, TypeContext* ctx, const std::function<Ty(const Term&, TypeContext*)>& type_checker) {
        switch (command->getType()) {
            case CommandType::IMPORT: {
                _runCommand(dynamic_cast<CommandImport*>(command.get()), ctx, type_checker);
                return;
            }
            case CommandType::DEF_IND: {
                _runCommand(dynamic_cast<CommandDefInductive*>(command.get()), ctx);
                return;
            }
            case CommandType::BIND: {
                _runCommand(dynamic_cast<CommandBind*>(command.get()), ctx, type_checker);
                return;
            }
        }
    }

    class _MarkedTypeContext: public TypeContext {
    public:
        std::unordered_map<std::string, int> mark_count;
        void clear() {mark_count.clear();}
        virtual BindLog bind(const std::string& name, const Ty& type) {
            ++mark_count[name]; return TypeContext::bind(name, type);
        }
        virtual void cancelBind(const BindLog& log) {
            --mark_count[log.name]; TypeContext::cancelBind(log);
        }
    };
}

// todo: add unfold programs
AlignTypeInfoList incre::collectAlignType(const IncreProgram &program) {
    AlignTypeInfoList info;
    int command_id = 0;

    auto label = [](const Term& term, TypeContext* ctx, const ExternalTypeMap& ext) {
        auto* ct = dynamic_cast<TmLabeledLabel*>(term.get()); assert(ct);
        return std::make_shared<TyLabeledCompress>(incre::getType(ct->content, ctx, ext), ct->id);
    };
    auto align = [&info, &command_id](const Term& term, TypeContext* ctx, const ExternalTypeMap& ext) {
        auto* ct = dynamic_cast<TmLabeledAlign*>(term.get()); assert(ct);
        int id = ct->id;
        while (info.size() <= id) info.emplace_back();

        auto* mark_content = dynamic_cast<_MarkedTypeContext*>(ctx);
        std::unordered_map<std::string, Ty> inps;
        for (const auto& [name, type]: ctx->binding_map) {
            if (mark_content->mark_count[name]) inps[name] = incre::unfoldTypeWithLabeledCompress(type, ctx);
        }

        auto res = incre::getType(ct->content, ctx, ext);
        info[id] = std::make_shared<AlignTypeInfoData>(term, inps, res, command_id);
        return res;
    };

    ExternalTypeMap ext({{TermType::LABEL, ExternalTypeRule{label}}, {TermType::ALIGN, ExternalTypeRule{align}}});

    auto type_checker = [ext](const Term& term, TypeContext* ctx) {
        return incre::getType(term, ctx, ext);
    };
    auto* ctx = new _MarkedTypeContext();
    for (command_id = 0; command_id < program->commands.size(); ++command_id) {
        _runCommand(program->commands[command_id], ctx, type_checker);
        ctx->clear();
    }
    delete ctx;
    return info;
}