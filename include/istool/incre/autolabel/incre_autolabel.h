//
// Created by pro on 2023/1/22.
//

#ifndef ISTOOL_INCRE_AUTOLABEL_H
#define ISTOOL_INCRE_AUTOLABEL_H

#include "istool/incre/language/incre_types.h"

namespace incre::autolabel {
    class TyZ3LabeledCompress: public syntax::TyCompress {
    public:
        z3::expr label;
        TyZ3LabeledCompress(const syntax::Ty& content, const z3::expr& _label);
        virtual std::string toString() const;
        ~TyZ3LabeledCompress() = default;
    };

    class Z3LabeledRangeInfo: public syntax::VarRange {
    public:
        z3::expr scalar_cond, base_cond; // under which conditions this variable will be limited to scalar/base
        Z3LabeledRangeInfo(const z3::expr& _scalar_cond, const z3::expr& _base_cond);
        virtual std::string toString() const;
    };

    class Z3LabeledRangeInfoManager: public types::VarRangeManager {
        z3::expr KTrue, KFalse;
    public:
        z3::expr_vector cons_list;
        Z3LabeledRangeInfoManager(z3::context& ctx);
        virtual syntax::PVarRange constructBasicRange(syntax::BasicRangeType range);
        virtual syntax::PVarRange intersectRange(const syntax::PVarRange& x, const syntax::PVarRange& y);
        virtual void checkAndUpdate(const syntax::PVarRange& x, syntax::TypeData* type);
    };

    class Z3Context {
    public:
        z3::context ctx;
        int tmp_id = 0;
        z3::expr getBooleanVar();
    };

    class SymbolicIncreTypeChecker: public types::DefaultIncreTypeChecker {
    protected:
        virtual syntax::Ty postProcess(syntax::TermData* term, const IncreContext& ctx, const syntax::Ty& res);
    public:
        std::unordered_map<syntax::TermData*, syntax::Ty> type_map;
        std::unordered_map<syntax::TermData*, z3::expr> label_map, unlabel_map, rewrite_map;
        z3::expr_vector cons_list;
        Z3Context* z3_ctx;

        SymbolicIncreTypeChecker(Z3Context* z3_ctx);
        virtual ~SymbolicIncreTypeChecker() = default;
        virtual void unify(const syntax::Ty& x, const syntax::Ty& y);
    };

    namespace util {
        std::pair<z3::expr, syntax::Ty> unfoldCompressType(const syntax::Ty& type, z3::context& ctx);
        std::pair<z3::expr, syntax::Ty> unfoldCompressType(syntax::TyCompress* type, z3::context& ctx);
    }
}

#endif //ISTOOL_INCRE_AUTOLABEL_H
