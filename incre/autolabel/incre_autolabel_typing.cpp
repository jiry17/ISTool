//
// Created by pro on 2023/1/22.
//

#include "istool/incre/autolabel/incre_autolabel.h"
#include "glog/logging.h"

using namespace incre;
using namespace incre::syntax;
using namespace incre::autolabel;

autolabel::TyZ3LabeledCompress::TyZ3LabeledCompress(const syntax::Ty &content, const z3::expr &_label):
    syntax::TyCompress(content), label(_label) {
}

std::string TyZ3LabeledCompress::toString() const {
    return "[" + label.to_string() + "]" + body->toString();
}

z3::expr Z3Context::getBooleanVar() {
    auto name = "tmp" + std::to_string(tmp_id++);
    return ctx.bool_const(name.c_str());
}

SymbolicIncreTypeChecker::SymbolicIncreTypeChecker(Z3Context *_z3_ctx):
    types::DefaultIncreTypeChecker(new Z3LabeledRangeInfoManager(_z3_ctx->ctx)), z3_ctx(_z3_ctx), cons_list(_z3_ctx->ctx) {
}

Z3LabeledRangeInfoManager::Z3LabeledRangeInfoManager(z3::context &ctx): cons_list(ctx), KTrue(ctx.bool_val(true)), KFalse(ctx.bool_val(false)) {
}

Z3LabeledRangeInfo::Z3LabeledRangeInfo(const z3::expr &_scalar_cond, const z3::expr &_base_cond):
    scalar_cond(_scalar_cond.simplify()), base_cond(_base_cond.simplify()) {
}

std::string Z3LabeledRangeInfo::toString() const {
    return "[Scalr:" + scalar_cond.to_string() + ", Base:" + base_cond.to_string() + "]";
}

syntax::PVarRange Z3LabeledRangeInfoManager::constructBasicRange(syntax::BasicRangeType range) {
    switch (range) {
        case BasicRangeType::SCALAR: return std::make_shared<Z3LabeledRangeInfo>(KTrue, KTrue);
        case BasicRangeType::BASE: return std::make_shared<Z3LabeledRangeInfo>(KFalse, KTrue);
        case BasicRangeType::ANY: return std::make_shared<Z3LabeledRangeInfo>(KTrue, KTrue);
    }
}

namespace {
    std::pair<z3::expr, z3::expr> unfoldRange(VarRange* range) {
        auto* zr = dynamic_cast<Z3LabeledRangeInfo*>(range);
        assert(zr);
        return {zr->scalar_cond, zr->base_cond};
    }
}

syntax::PVarRange Z3LabeledRangeInfoManager::intersectRange(const syntax::PVarRange &x, const syntax::PVarRange &y) {
    auto [x_scalar, x_base] = unfoldRange(x.get());
    auto [y_scalar, y_base] = unfoldRange(y.get());
    return std::make_shared<Z3LabeledRangeInfo>(x_scalar | y_scalar, x_base | y_base);
}

namespace {
    void _collectScalarConstraint(TypeData* type, const z3::expr& cond, z3::expr_vector& cons_list) {
        if (type->getType() == TypeType::VAR) {
            auto* tv = dynamic_cast<TyVar*>(type); assert(tv);
            if (!tv->is_bounded()) {
                auto [_index, _level, info] = tv->get_var_info();
                auto [scalar_cond, base_cond] = unfoldRange(info.get());
                tv->updateRange(std::make_shared<Z3LabeledRangeInfo>(scalar_cond | cond, base_cond));
                return;
            }
        }
        if (type->getType() == TypeType::ARR || type->getType() == TypeType::IND) {
            cons_list.push_back(!cond); return;
        }
        if (type->getType() == TypeType::COMPRESS) {
            auto* tc = dynamic_cast<TyCompress*>(type);
            auto [label, body] = util::unfoldCompressType(tc, cond.ctx());
            _collectScalarConstraint(body.get(), cond & !label, cons_list);
            return;
        }
        for (auto& sub_type: getSubTypes(type)) {
            _collectScalarConstraint(sub_type.get(), cond, cons_list);
        }
    }

    void _collectBaseConstraint(TypeData* type, const z3::expr& cond, z3::expr_vector& cons_list) {
        if (type->getType() == TypeType::VAR) {
            auto* tv = dynamic_cast<TyVar*>(type);
            if (!tv->is_bounded()) {
                auto [_index, _level, info] = tv->get_var_info();
                auto [scalar_cond, base_cond] = unfoldRange(info.get());
                tv->updateRange(std::make_shared<Z3LabeledRangeInfo>(scalar_cond, base_cond & cond));
                return;
            }
        }
        if (type->getType() == TypeType::ARR) {
            cons_list.push_back(!cond); return;
        }
        for (auto& sub_type: getSubTypes(type)) {
            _collectBaseConstraint(sub_type.get(), cond, cons_list);
        }
    }
}

void Z3LabeledRangeInfoManager::checkAndUpdate(const syntax::PVarRange &x, syntax::TypeData *type) {
    auto* xr = dynamic_cast<Z3LabeledRangeInfo*>(x.get()); assert(xr);
    _collectScalarConstraint(type, xr->scalar_cond, cons_list);
    _collectBaseConstraint(type, xr->base_cond, cons_list);
}

void SymbolicIncreTypeChecker::unify(const syntax::Ty &x, const syntax::Ty &y) {
    if (x->getType() == TypeType::VAR) return types::DefaultIncreTypeChecker::unify(x, y);
    if (y->getType() == TypeType::VAR) return types::DefaultIncreTypeChecker::unify(y, x);
    auto [x_label, x_body] = util::unfoldCompressType(x, z3_ctx->ctx);
    auto [y_label, y_body] = util::unfoldCompressType(y, z3_ctx->ctx);
    cons_list.push_back(x_label == y_label);
    types::DefaultIncreTypeChecker::unify(x_body, y_body);
}

namespace {
    bool _isClearArrow(TypeData* type) {
        if (type->getType() == TypeType::ARR) return true;
        if (type->getType() == TypeType::COMPRESS) return false;
        for (auto& sub_type: getSubTypes(type)) {
            if (_isClearArrow(sub_type.get())) return true;
        }
        return false;
    }
}

syntax::Ty
SymbolicIncreTypeChecker::postProcess(syntax::TermData *term, const IncreContext &ctx, const syntax::Ty &res) {
    if (_isClearArrow(res.get())) return res;
    auto label_var = z3_ctx->getBooleanVar(), unlabel_var = z3_ctx->getBooleanVar();
    cons_list.push_back(!(label_var & unlabel_var));
    _collectBaseConstraint(res.get(), label_var | unlabel_var, cons_list);

    auto [raw_label, raw_body] = util::unfoldCompressType(res, z3_ctx->ctx);
    cons_list.push_back(z3::implies(unlabel_var, raw_label));
    cons_list.push_back(z3::implies(label_var, !raw_label));
    auto new_label = label_var | (raw_label & unlabel_var);
    auto new_res = std::make_shared<TyZ3LabeledCompress>(new_label, raw_body);

}