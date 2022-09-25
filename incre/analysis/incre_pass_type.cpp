//
// Created by pro on 2022/9/23.
//

#include "istool/incre/analysis/incre_instru_info.h"
#include "glog/logging.h"

using namespace incre;

namespace {
    Ty _unfoldAll(const Ty& x, TypeContext* ctx, std::vector<std::string>& tmps);

#define UnfoldAllCase(name) return _unfoldAll(dynamic_cast<Ty ## name*>(x.get()), x, ctx, tmps)
#define UnfoldAllHead(name) Ty _unfoldAll(Ty ## name* x, const Ty& _x, TypeContext* ctx, std::vector<std::string>& tmps)

    UnfoldAllHead(Var) {
        for (auto& name: tmps) if (x->name == name) return _x;
        return _unfoldAll(ctx->lookup(x->name), ctx, tmps);
    }
    UnfoldAllHead(LabeledCompress) {
        auto content = _unfoldAll(x->content, ctx, tmps);
        return std::make_shared<TyLabeledCompress>(content, x->id);
    }
    UnfoldAllHead(Tuple) {
        TyList fields;
        for (const auto& field: x->fields) {
            fields.push_back(_unfoldAll(field, ctx, tmps));
        }
        return std::make_shared<TyTuple>(fields);
    }
    UnfoldAllHead(Inductive) {
        tmps.push_back(x->name);
        std::vector<std::pair<std::string, Ty>> cons_list;
        for (const auto& [cname, cty]: x->constructors) {
            cons_list.emplace_back(cname, _unfoldAll(cty, ctx, tmps));
        }
        tmps.pop_back();
        return std::make_shared<TyInductive>(x->name, cons_list);
    }
    UnfoldAllHead(Arrow) {
        auto source = _unfoldAll(x->source, ctx, tmps);
        auto target = _unfoldAll(x->target, ctx, tmps);
        return std::make_shared<TyArrow>(source, target);
    }

    Ty _unfoldAll(const Ty& x, TypeContext* ctx, std::vector<std::string>& tmps) {
        switch (x->getType()) {
            case TyType::INT:
            case TyType::UNIT:
            case TyType::BOOL:
                return x;
            case TyType::VAR: UnfoldAllCase(Var);
            case TyType::COMPRESS: UnfoldAllCase(LabeledCompress);
            case TyType::TUPLE: UnfoldAllCase(Tuple);
            case TyType::IND: UnfoldAllCase(Inductive);
            case TyType::ARROW: UnfoldAllCase(Arrow);
        }
    }
}

Ty incre::unfoldAll(const Ty& x, TypeContext* ctx) {
    std::vector<std::string> tmps;
    return _unfoldAll(x, ctx, tmps);
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
}

// todo: add unfold programs
PassTypeInfoList incre::collectPassType(const IncreProgram &program) {
    PassTypeInfoList info;
    auto create = [](const Term& term, TypeContext* ctx, const ExternalTypeMap& ext) {
         auto* ct = dynamic_cast<TmLabeledCreate*>(term.get());
         assert(ct);
         auto res = incre::getType(ct->def, ctx, ext);
         return std::make_shared<TyLabeledCompress>(res, ct->id);
    };
    auto pass = [&info](const Term& term, TypeContext* ctx, const ExternalTypeMap& ext) {
        auto* pt = dynamic_cast<TmLabeledPass*>(term.get());
        assert(pt); int id = pt->tau_id;
        while (info.size() <= id) info.emplace_back();

        std::vector<TypeContext::BindLog> bind_list;
        TyList defs;
        std::unordered_map<std::string, Ty> inps;
        for (const auto& [name, type]: ctx->binding_map) {
            inps[name] = incre::unfoldAll(type, ctx);
        }
        for (const auto& def: pt->defs) {
            defs.push_back(incre::getType(def, ctx, ext));
        }
        for (int i = 0; i < defs.size(); ++i) {
            auto name = pt->names[i];
            auto* ct = dynamic_cast<TyLabeledCompress*>(defs[i].get());
            assert(ct);
            bind_list.push_back(ctx->bind(name, ct->content));
            inps[name] = incre::unfoldAll(defs[i], ctx);
        }
        auto res = incre::getType(pt->content, ctx, ext);
        for (int i = int(bind_list.size()) - 1; i >= 0; --i) {
            ctx->cancelBind(bind_list[i]);
        }

        info[id] = std::make_shared<PassTypeInfoData>(pt, inps, res);
        return res;
    };

    ExternalTypeMap ext({{TermType::CREATE, ExternalTypeRule{create}}, {TermType::PASS, ExternalTypeRule{pass}}});

    auto type_checker = [ext](const Term& term, TypeContext* ctx) {
        return incre::getType(term, ctx, ext);
    };
    auto* ctx = new TypeContext();
    for (const auto& command: program->commands) {
        _runCommand(command, ctx, type_checker);
    }
    delete ctx;
    return info;
}