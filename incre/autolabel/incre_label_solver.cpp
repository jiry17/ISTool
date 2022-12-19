//
// Created by pro on 2022/12/18.
//

#include "istool/incre/autolabel/incre_autolabel_solver.h"
#include "glog/logging.h"

using namespace incre;
using namespace incre::autolabel;

z3::model BasicIncreLabelSolver::solve(LabelContext *ctx, ProgramData *program) {
    z3::solver solver(ctx->ctx);
    solver.add(ctx->cons_list);
    auto res = solver.check();
    if (res == z3::unsat) {
        LOG(FATAL) << "No valid label scheme exists";
    }
    return solver.get_model();
}

namespace {
    class ExtraLabelContext{
    public:
        z3::context* ctx;
        z3::expr_vector cons_list;
        std::unordered_map<TermData*, z3::expr> var_map;
        int ind = 0;
        ExtraLabelContext(z3::context* _ctx): ctx(_ctx), cons_list(*ctx) {}

        void addCons(const z3::expr& cons) {
            cons_list.push_back(cons);
        }
        z3::expr registerTerm(TermData* term) {
            auto it = var_map.find(term);
            if (it != var_map.end()) return it->second;
            auto name = "cover@" + std::to_string(ind++);
            auto var = ctx->bool_const(name.c_str());
            var_map.insert({term, var});
            return var;
        }
    };

    z3::expr _collectExtra(const Term& term, LabelContext* label_ctx, ExtraLabelContext* extra_ctx) {
        auto label = extra_ctx->registerTerm(term.get());
        auto sub_terms = incre::getSubTerms(term.get());
        for (auto& sub: sub_terms) {
            auto sub_label = _collectExtra(sub, label_ctx, extra_ctx);
            extra_ctx->addCons(z3::implies(label, sub_label));
        }
        auto it = label_ctx->possible_pass_map.find(term.get());
        if (it != label_ctx->possible_pass_map.end()) {
            extra_ctx->addCons(z3::implies(it->second, label));
        }
        return label;
    }
}

z3::model MinCoverIncreLabelSolver::solve(LabelContext *ctx, ProgramData *program) {
    auto* extra_ctx = new ExtraLabelContext(&ctx->ctx);

    // TODO: deal with names involved by inductive
    for (const auto& command: program->commands) {
        auto* cb = dynamic_cast<CommandBind*>(command.get());
        if (!cb) continue;
        auto* tb = dynamic_cast<TermBinding*>(cb->binding.get());
        if (!tb) continue;
        _collectExtra(tb->term, ctx, extra_ctx);
    }

    z3::optimize o(ctx->ctx);
    o.add(ctx->cons_list);
    o.add(extra_ctx->cons_list);
    for (const auto& [term, var]: extra_ctx->var_map) {
        o.add_soft(!var, 1);
    }
    auto res = o.check();
    if (res == z3::unsat) {
        LOG(FATAL) << "No valid label scheme exists";
    }
    return o.get_model();
}

namespace {
    void _collectCompressedLabel(TyData* type, z3::expr_vector& tmps) {
        switch (type->getType()) {
            case TyType::VAR:
            case TyType::INT:
            case TyType::BOOL:
            case TyType::UNIT: return;
            case TyType::ARROW: {
                auto* ta = dynamic_cast<TyArrow*>(type);
                _collectCompressedLabel(ta->source.get(), tmps);
                _collectCompressedLabel(ta->target.get(), tmps);
                return;
            }
            case TyType::TUPLE: {
                auto* tt = dynamic_cast<TyLabeledTuple*>(type);
                tmps.push_back(tt->is_compress);
                for (auto& field: tt->fields) _collectCompressedLabel(field.get(), tmps);
                return;
            }
            case TyType::IND: {
                auto* ti = dynamic_cast<TyLabeledInductive*>(type);
                tmps.push_back(ti->is_compress);
                for (auto& [name, cons_type]: ti->constructors) {
                    _collectCompressedLabel(cons_type.get(), tmps);
                }
                return;
            }
            case TyType::COMPRESS: {
                LOG(FATAL) << "Unexpected TyType COMPRESS";
            }
        }
    }

    z3::expr _collectCompressedLabel(TyData* type, z3::context& ctx) {
        z3::expr_vector tmp(ctx);
        _collectCompressedLabel(type, tmp);
        return z3::mk_or(tmp);
    }
}

z3::model MinInvolvedIncreLabelSolver::solve(LabelContext *ctx, ProgramData *program) {
    z3::optimize o(ctx->ctx); o.add(ctx->cons_list);

    int id = 0;
    for (auto& [term, type]: ctx->labeled_type_map) {
        auto name = "c@" + std::to_string(id++);
        auto var = ctx->ctx.bool_const(name.c_str());
        o.add(var == _collectCompressedLabel(type.get(), ctx->ctx));
        o.add_soft(!var, 1);
    }

    auto res = o.check();
    if (res == z3::unsat) {
        LOG(FATAL) << "No valid label scheme exists";
    }
    return o.get_model();
}

z3::model MinMixedIncerLbaleSolver::solve(LabelContext *ctx, ProgramData *program) {

    auto* extra_ctx = new ExtraLabelContext(&ctx->ctx);

    // TODO: deal with names involved by inductive
    for (const auto& command: program->commands) {
        auto* cb = dynamic_cast<CommandBind*>(command.get());
        if (!cb) continue;
        auto* tb = dynamic_cast<TermBinding*>(cb->binding.get());
        if (!tb) continue;
        _collectExtra(tb->term, ctx, extra_ctx);
    }

    z3::optimize o(ctx->ctx);
    o.add(ctx->cons_list);
    o.add(extra_ctx->cons_list);
    for (const auto& [term, var]: extra_ctx->var_map) {
        auto it = ctx->labeled_type_map.find(term);
        if (it == ctx->labeled_type_map.end()) continue;
        o.add_soft(!var && !(_collectCompressedLabel(it->second.get(), ctx->ctx)), 1);
    }
    auto res = o.check();
    if (res == z3::unsat) {
        LOG(FATAL) << "No valid label scheme exists";
    }
    return o.get_model();
}