//
// Created by pro on 2022/12/6.
//

#ifndef ISTOOL_INCRE_AUTOLABEL_H
#define ISTOOL_INCRE_AUTOLABEL_H

#include "istool/incre/language/incre.h"

namespace incre::autolabel {
    class TyLabeledInductive: public TyInductive {
    public:
        z3::expr is_compress;
        TyLabeledInductive(const z3::expr& _is_compress, const std::string& name, const std::vector<std::pair<std::string, Ty>>& cons_list);
        virtual ~TyLabeledInductive() = default;
        virtual std::string toString() const;
    };

    class TyLabeledTuple: public TyTuple {
    public:
        z3::expr is_compress;
        TyLabeledTuple(const z3::expr& _is_compress, const TyList& type_list);
        virtual ~TyLabeledTuple() = default;
        virtual std::string toString() const;
    };

    class LabelContext: public TypeContext {
    public:
        z3::context ctx;
    private:
        std::unordered_map<std::string, int> tmp_ind_map;
        z3::expr getTmpVar(const std::string& prefix);
        z3::model model;
    public:
        z3::expr_vector cons_list;
        std::unordered_map<TermData*, z3::expr> flip_map, possible_pass_map, unlabel_info_map;
        std::unordered_map<TermData*, Ty> labeled_type_map;
        std::unordered_map<TyData*, z3::expr> type_compress_map;
        bool getLabel(const z3::expr& expr) const;
        void addConstraint(const z3::expr& cons);
        LabelContext();
        ~LabelContext();
        z3::expr getFlipVar(TermData* term);
        z3::expr getPossiblePassVar(TermData* term);
        void addUnlabelInfo(TermData* term, const z3::expr& expr);
        void registerType(TermData* term, const Ty& type);
        z3::expr getCompressVar(TyData* type);

        virtual void setModel(const z3::model& _model);
    };

    class IncreLabelSolver {
    public:
        virtual z3::model solve(LabelContext* ctx, ProgramData* data) = 0;
        virtual ~IncreLabelSolver() = default;
    };

    bool isCompressibleType(const Ty& type, TypeContext* ctx);

    Ty unfoldTypeWithZ3Labels(const Ty& type, TypeContext* ctx);
    bool isLabeledType(TyData* type);
    z3::expr getLabel(TyData* type);
    Ty changeLabel(TyData* type, const z3::expr& new_label);

    Ty labelType(const Ty& type, LabelContext* ctx);
    Ty labelTerm(const Term& term, LabelContext* ctx);
    void initProgramLabel(ProgramData* program, LabelContext* ctx);

    Ty constructType(const Ty& type, LabelContext* ctx);
    Term constructTerm(const Term& term, LabelContext* ctx);

    IncreProgram labelProgram(ProgramData* program);
}

#endif //ISTOOL_INCRE_AUTOLABEL_H
